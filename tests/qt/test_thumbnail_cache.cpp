// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/thumbnails/thumbnailcache.h"

#include <QDir>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

class TestThumbnailCache : public QObject
{
    Q_OBJECT

private slots:
    void testThumbnailGenerationAndCaching();
    void testClearCaches();
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

QTEST_MAIN(TestThumbnailCache)
#include "test_thumbnail_cache.moc"
