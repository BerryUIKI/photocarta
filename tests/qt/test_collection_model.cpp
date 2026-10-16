// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/collectionmodel.h"
#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogmodel.h"
#include "catalog/database/catalogdatabase.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

class TestCollectionModel : public QObject
{
    Q_OBJECT

private slots:
    void testEmptyModel();
    void testAddAndReloadCollections();
    void testCollectionItemCounts();
    void testCollectionFilterInCatalogFilterModel();
    void testToggleAvailabilityPreservesCollection();
    void testRemoveCollection();
    void testActiveCollectionSelection();
};

void TestCollectionModel::testEmptyModel()
{
    CollectionModel model;
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.activeCollectionId(), 0);
    QVERIFY(model.get(0).isEmpty());
    QCOMPARE(model.findCollectionIndex(42), -1);
}

void TestCollectionModel::testAddAndReloadCollections()
{
    CatalogDatabase db(QStringLiteral("test_conn_coll_model_1"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    CollectionModel model;
    model.setDatabase(&db);
    QCOMPARE(model.count(), 0);

    const int id1 = db.addOrGetCollection(QStringLiteral("/photos/nature"), QStringLiteral("Nature"), QStringLiteral("local"));
    const int id2 = db.addOrGetCollection(QStringLiteral("/photos/urban"), QStringLiteral("Urban"), QStringLiteral("removable"));
    QVERIFY(id1 > 0);
    QVERIFY(id2 > 0);

    model.reload();
    QCOMPARE(model.count(), 2);

    const int idxNature = model.findCollectionIndex(id1);
    QVERIFY(idxNature >= 0);
    QCOMPARE(model.data(model.index(idxNature, 0), CollectionModel::NameRole).toString(), QStringLiteral("Nature"));
    QCOMPARE(model.data(model.index(idxNature, 0), CollectionModel::RootPathRole).toString(), QStringLiteral("/photos/nature"));
    QCOMPARE(model.data(model.index(idxNature, 0), CollectionModel::CollectionTypeRole).toString(), QStringLiteral("local"));
    QCOMPARE(model.data(model.index(idxNature, 0), CollectionModel::IsAvailableRole).toBool(), true);
    QCOMPARE(model.data(model.index(idxNature, 0), CollectionModel::ItemCountRole).toInt(), 0);

    const int idxUrban = model.findCollectionIndex(id2);
    QVERIFY(idxUrban >= 0);
    QCOMPARE(model.data(model.index(idxUrban, 0), CollectionModel::NameRole).toString(), QStringLiteral("Urban"));
    QCOMPARE(model.data(model.index(idxUrban, 0), CollectionModel::CollectionTypeRole).toString(), QStringLiteral("removable"));
}

void TestCollectionModel::testCollectionItemCounts()
{
    CatalogDatabase db(QStringLiteral("test_conn_coll_model_2"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const int id1 = db.addOrGetCollection(QStringLiteral("/media/album1"), QStringLiteral("Album 1"));
    const int id2 = db.addOrGetCollection(QStringLiteral("/media/album2"), QStringLiteral("Album 2"));

    CatalogItem itemA;
    itemA.filePath = QStringLiteral("/media/album1/photoA.jpg");
    itemA.fileName = QStringLiteral("photoA.jpg");
    itemA.fileSizeBytes = 1000;
    itemA.fileLastModified = QDateTime::currentDateTime();

    CatalogItem itemB;
    itemB.filePath = QStringLiteral("/media/album1/photoB.jpg");
    itemB.fileName = QStringLiteral("photoB.jpg");
    itemB.fileSizeBytes = 2000;
    itemB.fileLastModified = QDateTime::currentDateTime();

    CatalogItem itemC;
    itemC.filePath = QStringLiteral("/media/album2/photoC.jpg");
    itemC.fileName = QStringLiteral("photoC.jpg");
    itemC.fileSizeBytes = 3000;
    itemC.fileLastModified = QDateTime::currentDateTime();

    QVERIFY(db.insertOrUpdateItems({itemA, itemB}, id1));
    QVERIFY(db.insertOrUpdateItems({itemC}, id2));

    CollectionModel model;
    model.setDatabase(&db);

    const int idx1 = model.findCollectionIndex(id1);
    const int idx2 = model.findCollectionIndex(id2);
    QCOMPARE(model.data(model.index(idx1, 0), CollectionModel::ItemCountRole).toInt(), 2);
    QCOMPARE(model.data(model.index(idx2, 0), CollectionModel::ItemCountRole).toInt(), 1);
}

void TestCollectionModel::testCollectionFilterInCatalogFilterModel()
{
    CatalogDatabase db(QStringLiteral("test_conn_coll_model_3"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const int id1 = db.addOrGetCollection(QStringLiteral("/photos/birds"), QStringLiteral("Birds"));
    const int id2 = db.addOrGetCollection(QStringLiteral("/photos/cars"), QStringLiteral("Cars"));

    CatalogItem item1;
    item1.filePath = QStringLiteral("/photos/birds/eagle.jpg");
    item1.fileName = QStringLiteral("eagle.jpg");
    item1.fileSizeBytes = 1024;
    item1.fileLastModified = QDateTime::currentDateTime();

    CatalogItem item2;
    item2.filePath = QStringLiteral("/photos/cars/porsche.jpg");
    item2.fileName = QStringLiteral("porsche.jpg");
    item2.fileSizeBytes = 2048;
    item2.fileLastModified = QDateTime::currentDateTime();

    db.insertOrUpdateItems({item1}, id1);
    db.insertOrUpdateItems({item2}, id2);

    CatalogModel catModel;
    catModel.setDatabase(&db);
    catModel.loadFromDatabase(0);
    QCOMPARE(catModel.count(), 2);

    CatalogFilterModel filterModel;
    filterModel.setSourceModel(&catModel);
    QCOMPARE(filterModel.count(), 2);

    // Filter by collection 1
    filterModel.setCollectionFilter(id1);
    QCOMPARE(filterModel.collectionFilter(), id1);
    QCOMPARE(filterModel.count(), 1);
    QCOMPARE(filterModel.get(0).value(QStringLiteral("fileName")).toString(), QStringLiteral("eagle.jpg"));

    // Filter by collection 2
    filterModel.setCollectionFilter(id2);
    QCOMPARE(filterModel.count(), 1);
    QCOMPARE(filterModel.get(0).value(QStringLiteral("fileName")).toString(), QStringLiteral("porsche.jpg"));

    // Clear collection filter (0 = all collections)
    filterModel.setCollectionFilter(0);
    QCOMPARE(filterModel.count(), 2);
}

void TestCollectionModel::testToggleAvailabilityPreservesCollection()
{
    CatalogDatabase db(QStringLiteral("test_conn_coll_model_4"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const int id = db.addOrGetCollection(QStringLiteral("/mnt/external_drive"), QStringLiteral("External Drive"), QStringLiteral("removable"));

    CatalogItem item;
    item.filePath = QStringLiteral("/mnt/external_drive/shot.jpg");
    item.fileName = QStringLiteral("shot.jpg");
    item.fileSizeBytes = 4096;
    item.fileLastModified = QDateTime::currentDateTime();
    db.insertOrUpdateItems({item}, id);

    CollectionModel model;
    model.setDatabase(&db);
    QCOMPARE(model.count(), 1);

    // Toggle to offline
    QSignalSpy availSpy(&model, &CollectionModel::collectionAvailabilityChanged);
    QVERIFY(model.toggleAvailability(id));
    QCOMPARE(availSpy.count(), 1);
    QCOMPARE(availSpy.first().at(0).toInt(), id);
    QCOMPARE(availSpy.first().at(1).toBool(), false);

    // Crucial check: Collection is preserved, not deleted!
    QCOMPARE(model.count(), 1);
    const int idx = model.findCollectionIndex(id);
    QCOMPARE(model.data(model.index(idx, 0), CollectionModel::IsAvailableRole).toBool(), false);

    // Verify item is preserved in database with isMissing = true
    const CatalogItem fetched = db.fetchItemByPath(item.filePath);
    QCOMPARE(fetched.filePath, item.filePath);
    QVERIFY(fetched.isMissing);

    // Toggle back to online
    QVERIFY(model.toggleAvailability(id));
    QCOMPARE(model.data(model.index(idx, 0), CollectionModel::IsAvailableRole).toBool(), true);
    const CatalogItem onlineFetched = db.fetchItemByPath(item.filePath);
    QVERIFY(!onlineFetched.isMissing);
}

void TestCollectionModel::testRemoveCollection()
{
    CatalogDatabase db(QStringLiteral("test_conn_coll_model_5"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const int id = db.addOrGetCollection(QStringLiteral("/photos/to_delete"), QStringLiteral("Temporary"));

    CatalogItem item;
    item.filePath = QStringLiteral("/photos/to_delete/temp.jpg");
    item.fileName = QStringLiteral("temp.jpg");
    item.fileSizeBytes = 100;
    item.fileLastModified = QDateTime::currentDateTime();
    db.insertOrUpdateItems({item}, id);

    CollectionModel model;
    model.setDatabase(&db);
    QCOMPARE(model.count(), 1);

    QSignalSpy removeSpy(&model, &CollectionModel::collectionRemoved);
    QVERIFY(model.removeCollection(id));
    QCOMPARE(removeSpy.count(), 1);
    QCOMPARE(removeSpy.first().at(0).toInt(), id);
    QCOMPARE(model.count(), 0);

    // Verify items in deleted collection are also removed
    const CatalogItem fetched = db.fetchItemByPath(item.filePath);
    QVERIFY(fetched.filePath.isEmpty());
}

void TestCollectionModel::testActiveCollectionSelection()
{
    CatalogDatabase db(QStringLiteral("test_conn_coll_model_6"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const int id1 = db.addOrGetCollection(QStringLiteral("/photos/one"), QStringLiteral("One"));
    const int id2 = db.addOrGetCollection(QStringLiteral("/photos/two"), QStringLiteral("Two"));

    CollectionModel model;
    model.setDatabase(&db);
    QCOMPARE(model.count(), 2);

    const int idx1 = model.findCollectionIndex(id1);
    const int idx2 = model.findCollectionIndex(id2);

    QCOMPARE(model.data(model.index(idx1, 0), CollectionModel::IsActiveRole).toBool(), false);
    QCOMPARE(model.data(model.index(idx2, 0), CollectionModel::IsActiveRole).toBool(), false);

    model.setActiveCollectionId(id1);
    QCOMPARE(model.activeCollectionId(), id1);
    QCOMPARE(model.data(model.index(idx1, 0), CollectionModel::IsActiveRole).toBool(), true);
    QCOMPARE(model.data(model.index(idx2, 0), CollectionModel::IsActiveRole).toBool(), false);

    model.setActiveCollectionId(id2);
    QCOMPARE(model.activeCollectionId(), id2);
    QCOMPARE(model.data(model.index(idx1, 0), CollectionModel::IsActiveRole).toBool(), false);
    QCOMPARE(model.data(model.index(idx2, 0), CollectionModel::IsActiveRole).toBool(), true);
}

QTEST_MAIN(TestCollectionModel)
#include "test_collection_model.moc"
