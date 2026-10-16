// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumbnailcache.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QLoggingCategory>
#include <QStandardPaths>

namespace
{
Q_LOGGING_CATEGORY(lcThumb, "photocarta.thumbnails")
constexpr int DEFAULT_MAX_MEMORY_CACHE_BYTES = 64 * 1024 * 1024; // 64 MB
}

namespace PhotoCarta
{

ThumbnailCache::ThumbnailCache(QObject *parent)
    : QObject(parent),
      m_memoryCache(DEFAULT_MAX_MEMORY_CACHE_BYTES)
{
    const QString cacheBase = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    m_diskCacheDir = QDir(cacheBase).filePath(QStringLiteral("thumbnails"));
    QDir().mkpath(m_diskCacheDir);
}

ThumbnailCache::~ThumbnailCache() = default;

ThumbnailCache &ThumbnailCache::instance()
{
    static ThumbnailCache s_instance;
    return s_instance;
}

QString ThumbnailCache::diskCachePath() const
{
    QMutexLocker locker(&m_mutex);
    return m_diskCacheDir;
}

void ThumbnailCache::setDiskCachePath(const QString &path)
{
    QMutexLocker locker(&m_mutex);
    m_diskCacheDir = path;
    QDir().mkpath(m_diskCacheDir);
}

QString ThumbnailCache::cacheKey(const QString &filePath, const QSize &targetSize) const
{
    const QFileInfo fi(filePath);
    const QString keyStr = QStringLiteral("%1:%2:%3x%4")
                               .arg(fi.absoluteFilePath())
                               .arg(fi.lastModified().toSecsSinceEpoch())
                               .arg(targetSize.width())
                               .arg(targetSize.height());
    return QString::fromLatin1(QCryptographicHash::hash(keyStr.toUtf8(), QCryptographicHash::Sha1).toHex());
}

QString ThumbnailCache::diskFilePath(const QString &key) const
{
    return QDir(m_diskCacheDir).filePath(key + QStringLiteral(".jpg"));
}

bool ThumbnailCache::hasCachedThumbnail(const QString &filePath, const QSize &targetSize) const
{
    QMutexLocker locker(&m_mutex);
    const QString key = cacheKey(filePath, targetSize);
    if (m_memoryCache.contains(key))
    {
        return true;
    }
    return QFileInfo::exists(diskFilePath(key));
}

QImage ThumbnailCache::getOrCreateThumbnail(const QString &filePath, const QSize &targetSize)
{
    QMutexLocker locker(&m_mutex);
    const QString key = cacheKey(filePath, targetSize);

    // 1. Check in-memory bounded cache
    if (auto *cached = m_memoryCache.object(key))
    {
        return *cached;
    }

    // 2. Check on-disk cache
    const QString diskPath = diskFilePath(key);
    if (QFileInfo::exists(diskPath))
    {
        QImage diskImage(diskPath);
        if (!diskImage.isNull())
        {
            m_memoryCache.insert(key, new QImage(diskImage), static_cast<int>(diskImage.sizeInBytes()));
            return diskImage;
        }
    }

    // 3. Generate from source image file
    const QImage generated = generateThumbnail(filePath, targetSize);
    if (!generated.isNull())
    {
        // Save to disk asynchronously / synchronously
        generated.save(diskPath, "JPG", 85);
        m_memoryCache.insert(key, new QImage(generated), static_cast<int>(generated.sizeInBytes()));
    }

    return generated;
}

QImage ThumbnailCache::generateThumbnail(const QString &filePath, const QSize &targetSize) const
{
    if (!QFileInfo::exists(filePath))
    {
        return {};
    }

    QImageReader reader(filePath);
    reader.setAutoTransform(true);

    const QSize origSize = reader.size();
    if (origSize.isValid() && targetSize.isValid())
    {
        const QSize scaledSize = origSize.scaled(targetSize, Qt::KeepAspectRatio);
        reader.setScaledSize(scaledSize);
    }

    QImage image = reader.read();
    if (image.isNull())
    {
        qCDebug(lcThumb) << "Failed to read image for thumbnail:" << filePath << reader.errorString();
        return {};
    }

    // Ensure thumbnail matches bounding box if reader setScaledSize wasn't exact
    if (targetSize.isValid() && (image.width() > targetSize.width() || image.height() > targetSize.height()))
    {
        image = image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return image;
}

void ThumbnailCache::clearMemoryCache()
{
    QMutexLocker locker(&m_mutex);
    m_memoryCache.clear();
}

void ThumbnailCache::clearDiskCache()
{
    QMutexLocker locker(&m_mutex);
    m_memoryCache.clear();
    QDir dir(m_diskCacheDir);
    const QStringList files = dir.entryList(QStringList() << QStringLiteral("*.jpg"), QDir::Files);
    for (const QString &file : files)
    {
        dir.remove(file);
    }
}

int ThumbnailCache::memoryCacheCost() const
{
    QMutexLocker locker(&m_mutex);
    return m_memoryCache.totalCost();
}

int ThumbnailCache::memoryCacheMaxCost() const
{
    QMutexLocker locker(&m_mutex);
    return m_memoryCache.maxCost();
}

void ThumbnailCache::setMemoryCacheMaxCost(int maxCostBytes)
{
    QMutexLocker locker(&m_mutex);
    m_memoryCache.setMaxCost(maxCostBytes);
}

} // namespace PhotoCarta
