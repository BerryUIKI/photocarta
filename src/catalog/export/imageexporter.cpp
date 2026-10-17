// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imageexporter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMetaObject>
#include <QUrl>
#include <QtConcurrent>

namespace PhotoCarta
{

ImageExporter::ImageExporter(QObject *parent)
    : QObject(parent)
    , m_cancelFlag(std::make_shared<std::atomic<bool>>(false))
    , m_watcher(new QFutureWatcher<ExportResult>(this))
{
    connect(m_watcher, &QFutureWatcher<ExportResult>::finished, this, [this]() {
        const ExportResult result = m_watcher->result();
        m_isExporting.store(false);
        emit exportStateChanged();

        if (m_cancelFlag && m_cancelFlag->load())
        {
            emit exportCanceled();
        }
        else
        {
            emit exportFinished(result);
        }
    });
}

ImageExporter::~ImageExporter()
{
    cancel();
    if (m_watcher->isRunning())
    {
        m_watcher->waitForFinished();
    }
}

bool ImageExporter::isExporting() const
{
    return m_isExporting.load();
}

int ImageExporter::progress() const
{
    return m_completed.load();
}

int ImageExporter::total() const
{
    return m_total;
}

QString ImageExporter::generateOutputFilename(const QString &originalFileName,
                                             const ExportSettings &settings,
                                             const QString &targetDirectory)
{
    QFileInfo fi(originalFileName);
    QString baseName = fi.completeBaseName();
    if (baseName.isEmpty())
    {
        baseName = QStringLiteral("photo");
    }

    QString ext;
    const QString fmt = settings.format.toLower();
    if (fmt == QStringLiteral("jpg") || fmt == QStringLiteral("jpeg"))
    {
        ext = QStringLiteral("jpg");
    }
    else if (fmt == QStringLiteral("png"))
    {
        ext = QStringLiteral("png");
    }
    else if (fmt == QStringLiteral("webp"))
    {
        ext = QStringLiteral("webp");
    }
    else
    {
        ext = fi.suffix().toLower();
        if (ext.isEmpty())
        {
            ext = QStringLiteral("jpg");
        }
    }

    const QString candidateBase = settings.filenamePrefix + baseName + settings.filenameSuffix;

    if (targetDirectory.isEmpty() || settings.overwriteExisting)
    {
        return candidateBase + QStringLiteral(".") + ext;
    }

    QDir dir(targetDirectory);
    QString finalName = candidateBase + QStringLiteral(".") + ext;
    if (!dir.exists(finalName))
    {
        return finalName;
    }

    int counter = 1;
    while (dir.exists(QStringLiteral("%1 (%2).%3").arg(candidateBase).arg(counter).arg(ext)))
    {
        counter++;
    }
    return QStringLiteral("%1 (%2).%3").arg(candidateBase).arg(counter).arg(ext);
}

bool ImageExporter::exportSingleItem(const CatalogItem &item,
                                     const ExportSettings &settings,
                                     QString *outExportedPath,
                                     QString *outError)
{
    if (settings.outputDirectory.isEmpty())
    {
        if (outError)
        {
            *outError = QStringLiteral("Output directory is not specified");
        }
        return false;
    }

    QDir targetDir(settings.outputDirectory);
    if (!targetDir.exists())
    {
        if (!targetDir.mkpath(settings.outputDirectory))
        {
            if (outError)
            {
                *outError = QStringLiteral("Could not create target directory: ") + settings.outputDirectory;
            }
            return false;
        }
    }

    const QString originalName = item.fileName.isEmpty()
        ? QFileInfo(item.filePath).fileName()
        : item.fileName;

    const QString targetFileName = generateOutputFilename(originalName, settings, settings.outputDirectory);
    const QString targetPath = targetDir.filePath(targetFileName);

    QString srcPath = item.filePath;
    if (srcPath.startsWith(QStringLiteral("file:///")))
    {
        srcPath = QUrl(srcPath).toLocalFile();
    }

    const QString fmt = settings.format.toLower();
    const bool isOriginalFormat = (fmt == QStringLiteral("original"));
    const bool noResize = (settings.maxDimension <= 0);

    // Fast-path: lossless copy if format is original, no resize requested, and source is an existing local disk file
    if (isOriginalFormat && noResize && !srcPath.startsWith(QStringLiteral("qrc:")))
    {
        if (QFile::exists(srcPath))
        {
            if (settings.overwriteExisting && QFile::exists(targetPath))
            {
                QFile::remove(targetPath);
            }

            if (QFile::copy(srcPath, targetPath))
            {
                if (outExportedPath)
                {
                    *outExportedPath = targetPath;
                }
                return true;
            }
        }
    }

    // Decode and process image
    QImage img;
    if (!img.load(srcPath))
    {
        if (outError)
        {
            *outError = QStringLiteral("Failed to decode image from ") + srcPath;
        }
        return false;
    }

    // Apply dimension scaling if requested
    if (settings.maxDimension > 0)
    {
        if (img.width() > settings.maxDimension || img.height() > settings.maxDimension)
        {
            img = img.scaled(settings.maxDimension, settings.maxDimension, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    }

    // Determine target format code
    const char *saveFormat = nullptr;
    if (fmt == QStringLiteral("jpg") || fmt == QStringLiteral("jpeg"))
    {
        saveFormat = "JPG";
    }
    else if (fmt == QStringLiteral("png"))
    {
        saveFormat = "PNG";
    }
    else if (fmt == QStringLiteral("webp"))
    {
        saveFormat = "WEBP";
    }
    else
    {
        const QString s = QFileInfo(targetPath).suffix().toUpper();
        if (s == QStringLiteral("JPG") || s == QStringLiteral("JPEG"))
        {
            saveFormat = "JPG";
        }
        else if (s == QStringLiteral("PNG"))
        {
            saveFormat = "PNG";
        }
        else if (s == QStringLiteral("WEBP"))
        {
            saveFormat = "WEBP";
        }
    }

    const int quality = qBound(1, settings.quality, 100);

    if (settings.overwriteExisting && QFile::exists(targetPath))
    {
        QFile::remove(targetPath);
    }

    if (!img.save(targetPath, saveFormat, quality))
    {
        if (outError)
        {
            *outError = QStringLiteral("Failed to save image to ") + targetPath;
        }
        return false;
    }

    if (outExportedPath)
    {
        *outExportedPath = targetPath;
    }
    return true;
}

bool ImageExporter::startExport(const QList<CatalogItem> &items, const ExportSettings &settings)
{
    if (m_isExporting.load())
    {
        return false;
    }

    if (items.isEmpty())
    {
        return false;
    }

    m_isExporting.store(true);
    m_cancelFlag = std::make_shared<std::atomic<bool>>(false);
    m_total = static_cast<int>(items.size());
    m_completed.store(0);

    emit exportStarted(m_total);
    emit exportStateChanged();

    auto cancelFlag = m_cancelFlag;

    auto future = QtConcurrent::run([this, items, settings, cancelFlag]() -> ExportResult {
        ExportResult result;
        result.totalCount = static_cast<int>(items.size());

        for (int i = 0; i < items.size(); ++i)
        {
            if (cancelFlag->load())
            {
                break;
            }

            const CatalogItem &item = items.at(i);
            QString exportedPath;
            QString errorMsg;

            const bool ok = exportSingleItem(item, settings, &exportedPath, &errorMsg);
            const int completed = m_completed.fetch_add(1) + 1;

            if (ok)
            {
                result.successCount++;
                result.exportedFiles.append(exportedPath);
                QMetaObject::invokeMethod(this, [this, i, src = item.filePath, exportedPath]() {
                    emit itemExported(i, src, exportedPath);
                }, Qt::QueuedConnection);
            }
            else
            {
                result.failCount++;
                result.failedFiles.append(item.filePath);
                result.errorMessages.append(errorMsg);
                QMetaObject::invokeMethod(this, [this, i, src = item.filePath, errorMsg]() {
                    emit itemFailed(i, src, errorMsg);
                }, Qt::QueuedConnection);
            }

            QMetaObject::invokeMethod(this, [this, completed, tot = result.totalCount]() {
                emit progressChanged(completed, tot);
            }, Qt::QueuedConnection);
        }

        return result;
    });

    m_watcher->setFuture(future);
    return true;
}

void ImageExporter::cancel()
{
    if (m_cancelFlag)
    {
        m_cancelFlag->store(true);
    }
}

} // namespace PhotoCarta
