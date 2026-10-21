// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogitem.h"
#include "catalog/catalogmodel.h"
#include "catalog/database/catalogdatabase.h"
#include "catalog/fileops/trashservice.h"
#include "catalog/selectionmanager.h"
#include "catalog/sessioncontroller.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

class TestTrash : public QObject
{
    Q_OBJECT

private slots:
    void testEvaluateCandidatesRefusals();
    void testMoveToTrashWithDisposableFiles();
    void testMoveToTrashWithSidecar();
    void testOfflineItemsRefused();
    void testDatabaseRemoveItems();
    void testModelRemoveItems();
    void testControllerTrashDialogFlow();
};

namespace
{
CatalogItem makeItem(const QString &path)
{
    CatalogItem item;
    item.id = QStringLiteral("id-") + QFileInfo(path).fileName();
    item.filePath = path;
    item.fileName = QFileInfo(path).fileName();
    return item;
}

QString writeFile(const QString &path, const char *content)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
    {
        return QString();
    }
    f.write(content);
    f.close();
    return path;
}
} // namespace

void TestTrash::testEvaluateCandidatesRefusals()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString realFile = writeFile(tempDir.filePath(QStringLiteral("real.jpg")), "x");
    QVERIFY(!realFile.isEmpty());

    CatalogItem demo;
    demo.id = QStringLiteral("demo");
    demo.filePath = QStringLiteral("qrc:/fixtures/demo.jpg");
    demo.fileName = QStringLiteral("demo.jpg");
    demo.isDemoFixture = true;

    CatalogItem offline;
    offline.id = QStringLiteral("offline");
    offline.filePath = QStringLiteral("/mnt/network/photo.jpg");
    offline.fileName = QStringLiteral("photo.jpg");
    offline.isMissing = true;

    CatalogItem missing;
    missing.id = QStringLiteral("missing");
    missing.filePath = tempDir.filePath(QStringLiteral("does-not-exist.jpg"));
    missing.fileName = QStringLiteral("does-not-exist.jpg");

    CatalogItem ok;
    ok.id = QStringLiteral("ok");
    ok.filePath = realFile;
    ok.fileName = QStringLiteral("real.jpg");

    const QList<TrashCandidate> candidates =
        TrashService::evaluateCandidates({demo, offline, missing, ok});

    QCOMPARE(candidates.size(), 4);
    QCOMPARE(candidates[0].status, TrashStatus::DemoFixture);
    QCOMPARE(candidates[1].status, TrashStatus::CollectionOffline);
    QCOMPARE(candidates[2].status, TrashStatus::SourceFileNotFound);
    QCOMPARE(candidates[3].status, TrashStatus::Ok);
    QVERIFY(!candidates[0].canTrash());
    QVERIFY(candidates[3].canTrash());
}

void TestTrash::testMoveToTrashWithDisposableFiles()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString fileA = writeFile(tempDir.filePath(QStringLiteral("a.jpg")), "A");
    const QString fileB = writeFile(tempDir.filePath(QStringLiteral("b.jpg")), "B");
    const QString fileC = writeFile(tempDir.filePath(QStringLiteral("c-missing-refusal.txt")), "C");
    QVERIFY(!fileA.isEmpty() && !fileB.isEmpty() && !fileC.isEmpty());

    // Simulate one file being on an offline collection by deleting it from disk first
    QVERIFY(QFile::remove(fileC));

    const TrashResult result = TrashService::moveToTrash(
        {makeItem(fileA), makeItem(fileB), makeItem(fileC)});

    QCOMPARE(result.totalCount, 3);
    QCOMPARE(result.successCount, 2);
    QCOMPARE(result.failCount, 1);
    QCOMPARE(result.errorMessages.size(), 1);
    QVERIFY(!QFileInfo::exists(fileA));
    QVERIFY(!QFileInfo::exists(fileB));
}

