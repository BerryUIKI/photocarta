// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "directoryscanner.h"
#include "../metadata/exifparser.h"
#include "../metadata/xmpsidecar.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QMetaObject>
#include <QtConcurrent>

namespace PhotoCarta
{

DirectoryScanner::DirectoryScanner(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<CatalogItem>("CatalogItem");
    qRegisterMetaType<QList<CatalogItem>>("QList<CatalogItem>");
}

DirectoryScanner::~DirectoryScanner()
{
    cancel();
}

const QSet<QString> &DirectoryScanner::supportedExtensions()
{
    static const QSet<QString> extensions = {
        // Standard Web and Raster Formats
        QStringLiteral("jpg"),
        QStringLiteral("jpeg"),
        QStringLiteral("png"),
        QStringLiteral("webp"),
        QStringLiteral("gif"),
        QStringLiteral("bmp"),
        QStringLiteral("tif"),
        QStringLiteral("tiff"),
        QStringLiteral("heic"),
        QStringLiteral("heif"),
        QStringLiteral("avif"),
        // Common Digital Camera RAW Formats (digiKam interoperability)
        QStringLiteral("cr2"),
        QStringLiteral("cr3"),
        QStringLiteral("nef"),
        QStringLiteral("arw"),
        QStringLiteral("dng"),
        QStringLiteral("orf"),
        QStringLiteral("rw2"),
        QStringLiteral("pef"),
        QStringLiteral("raf")
    };
    return extensions;
}

bool DirectoryScanner::isSupportedImageFile(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    return supportedExtensions().contains(suffix);
}

CatalogItem DirectoryScanner::inspectFile(const QString &filePath)
{
    const QFileInfo fi(filePath);
    CatalogItem item;

    // Stable unique identifier derived from SHA-1 of canonical/absolute path
    const QString absPath = fi.absoluteFilePath();
    item.id = QString::fromLatin1(QCryptographicHash::hash(absPath.toUtf8(), QCryptographicHash::Sha1).toHex());
    item.filePath = absPath;
    item.fileName = fi.fileName();
    item.title = fi.completeBaseName();
    item.fileSizeBytes = fi.size();
    item.formattedFileSize = CatalogItem::formatBytes(item.fileSizeBytes);

    const QDateTime lastMod = fi.lastModified();
    item.fileLastModified = lastMod;
    item.formattedLastModified = lastMod.toString(QStringLiteral("yyyy-MM-dd hh:mm"));

    // Fast image metadata without loading full pixel buffer into memory
    QImageReader reader(absPath);
    reader.setAutoTransform(true);
    const QSize size = reader.size();
    if (size.isValid())
    {
        item.dimensions = size;
        item.formattedDimensions = QString::asprintf("%d × %d", size.width(), size.height());
    }

    // MIME type resolution
    static QMimeDatabase mimeDb;
    item.mimeType = mimeDb.mimeTypeForFile(fi).name();

    // Default dates and preview URLs
    item.dateTaken = lastMod;
    item.formattedDate = item.formattedLastModified;
    item.thumbnailUrl = QUrl(QStringLiteral("image://thumbnail/") + absPath);
    item.previewUrl = QUrl::fromLocalFile(absPath);
    item.isDemoFixture = false;
    item.isMissing = false;

    // Parse EXIF metadata (camera, lens, exposure, capture date)
    const ExifData exif = ExifParser::parseFile(absPath);
    if (exif.isValid)
    {
        if (!exif.cameraModel.isEmpty())
        {
            item.cameraModel = exif.cameraModel;
        }
        if (!exif.lensModel.isEmpty())
        {
            item.lensModel = exif.lensModel;
        }
        if (!exif.focalLength.isEmpty())
        {
            item.focalLength = exif.focalLength;
        }
        if (!exif.aperture.isEmpty())
        {
            item.aperture = exif.aperture;
        }
        if (!exif.shutterSpeed.isEmpty())
        {
            item.shutterSpeed = exif.shutterSpeed;
        }
        if (exif.iso > 0)
        {
            item.iso = exif.iso;
        }
        if (exif.dateTaken.isValid())
        {
            item.dateTaken = exif.dateTaken;
            item.formattedDate = exif.dateTaken.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
        }
        if (item.dimensions.isEmpty() && exif.dimensions.isValid())
        {
            item.dimensions = exif.dimensions;
            item.formattedDimensions = QString::asprintf("%d × %d", exif.dimensions.width(), exif.dimensions.height());
        }
    }

    // Inspect companion XMP sidecar if available (<file>.<ext>.xmp or <file>.xmp)
    if (XmpSidecar::hasSidecar(absPath))
    {
        XmpMetadata xmp;
        const QString sidecarPath = XmpSidecar::sidecarPathForImage(absPath, true);
        if (XmpSidecar::readFromFile(sidecarPath, xmp))
        {
            if (!xmp.title.isEmpty())
            {
                item.title = xmp.title;
            }
            if (!xmp.caption.isEmpty())
            {
                item.caption = xmp.caption;
            }
            if (xmp.rating > 0)
            {
                item.rating = xmp.rating;
            }
            if (xmp.colorLabel > 0)
            {
                item.colorLabel = xmp.colorLabel;
            }
            if (xmp.isFlagged)
            {
                item.isFlagged = true;
            }
            if (!xmp.tags.isEmpty())
            {
                item.tags = xmp.tags;
            }
            if (xmp.dateTaken.isValid())
            {
                item.dateTaken = xmp.dateTaken;
                item.formattedDate = xmp.dateTaken.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
            }
        }
    }

    return item;
}

bool DirectoryScanner::isScanning() const
{
    return m_isScanning.loadRelaxed() != 0;
}

void DirectoryScanner::cancel()
{
    if (m_isScanning.loadRelaxed())
    {
        m_cancelRequested.storeRelaxed(1);
    }
}

void DirectoryScanner::startScan(const QString &directoryPath, bool recursive)
{
    if (m_isScanning.loadRelaxed())
    {
        cancel();
    }

    m_isScanning.storeRelaxed(1);
    m_cancelRequested.storeRelaxed(0);

    emit scanStarted(directoryPath);

    // Run scanning in Qt thread pool to ensure UI responsiveness
    (void)QtConcurrent::run([this, directoryPath, recursive]() {
        doScan(directoryPath, recursive);
    });
}

void DirectoryScanner::doScan(const QString &directoryPath, bool recursive)
{
    QElapsedTimer timer;
    timer.start();

    const QDir::Filters filters = QDir::Files | QDir::Readable | QDir::NoDotAndDotDot;
    const QDirIterator::IteratorFlags flags = recursive
        ? (QDirIterator::Subdirectories | QDirIterator::FollowSymlinks)
        : QDirIterator::NoIteratorFlags;

    QDirIterator it(directoryPath, filters, flags);
    QList<CatalogItem> batch;
    batch.reserve(50);
    int totalFound = 0;

    while (it.hasNext())
    {
        if (m_cancelRequested.loadRelaxed())
        {
            m_isScanning.storeRelaxed(0);
            emit scanCanceled();
            return;
        }

        const QString currentFile = it.next();
        if (isSupportedImageFile(currentFile))
        {
            CatalogItem item = inspectFile(currentFile);
            batch.append(item);
            totalFound++;

            emit progress(totalFound, currentFile);

            if (batch.size() >= 50)
            {
                emit batchDiscovered(batch);
                batch.clear();
                batch.reserve(50);
            }
        }
    }

    if (!batch.isEmpty() && !m_cancelRequested.loadRelaxed())
    {
        emit batchDiscovered(batch);
        batch.clear();
    }

    m_isScanning.storeRelaxed(0);

    if (m_cancelRequested.loadRelaxed())
    {
        emit scanCanceled();
    }
    else
    {
        emit scanFinished(totalFound, timer.elapsed());
    }
}

QList<CatalogItem> DirectoryScanner::scanSynchronously(const QString &directoryPath, bool recursive)
{
    m_isScanning.storeRelaxed(1);
    m_cancelRequested.storeRelaxed(0);

    const QDir::Filters filters = QDir::Files | QDir::Readable | QDir::NoDotAndDotDot;
    const QDirIterator::IteratorFlags flags = recursive
        ? (QDirIterator::Subdirectories | QDirIterator::FollowSymlinks)
        : QDirIterator::NoIteratorFlags;

    QDirIterator it(directoryPath, filters, flags);
    QList<CatalogItem> results;

    while (it.hasNext())
    {
        if (m_cancelRequested.loadRelaxed())
        {
            break;
        }

        const QString currentFile = it.next();
        if (isSupportedImageFile(currentFile))
        {
            results.append(inspectFile(currentFile));
        }
    }

    m_isScanning.storeRelaxed(0);
    return results;
}

} // namespace PhotoCarta
