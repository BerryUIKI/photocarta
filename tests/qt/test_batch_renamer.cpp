// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogitem.h"
#include "catalog/catalogmodel.h"
#include "catalog/database/catalogdatabase.h"
#include "catalog/metadata/xmpsidecar.h"
#include "catalog/organization/batchrenamer.h"
#include "catalog/selectionmanager.h"
#include "catalog/sessioncontroller.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

class TestBatchRenamer : public QObject
{
    Q_OBJECT

private slots:
    void testTokenFormatting();
    void testCaseConversion();
    void testFindAndReplace();
    void testCollisionDetection();
    void testDemoFixtureRejection();
    void testExecutionWithSidecars();
    void testCatalogModelAndDatabaseIntegration();
    void testPartialFailureHandling();
};

void TestBatchRenamer::testTokenFormatting()
{
    CatalogItem item;
    item.id = QStringLiteral("item-001");
    item.filePath = QStringLiteral("/photos/vacation/DSC_0123.JPG");
    item.fileName = QStringLiteral("DSC_0123.JPG");
    item.title = QStringLiteral("Sunset Beach");
    item.dateTaken = QDateTime::fromString(QStringLiteral("2026-07-15 14:30:45"), QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    item.cameraModel = QStringLiteral("Sony A7R V");
    item.lensModel = QStringLiteral("FE 24-70mm F2.8 GM II");
    item.rating = 5;
    item.colorLabel = 1; // Red

    // 1. Date and sequence formatting
    RenameRule rule1;
    rule1.pattern = QStringLiteral("{YYYY}_{MM}_{DD}_{seq:3}");
    QString res1 = BatchRenamer::formatFileName(item, 7, rule1);
    QCOMPARE(res1, QStringLiteral("2026_07_15_007.JPG"));

    // 2. Original name, camera, and rating tokens
    RenameRule rule2;
    rule2.pattern = QStringLiteral("{camera}_{original}_R{rating}");
    QString res2 = BatchRenamer::formatFileName(item, 1, rule2);
    QCOMPARE(res2, QStringLiteral("Sony A7R V_DSC_0123_R5.JPG"));

    // 3. Compact date and time tokens
    RenameRule rule3;
    rule3.pattern = QStringLiteral("{date}_{time}_{label}");
    QString res3 = BatchRenamer::formatFileName(item, 1, rule3);
    QCOMPARE(res3, QStringLiteral("20260715_143045_Red.JPG"));
}

void TestBatchRenamer::testCaseConversion()
{
    CatalogItem item;
    item.filePath = QStringLiteral("/photos/Photo_Test_FILE.JPEG");
    item.dateTaken = QDateTime::fromString(QStringLiteral("2026-01-01 10:00:00"), QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    RenameRule rule;
    rule.pattern = QStringLiteral("{original}");

    rule.caseConversion = CaseConversion::Lowercase;
    QCOMPARE(BatchRenamer::formatFileName(item, 1, rule), QStringLiteral("photo_test_file.JPEG"));

    rule.caseConversion = CaseConversion::Uppercase;
    QCOMPARE(BatchRenamer::formatFileName(item, 1, rule), QStringLiteral("PHOTO_TEST_FILE.JPEG"));

    rule.caseConversion = CaseConversion::TitleCase;
    QCOMPARE(BatchRenamer::formatFileName(item, 1, rule), QStringLiteral("Photo_Test_File.JPEG"));
}

void TestBatchRenamer::testFindAndReplace()
{
    CatalogItem item;
    item.filePath = QStringLiteral("/photos/DSC_0100_RAW.CR3");
    item.dateTaken = QDateTime::fromString(QStringLiteral("2026-02-02 12:00:00"), QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    RenameRule rule;
    rule.pattern = QStringLiteral("{original}");
    rule.findText = QStringLiteral("DSC_");
    rule.replaceText = QStringLiteral("Trip2026_");
    rule.matchCase = true;

    QString res = BatchRenamer::formatFileName(item, 1, rule);
    QCOMPARE(res, QStringLiteral("Trip2026_0100_RAW.CR3"));
}

void TestBatchRenamer::testCollisionDetection()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Create 2 real files on disk
    const QString fileA = tempDir.filePath(QStringLiteral("photoA.jpg"));
    const QString fileB = tempDir.filePath(QStringLiteral("photoB.jpg"));
    const QString existingTarget = tempDir.filePath(QStringLiteral("existing.jpg"));

    QFile fa(fileA);
    QVERIFY(fa.open(QIODevice::WriteOnly));
    fa.write("A");
    fa.close();

    QFile fb(fileB);
    QVERIFY(fb.open(QIODevice::WriteOnly));
    fb.write("B");
    fb.close();

    QFile fe(existingTarget);
    QVERIFY(fe.open(QIODevice::WriteOnly));
    fe.write("E");
    fe.close();

    CatalogItem itemA;
    itemA.id = QStringLiteral("a");
    itemA.filePath = fileA;
    itemA.fileName = QStringLiteral("photoA.jpg");

    CatalogItem itemB;
    itemB.id = QStringLiteral("b");
    itemB.filePath = fileB;
    itemB.fileName = QStringLiteral("photoB.jpg");

    // Case 1: Collision with existing file on disk
    RenameRule ruleConflictDisk;
    ruleConflictDisk.pattern = QStringLiteral("existing");
    QList<RenameCandidate> candDisk = BatchRenamer::generatePreview({itemA}, ruleConflictDisk);
    QCOMPARE(candDisk.size(), 1);
    QCOMPARE(candDisk.first().status, RenameStatus::ConflictFileExists);
    QVERIFY(!candDisk.first().canRename());

    // Case 2: Duplicate target generated within the same batch
    RenameRule ruleDuplicate;
    ruleDuplicate.pattern = QStringLiteral("same_name"); // Both items would produce "same_name.jpg"
    QList<RenameCandidate> candBatch = BatchRenamer::generatePreview({itemA, itemB}, ruleDuplicate);
    QCOMPARE(candBatch.size(), 2);
    QCOMPARE(candBatch[0].status, RenameStatus::Ok);
    QCOMPARE(candBatch[1].status, RenameStatus::ConflictBatchDuplicate);
    QVERIFY(!candBatch[1].canRename());
}

void TestBatchRenamer::testDemoFixtureRejection()
{
    CatalogItem demoItem;
    demoItem.id = QStringLiteral("demo-001");
    demoItem.filePath = QStringLiteral("qrc:/assets/demo/demo_landscape.svg");
    demoItem.fileName = QStringLiteral("demo_landscape.svg");
    demoItem.isDemoFixture = true;

    RenameRule rule;
    rule.pattern = QStringLiteral("new_name_{seq}");

    QList<RenameCandidate> candidates = BatchRenamer::generatePreview({demoItem}, rule);
    QCOMPARE(candidates.size(), 1);
    QCOMPARE(candidates.first().status, RenameStatus::DemoFixture);
    QVERIFY(!candidates.first().canRename());
}

void TestBatchRenamer::testExecutionWithSidecars()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString imgPath = tempDir.filePath(QStringLiteral("IMG_5500.JPG"));
    const QString sidecarPath = tempDir.filePath(QStringLiteral("IMG_5500.JPG.xmp"));

    // Create real image file
    QImage img(100, 100, QImage::Format_RGB32);
    img.fill(Qt::blue);
    QVERIFY(img.save(imgPath, "JPG"));

    // Create companion XMP sidecar file
    XmpMetadata xmp;
    xmp.title = QStringLiteral("Mountain Sunrise");
    xmp.rating = 4;
    xmp.colorLabel = 3; // Green
    xmp.tags = {QStringLiteral("Nature"), QStringLiteral("Mountains")};
    QVERIFY(XmpSidecar::writeToFile(sidecarPath, xmp));
    QVERIFY(QFile::exists(sidecarPath));

    CatalogItem item;
    item.id = QStringLiteral("item-sidecar");
    item.filePath = imgPath;
    item.fileName = QStringLiteral("IMG_5500.JPG");
    item.dateTaken = QDateTime::fromString(QStringLiteral("2026-05-20 06:15:00"), QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    RenameRule rule;
    rule.pattern = QStringLiteral("Sunrise_{seq:3}");
    rule.startSequence = 1;

    // Generate preview
    QList<RenameCandidate> candidates = BatchRenamer::generatePreview({item}, rule);
    QCOMPARE(candidates.size(), 1);
    QVERIFY(candidates.first().canRename());
    QVERIFY(candidates.first().hasSidecar);
    QCOMPARE(candidates.first().proposedFileName, QStringLiteral("Sunrise_001.JPG"));

    // Execute rename
    BatchRenameResult result = BatchRenamer::executeRename(candidates);
    QCOMPARE(result.totalCount, 1);
    QCOMPARE(result.successCount, 1);
    QCOMPARE(result.failCount, 0);

    const QString expectedNewImg = tempDir.filePath(QStringLiteral("Sunrise_001.JPG"));
    const QString expectedNewSidecar = tempDir.filePath(QStringLiteral("Sunrise_001.JPG.xmp"));

    // Old files must no longer exist
    QVERIFY(!QFile::exists(imgPath));
    QVERIFY(!QFile::exists(sidecarPath));

    // New files must exist
    QVERIFY(QFile::exists(expectedNewImg));
    QVERIFY(QFile::exists(expectedNewSidecar));

    // Sidecar metadata must be fully intact
    XmpMetadata reloadedXmp;
    QVERIFY(XmpSidecar::readFromFile(expectedNewSidecar, reloadedXmp));
    QCOMPARE(reloadedXmp.title, QStringLiteral("Mountain Sunrise"));
    QCOMPARE(reloadedXmp.rating, 4);
    QCOMPARE(reloadedXmp.colorLabel, 3);
    QVERIFY(reloadedXmp.tags.contains(QStringLiteral("Nature")));
}

void TestBatchRenamer::testCatalogModelAndDatabaseIntegration()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // 1. Setup SQLite catalog database
    const QString dbPath = tempDir.filePath(QStringLiteral("test_catalog.db"));
    CatalogDatabase database(QStringLiteral("test_batch_rename_connection"));
    QVERIFY(database.open(dbPath));

    // 2. Create real images
    const QString img1 = tempDir.filePath(QStringLiteral("orig_01.jpg"));
    const QString img2 = tempDir.filePath(QStringLiteral("orig_02.jpg"));
    QImage dummy(64, 64, QImage::Format_RGB32);
    dummy.fill(Qt::red);
    dummy.save(img1, "JPG");
    dummy.fill(Qt::green);
    dummy.save(img2, "JPG");

    // 3. Setup models and controller
    CatalogModel model;
    model.setDatabase(&database);

    CatalogFilterModel filterModel;
    filterModel.setSourceModel(&model);

    SelectionManager selectionManager(&filterModel);
    SessionController controller(&model, &filterModel, &selectionManager);
    controller.setDatabase(&database);

    // Add files to model
    model.addFilePaths({img1, img2});
    QCOMPARE(model.count(), 2);

    // Select all photos
    selectionManager.selectAll();
    QCOMPARE(selectionManager.selectedCount(), 2);

    // 4. Configure rename rule and execute via controller
    QVariantMap ruleMap;
    ruleMap.insert(QStringLiteral("pattern"), QStringLiteral("Batch_{seq:2}"));
    ruleMap.insert(QStringLiteral("startSequence"), 1);

    QVariantList preview = controller.previewBatchRename(ruleMap);
    QCOMPARE(preview.size(), 2);
    QCOMPARE(preview[0].toMap().value(QStringLiteral("proposedFileName")).toString(), QStringLiteral("Batch_01.jpg"));
    QCOMPARE(preview[1].toMap().value(QStringLiteral("proposedFileName")).toString(), QStringLiteral("Batch_02.jpg"));

    QVariantMap execResult = controller.executeBatchRename(ruleMap);
    QCOMPARE(execResult.value(QStringLiteral("successCount")).toInt(), 2);
    QCOMPARE(execResult.value(QStringLiteral("failCount")).toInt(), 0);

    // 5. Verify CatalogModel data is updated
    const QString newPath1 = tempDir.filePath(QStringLiteral("Batch_01.jpg"));
    const QString newPath2 = tempDir.filePath(QStringLiteral("Batch_02.jpg"));

    QCOMPARE(model.itemAt(0)->filePath, newPath1);
    QCOMPARE(model.itemAt(0)->fileName, QStringLiteral("Batch_01.jpg"));
    QCOMPARE(model.itemAt(1)->filePath, newPath2);
    QCOMPARE(model.itemAt(1)->fileName, QStringLiteral("Batch_02.jpg"));

    // 6. Verify CatalogDatabase SQLite records reflect new paths
    CatalogItem dbItem1 = database.fetchItemByPath(newPath1);
    QVERIFY(!dbItem1.filePath.isEmpty());
    QCOMPARE(dbItem1.fileName, QStringLiteral("Batch_01.jpg"));

    CatalogItem dbItem2 = database.fetchItemByPath(newPath2);
    QVERIFY(!dbItem2.filePath.isEmpty());
    QCOMPARE(dbItem2.fileName, QStringLiteral("Batch_02.jpg"));

    database.close();
}

void TestBatchRenamer::testPartialFailureHandling()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString validImg = tempDir.filePath(QStringLiteral("valid.jpg"));
    QImage dummy(32, 32, QImage::Format_RGB32);
    dummy.fill(Qt::yellow);
    dummy.save(validImg, "JPG");

    CatalogItem validItem;
    validItem.id = QStringLiteral("valid-1");
    validItem.filePath = validImg;
    validItem.fileName = QStringLiteral("valid.jpg");

    CatalogItem missingItem;
    missingItem.id = QStringLiteral("missing-2");
    missingItem.filePath = tempDir.filePath(QStringLiteral("does_not_exist.jpg"));
    missingItem.fileName = QStringLiteral("does_not_exist.jpg");

    CatalogItem demoItem;
    demoItem.id = QStringLiteral("demo-3");
    demoItem.filePath = QStringLiteral("qrc:/assets/demo/demo_astro.svg");
    demoItem.fileName = QStringLiteral("demo_astro.svg");
    demoItem.isDemoFixture = true;

    RenameRule rule;
    rule.pattern = QStringLiteral("Renamed_{seq}");

    QList<RenameCandidate> candidates = BatchRenamer::generatePreview({validItem, missingItem, demoItem}, rule);
    QCOMPARE(candidates.size(), 3);
    QCOMPARE(candidates[0].status, RenameStatus::Ok);
    QCOMPARE(candidates[1].status, RenameStatus::SourceFileNotFound);
    QCOMPARE(candidates[2].status, RenameStatus::DemoFixture);

    BatchRenameResult result = BatchRenamer::executeRename(candidates);
    QCOMPARE(result.totalCount, 3);
    QCOMPARE(result.successCount, 1);
    QCOMPARE(result.failCount, 2);
    QCOMPARE(result.failedRenames.size(), 2);
    QCOMPARE(result.successfulRenames.size(), 1);
}

QTEST_MAIN(TestBatchRenamer)
#include "test_batch_renamer.moc"
