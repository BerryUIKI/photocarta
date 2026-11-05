// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QCache>
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QSemaphore>
#include <QSize>
#include <QString>

namespace PhotoCarta
{

class ThumbnailCache : public QObject
{
    Q_OBJECT

public:
    explicit ThumbnailCache(QObject *parent = nullptr);
    ~ThumbnailCache() override;

    static ThumbnailCache &instance();

    /// Upper bound on full-image decodes that may run at the same time.
    [[nodiscard]] static int maxConcurrentDecodes();

    [[nodiscard]] QString diskCachePath() const;
    void setDiskCachePath(const QString &path);

    [[nodiscard]] QImage getOrCreateThumbnail(const QString &filePath,
                                              const QSize &targetSize = QSize(256, 256));

    [[nodiscard]] bool hasCachedThumbnail(const QString &filePath,
                                          const QSize &targetSize = QSize(256, 256)) const;

    void clearMemoryCache();
    void clearDiskCache();

    [[nodiscard]] int memoryCacheCost() const;
    [[nodiscard]] int memoryCacheMaxCost() const;
    void setMemoryCacheMaxCost(int maxCostBytes);

private:
    [[nodiscard]] QString cacheKey(const QString &filePath, const QSize &targetSize) const;
    [[nodiscard]] QString diskFilePath(const QString &cacheKey) const;
    [[nodiscard]] QImage generateThumbnail(const QString &filePath, const QSize &targetSize) const;
    void writeThumbnail(const QImage &image, const QString &diskPath) const;

    mutable QMutex m_mutex;
    QSemaphore m_decodeSlots;
    QString m_diskCacheDir;
    QCache<QString, QImage> m_memoryCache;
};

} // namespace PhotoCarta