void TestTrash::testMoveToTrashWithSidecar()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString image = writeFile(tempDir.filePath(QStringLiteral("photo.jpg")), "IMG");
    const QString sidecar = writeFile(tempDir.filePath(QStringLiteral("photo.jpg.xmp")), "XMP");
    QVERIFY(!image.isEmpty() && !sidecar.isEmpty());

    const TrashResult result = TrashService::moveToTrash({makeItem(image)});

    QCOMPARE(result.successCount, 1);
    QCOMPARE(result.failCount, 0);
    QVERIFY(!QFileInfo::exists(image));
    QVERIFY(!QFileInfo::exists(sidecar));
}

void TestTrash::testOfflineItemsRefused()
{
    // Offline collections must never be treated as deleted: the file record is preserved
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    CatalogItem offline;
    offline.id = QStringLiteral("offline");
    offline.filePath = tempDir.filePath(QStringLiteral("unavailable.jpg"));
    offline.fileName = QStringLiteral("unavailable.jpg");
    offline.isMissing = true;
    writeFile(offline.filePath, "data");

    const QList<TrashCandidate> candidates = TrashService::evaluateCandidates({offline});
    QCOMPARE(candidates.size(), 1);
    QCOMPARE(candidates[0].status, TrashStatus::CollectionOffline);

    // Even if the file record says missing, executing the batch must refuse it and leave it alone
    const TrashResult result = TrashService::moveToTrash({offline});
    QCOMPARE(result.successCount, 0);
    QCOMPARE(result.failCount, 1);
}

void TestTrash::testDatabaseRemoveItems()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    CatalogDatabase db(QStringLiteral("test_trash_db_conn"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const int collId = db.addOrGetCollection(tempDir.path(), QStringLiteral("Test Collection"));
    QVERIFY(collId > 0);

    const QString fileA = writeFile(tempDir.filePath(QStringLiteral("dbA.jpg")), "A");
    const QString fileB = writeFile(tempDir.filePath(QStringLiteral("dbB.jpg")), "B");
    QVERIFY(!fileA.isEmpty() && !fileB.isEmpty());

    QList<CatalogItem> items;
    items.append(makeItem(fileA));
    items.append(makeItem(fileB));
    items[0].rating = 3;
    items[1].tags = QStringList() << QStringLiteral("trip");
    QVERIFY(db.insertOrUpdateItems(items, collId));
    QCOMPARE(db.itemCount(), 2);

    QSignalSpy itemsSpy(&db, &CatalogDatabase::itemsChanged);
    QSignalSpy collectionsSpy(&db, &CatalogDatabase::collectionsChanged);

    QVERIFY(db.removeItems({fileA, fileB}));
    QCOMPARE(db.itemCount(), 0);
    QCOMPARE(db.fetchAllItems().size(), 0);
    QCOMPARE(itemsSpy.count(), 1);
    QCOMPARE(collectionsSpy.count(), 1);

    // Removing nothing / empty list is a no-op failure
    QVERIFY(!db.removeItems({}));
    db.close();
}

void TestTrash::testModelRemoveItems()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    CatalogDatabase db(QStringLiteral("test_trash_model_conn"));
    QVERIFY(db.open(QStringLiteral(":memory:")));
    const int collId = db.addOrGetCollection(tempDir.path(), QStringLiteral("Model Collection"));
    QVERIFY(collId > 0);

    const QString fileA = writeFile(tempDir.filePath(QStringLiteral("mA.jpg")), "A");
    const QString fileB = writeFile(tempDir.filePath(QStringLiteral("mB.jpg")), "B");
    const QString fileC = writeFile(tempDir.filePath(QStringLiteral("mC.jpg")), "C");
    QVERIFY(!fileA.isEmpty() && !fileB.isEmpty() && !fileC.isEmpty());

    QList<CatalogItem> items;
    items.append(makeItem(fileA));
    items.append(makeItem(fileB));
    items.append(makeItem(fileC));
    QVERIFY(db.insertOrUpdateItems(items, collId));

    CatalogModel model;
    model.setDatabase(&db);
    model.loadFromDatabase();
    QCOMPARE(model.count(), 3);

    // The catalog decides row order (date/name sorting), so capture the loaded
    // order instead of assuming insertion order.
    const QString pathAt0 = model.itemAt(0)->filePath;
    const QString pathAt1 = model.itemAt(1)->filePath;
    const QString pathAt2 = model.itemAt(2)->filePath;

    // Remove rows 0 and 2 (unsorted order to exercise index handling)
    QVERIFY(model.removeItems({2, 0}));
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.itemAt(0)->filePath, pathAt1);
    QCOMPARE(db.itemCount(), 1);
    QCOMPARE(db.fetchItemByPath(pathAt0).filePath, QString());
    QCOMPARE(db.fetchItemByPath(pathAt1).filePath, pathAt1);
    QCOMPARE(db.fetchItemByPath(pathAt2).filePath, QString());

    // Out-of-range indices are ignored
    QVERIFY(!model.removeItems({99}));
    QVERIFY(!model.removeItems({}));
    db.close();
}

