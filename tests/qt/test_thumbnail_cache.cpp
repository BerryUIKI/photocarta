// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/thumbnails/thumbnailcache.h"

#include <QColor>
#include <QDir>
#include <QImage>
#include <QList>
#include <QRunnable>
#include <QTemporaryDir>
#include <QTest>
#include <QThreadPool>

#include <vector>

using namespace PhotoCarta;

class TestThumbnailCache : public QObject
{
    Q_OBJECT

private slots:
    void testThumbnailGenerationAndCaching();
    void testClearCaches();
    void testConcurrentGeneration();
    void testConcurrentDuplicateRequest();
};

void TestThumbnailCache::testThumbnailGenerationAndCaching()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Create a high-res source test image (800x600)
    const QString srcPath = tempDir.filePath(QStringLiteral("highres.jpg"));
    QImage srcImg(800, 600, QImage::Format_RGB32);
    srcImg.fill(Qt::yellow);
    QVERIFY(srcImg.save(srcPath, "JPG"));

    ThumbnailCache cache;
    const QString cacheDir = tempDir.filePath(QStringLiteral("cache"));
    cache.setDiskCachePath(cacheDir);

    QVERIFY(!cache.hasCachedThumbnail(srcPath, QSize(200, 200)));

    // Request 200x200 thumbnail
    const QImage thumb = cache.getOrCreateThumbnail(srcPath, QSize(200, 200));
    QVERIFY(!thumb.isNull());
    QVERIFY(thumb.width() <= 200);
    QVERIFY(thumb.height() <= 200);
    // Aspect ratio 4:3 preserved (200x150)
    QCOMPARE(thumb.width(), 200);
    QCOMPARE(thumb.height(), 150);

    // Cache now contains the item in memory and disk
    QVERIFY(cache.hasCachedThumbnail(srcPath, QSize(200, 200)));
    QVERIFY(cache.memoryCacheCost() > 0);

    // Evict memory cache; disk cache should still provide it
    cache.clearMemoryCache();
    QCOMPARE(cache.memoryCacheCost(), 0);
    QVERIFY(cache.hasCachedThumbnail(srcPath, QSize(200, 200)));

    const QImage fromDisk = cache.getOrCreateThumbnail(srcPath, QSize(200, 200));
    QVERIFY(!fromDisk.isNull());
    QCOMPARE(fromDisk.size(), QSize(200, 150));
    QVERIFY(cache.memoryCacheCost() > 0);
}

void TestThumbnailCache::testClearCaches()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString srcPath = tempDir.filePath(QStringLiteral("photo.png"));
    QImage srcImg(100, 100, QImage::Format_ARGB32);
    srcImg.fill(Qt::cyan);
    QVERIFY(srcImg.save(srcPath));

    ThumbnailCache cache;
    cache.setDiskCachePath(tempDir.filePath(QStringLiteral("thumb_dir")));

    const QImage thumb = cache.getOrCreateThumbnail(srcPath, QSize(64, 64));
    QVERIFY(!thumb.isNull());
    QVERIFY(cache.hasCachedThumbnail(srcPath, QSize(64, 64)));

    cache.clearDiskCache();
    QCOMPARE(cache.memoryCacheCost(), 0);
    QVERIFY(!cache.hasCachedThumbnail(srcPath, QSize(64, 64)));
}

void TestThumbnailCache::testConcurrentGeneration()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    constexpr int imageCount = 8;
    QList<QString> paths;
    paths.reserve(imageCount);
    for (int i = 0; i < imageCount; ++i)
    {
        QImage source(1200, 800, QImage::Format_RGB32);
        source.fill(QColor::fromHsv((i * 37) % 360, 180, 220));
        const QString path = tempDir.filePath(QStringLiteral("concurrent_%1.jpg").arg(i));
        QVERIFY(source.save(path, "JPG", 90));
        paths.append(path);
    }

    ThumbnailCache cache;
    cache.setDiskCachePath(tempDir.filePath(QStringLiteral("cache")));

    std::vector<QImage> results(imageCount);
    QThreadPool pool;
    pool.setMaxThreadCount(4);
    for (int i = 0; i < imageCount; ++i)
    {
        pool.start(QRunnable::create([&cache, &results, i, path = paths.at(i)]() {
            results[i] = cache.getOrCreateThumbnail(path, QSize(200, 200));
        }));
    }
    QVERIFY(pool.waitForDone(30000));

    for (const QImage &thumb : results)
    {
        QVERIFY(!thumb.isNull());
        QVERIFY(thumb.width() <= 200);
        QVERIFY(thumb.height() <= 200);
    }
    QVERIFY(cache.memoryCacheCost() > 0);

    cache.clearMemoryCache();
    QCOMPARE(cache.memoryCacheCost(), 0);
    const QImage fromDisk = cache.getOrCreateThumbnail(paths.first(), QSize(200, 200));
    QVERIFY(!fromDisk.isNull());
    QVERIFY(fromDisk.width() <= 200);
}

void TestThumbnailCache::testConcurrentDuplicateRequest()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString srcPath = tempDir.filePath(QStringLiteral("shared.jpg"));
    QImage source(1600, 1200, QImage::Format_RGB32);
    source.fill(Qt::magenta);
    QVERIFY(source.save(srcPath, "JPG", 90));

    ThumbnailCache cache;
    cache.setDiskCachePath(tempDir.filePath(QStringLiteral("cache")));

    constexpr int requestCount = 8;
    std::vector<QImage> results(requestCount);
    QThreadPool pool;
    pool.setMaxThreadCount(4);
    for (int i = 0; i < requestCount; ++i)
    {
        pool.start(QRunnable::create([&cache, &results, i, srcPath]() {
            results[i] = cache.getOrCreateThumbnail(srcPath, QSize(96, 96));
        }));
    }
    QVERIFY(pool.waitForDone(30000));

    for (const QImage &thumb : results)
    {
        QVERIFY(!thumb.isNull());
        QCOMPARE(thumb.size(), QSize(96, 72));
    }
}

QTEST_MAIN(TestThumbnailCache)
#include "test_thumbnail_cache.moc"
