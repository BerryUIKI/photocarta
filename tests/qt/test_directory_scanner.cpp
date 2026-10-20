// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/ingestion/directoryscanner.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

class TestDirectoryScanner : public QObject
{
    Q_OBJECT

private slots:
    void testSupportedExtensions();
    void testInspectFile();
    void testSynchronousScan();
    void testAsyncScanAndSignals();
    void testScanCancellation();
};

void TestDirectoryScanner::testSupportedExtensions()
{
    const auto &exts = DirectoryScanner::supportedExtensions();
    QVERIFY(exts.contains(QStringLiteral("jpg")));
    QVERIFY(exts.contains(QStringLiteral("jpeg")));
    QVERIFY(exts.contains(QStringLiteral("png")));
    QVERIFY(exts.contains(QStringLiteral("webp")));
    QVERIFY(exts.contains(QStringLiteral("cr2")));
    QVERIFY(exts.contains(QStringLiteral("nef")));
    QVERIFY(exts.contains(QStringLiteral("arw")));

    QVERIFY(DirectoryScanner::isSupportedImageFile(QStringLiteral("test.JPG")));
    QVERIFY(DirectoryScanner::isSupportedImageFile(QStringLiteral("photo.png")));
    QVERIFY(DirectoryScanner::isSupportedImageFile(QStringLiteral("/path/to/RAW.dng")));
    QVERIFY(!DirectoryScanner::isSupportedImageFile(QStringLiteral("doc.txt")));
    QVERIFY(!DirectoryScanner::isSupportedImageFile(QStringLiteral("archive.zip")));
}

void TestDirectoryScanner::testInspectFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString testPath = tempDir.filePath(QStringLiteral("sample.png"));
    QImage img(200, 150, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::blue);
    QVERIFY(img.save(testPath));

    const CatalogItem item = DirectoryScanner::inspectFile(testPath);
    QCOMPARE(item.filePath, QFileInfo(testPath).absoluteFilePath());
    QCOMPARE(item.fileName, QStringLiteral("sample.png"));
    QCOMPARE(item.title, QStringLiteral("sample"));
    QCOMPARE(item.dimensions, QSize(200, 150));
    QVERIFY(item.fileSizeBytes > 0);
    QVERIFY(!item.formattedFileSize.isEmpty());
    QCOMPARE(item.mimeType, QStringLiteral("image/png"));
    QVERIFY(item.thumbnailUrl.isValid());
    QVERIFY(item.previewUrl.isValid());
    QCOMPARE(item.isDemoFixture, false);
}

void TestDirectoryScanner::testSynchronousScan()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Create 3 image files and 1 text file
    for (int i = 1; i <= 3; ++i)
    {
        const QString imgPath = tempDir.filePath(QString::asprintf("img_%d.jpg", i));
        QImage img(100, 100, QImage::Format_RGB32);
        img.fill(Qt::red);
        QVERIFY(img.save(imgPath));
    }

    QFile txtFile(tempDir.filePath(QStringLiteral("notes.txt")));
    QVERIFY(txtFile.open(QIODevice::WriteOnly));
    txtFile.write("Not an image");
    txtFile.close();

    DirectoryScanner scanner;
    const QList<CatalogItem> items = scanner.scanSynchronously(tempDir.path(), false);
    QCOMPARE(items.size(), 3);
}

void TestDirectoryScanner::testAsyncScanAndSignals()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    for (int i = 1; i <= 5; ++i)
    {
        const QString imgPath = tempDir.filePath(QString::asprintf("test_%d.png", i));
        QImage img(50, 50, QImage::Format_ARGB32);
        img.fill(Qt::green);
        QVERIFY(img.save(imgPath));
    }

    DirectoryScanner scanner;
    QSignalSpy startedSpy(&scanner, &DirectoryScanner::scanStarted);
    QSignalSpy batchSpy(&scanner, &DirectoryScanner::batchDiscovered);
    QSignalSpy progressSpy(&scanner, &DirectoryScanner::progress);
    QSignalSpy finishedSpy(&scanner, &DirectoryScanner::scanFinished);

    scanner.startScan(tempDir.path(), false);

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(startedSpy.count(), 1);
    QVERIFY(batchSpy.count() >= 1);
    QCOMPARE(progressSpy.count(), 5);
    QCOMPARE(finishedSpy.count(), 1);

    const int totalFound = finishedSpy.first().at(0).toInt();
    QCOMPARE(totalFound, 5);
}

void TestDirectoryScanner::testScanCancellation()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    DirectoryScanner scanner;
    QSignalSpy canceledSpy(&scanner, &DirectoryScanner::scanCanceled);

    scanner.startScan(tempDir.path(), true);
    scanner.cancel();

    // Cancellation will trigger scanCanceled
    if (canceledSpy.isEmpty())
    {
        canceledSpy.wait(1000);
    }
    QVERIFY(!scanner.isScanning());
}

QTEST_MAIN(TestDirectoryScanner)
#include "test_directory_scanner.moc"