void TestTrash::testControllerTrashDialogFlow()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    CatalogDatabase db(QStringLiteral("test_trash_ctrl_conn"));
    QVERIFY(db.open(QStringLiteral(":memory:")));
    const int collId = db.addOrGetCollection(tempDir.path(), QStringLiteral("Controller Collection"));
    QVERIFY(collId > 0);

    const QString fileA = writeFile(tempDir.filePath(QStringLiteral("cA.jpg")), "A");
    const QString fileB = writeFile(tempDir.filePath(QStringLiteral("cB.jpg")), "B");
    QVERIFY(!fileA.isEmpty() && !fileB.isEmpty());

    QList<CatalogItem> items;
    items.append(makeItem(fileA));
    items.append(makeItem(fileB));
    QVERIFY(db.insertOrUpdateItems(items, collId));

    CatalogModel model;
    model.setDatabase(&db);
    model.loadFromDatabase();

    CatalogFilterModel filterModel;
    filterModel.setSourceModel(&model);

    SelectionManager selection(&filterModel);
    selection.selectAll();

    SessionController controller(&model, &filterModel, &selection);

    // Dialog opens with all selected items as candidates
    controller.openTrashDialog();
    QVERIFY(controller.trashDialogOpen());
    QCOMPARE(controller.trashCandidateCount(), 2);
    QCOMPARE(controller.trashSkippedCount(), 0);

    // Confirming moves both files to trash and removes them from the model/database
    controller.confirmTrashSelected();
    QVERIFY(!controller.trashDialogOpen());
    QCOMPARE(model.count(), 0);
    QCOMPARE(db.itemCount(), 0);
    QVERIFY(!QFileInfo::exists(fileA));
    QVERIFY(!QFileInfo::exists(fileB));

    // Offline / missing selection is refused end to end
    const QString fileC = writeFile(tempDir.filePath(QStringLiteral("cC.jpg")), "C");
    QVERIFY(!fileC.isEmpty());
    QVERIFY(QFile::remove(fileC));

    QList<CatalogItem> single;
    single.append(makeItem(fileC));
    QVERIFY(db.insertOrUpdateItems(single, collId));
    model.loadFromDatabase();
    QCOMPARE(model.count(), 1);
    selection.clearSelection();
    selection.select(0);

    controller.openTrashDialog();
    QVERIFY(controller.trashDialogOpen());
    QCOMPARE(controller.trashCandidateCount(), 0);
    QCOMPARE(controller.trashSkippedCount(), 1);

    controller.confirmTrashSelected();
    QVERIFY(!controller.trashDialogOpen());
    // Refused item remains in the catalog
    QCOMPARE(model.count(), 1);
    QCOMPARE(db.itemCount(), 1);
    db.close();
}

QTEST_MAIN(TestTrash)
#include "test_trash.moc"
