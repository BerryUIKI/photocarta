// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/database/catalogdatabase.h"
#include "catalog/catalogitem.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

class TestDatabase : public QObject
{
    Q_OBJECT

private slots:
    void testOpenAndCloseMemoryDatabase();
    void testCollectionManagement();
    void testItemInsertionAndFetch();
    void testRatingAndFlagPersistence();
    void testTagsPersistence();
    void testBatchMetadataPersistence();
    void testOfflineCollectionPreservation();
};

void TestDatabase::testOpenAndCloseMemoryDatabase()
{
    CatalogDatabase db(QStringLiteral("test_conn_mem"));
    QSignalSpy openedSpy(&db, &CatalogDatabase::databaseOpened);
    QSignalSpy closedSpy(&db, &CatalogDatabase::databaseClosed);

    QVERIFY(!db.isOpen());
    QVERIFY(db.open(QStringLiteral(":memory:")));
    QVERIFY(db.isOpen());
    QCOMPARE(openedSpy.count(), 1);

    db.close();
    QVERIFY(!db.isOpen());
    QCOMPARE(closedSpy.count(), 1);
}

void TestDatabase::testCollectionManagement()
{
    CatalogDatabase db(QStringLiteral("test_conn_coll"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const QString root1 = QStringLiteral("C:/Photos/2026_Vacation");
    const int collId1 = db.addOrGetCollection(root1, QStringLiteral("Vacation"), QStringLiteral("local"));
    QVERIFY(collId1 > 0);

    // Re-adding same root path returns existing ID
    const int collId2 = db.addOrGetCollection(root1);
    QCOMPARE(collId1, collId2);

    const auto collections = db.getCollections();
    QCOMPARE(collections.size(), 1);
    QCOMPARE(collections.first().rootPath, root1);
    QCOMPARE(collections.first().name, QStringLiteral("Vacation"));
    QCOMPARE(collections.first().collectionType, QStringLiteral("local"));
    QCOMPARE(collections.first().isAvailable, true);

    const auto found = db.getCollectionByPath(root1);
    QCOMPARE(found.id, collId1);
}

void TestDatabase::testItemInsertionAndFetch()
{
    CatalogDatabase db(QStringLiteral("test_conn_items"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const QString root = QStringLiteral("C:/Photos");
    const int collId = db.addOrGetCollection(root, QStringLiteral("Photos"));

    CatalogItem item1;
    item1.id = QStringLiteral("item-001");
    item1.filePath = QStringLiteral("C:/Photos/landscape.jpg");
    item1.fileName = QStringLiteral("landscape.jpg");
    item1.title = QStringLiteral("Mountain Vista");
    item1.fileSizeBytes = 5432100;
    item1.formattedFileSize = CatalogItem::formatBytes(item1.fileSizeBytes);
    item1.mimeType = QStringLiteral("image/jpeg");
    item1.dimensions = QSize(4000, 3000);
    item1.cameraModel = QStringLiteral("Nikon Z7");
    item1.lensModel = QStringLiteral("NIKKOR Z 24-70mm f/4 S");
    item1.rating = 4;
    item1.isFlagged = true;
    item1.tags = {QStringLiteral("Nature"), QStringLiteral("Mountains")};

    CatalogItem item2;
    item2.id = QStringLiteral("item-002");
    item2.filePath = QStringLiteral("C:/Photos/portrait.png");
    item2.fileName = QStringLiteral("portrait.png");
    item2.title = QStringLiteral("Studio Portrait");
    item2.fileSizeBytes = 1234500;
    item2.formattedFileSize = CatalogItem::formatBytes(item2.fileSizeBytes);
    item2.mimeType = QStringLiteral("image/png");
    item2.dimensions = QSize(2000, 3000);
    item2.rating = 5;

    QSignalSpy itemsChangedSpy(&db, &CatalogDatabase::itemsChanged);
    QVERIFY(db.insertOrUpdateItems({item1, item2}, collId));
    QCOMPARE(itemsChangedSpy.count(), 1);
    QCOMPARE(db.itemCount(), 2);

    const auto all = db.fetchAllItems();
    QCOMPARE(all.size(), 2);

    const auto fetched1 = db.fetchItemByPath(item1.filePath);
    QCOMPARE(fetched1.id, QStringLiteral("1"));
    QCOMPARE(fetched1.filePath, item1.filePath);
    QCOMPARE(fetched1.title, item1.title);
    QCOMPARE(fetched1.rating, 4);
    QCOMPARE(fetched1.isFlagged, true);
    QCOMPARE(fetched1.cameraModel, QStringLiteral("Nikon Z7"));
    QCOMPARE(fetched1.tags.size(), 2);
    QVERIFY(fetched1.tags.contains(QStringLiteral("Nature")));
}

void TestDatabase::testRatingAndFlagPersistence()
{
    CatalogDatabase db(QStringLiteral("test_conn_meta"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const QString path = QStringLiteral("C:/Photos/test.jpg");
    CatalogItem item;
    item.id = QStringLiteral("test-1");
    item.filePath = path;
    item.fileName = QStringLiteral("test.jpg");
    item.rating = 1;
    item.isFlagged = false;
    db.insertOrUpdateItems({item});

    QVERIFY(db.updateItemRating(path, 5));
    QCOMPARE(db.fetchItemByPath(path).rating, 5);

    QVERIFY(db.updateItemFlagged(path, true));
    QCOMPARE(db.fetchItemByPath(path).isFlagged, true);
}

void TestDatabase::testTagsPersistence()
{
    CatalogDatabase db(QStringLiteral("test_conn_tags"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const QString path = QStringLiteral("C:/Photos/beach.jpg");
    CatalogItem item;
    item.id = QStringLiteral("beach-1");
    item.filePath = path;
    item.fileName = QStringLiteral("beach.jpg");
    item.tags = {QStringLiteral("Ocean"), QStringLiteral("Summer")};
    db.insertOrUpdateItems({item});

    auto tags = db.allTags();
    QCOMPARE(tags.size(), 2);
    QVERIFY(tags.contains(QStringLiteral("Ocean")));
    QVERIFY(tags.contains(QStringLiteral("Summer")));

    // Update tags
    QVERIFY(db.setItemTags(path, {QStringLiteral("Coast"), QStringLiteral("Sunset")}));
    const auto fetched = db.fetchItemByPath(path);
    QCOMPARE(fetched.tags.size(), 2);
    QVERIFY(fetched.tags.contains(QStringLiteral("Coast")));
    QVERIFY(fetched.tags.contains(QStringLiteral("Sunset")));
}

void TestDatabase::testBatchMetadataPersistence()
{
    CatalogDatabase db(QStringLiteral("test_conn_batch"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    CatalogItem item1;
    item1.id = QStringLiteral("batch-1");
    item1.filePath = QStringLiteral("C:/Photos/first.jpg");
    item1.fileName = QStringLiteral("first.jpg");

    CatalogItem item2;
    item2.id = QStringLiteral("batch-2");
    item2.filePath = QStringLiteral("C:/Photos/second.jpg");
    item2.fileName = QStringLiteral("second.jpg");

    QVERIFY(db.insertOrUpdateItems({item1, item2}));
    const QStringList paths = {item1.filePath, item2.filePath};

    QSignalSpy itemsChangedSpy(&db, &CatalogDatabase::itemsChanged);

    QVERIFY(db.updateItemsRating(paths, 3));
    QCOMPARE(itemsChangedSpy.count(), 1);
    QCOMPARE(db.fetchItemByPath(item1.filePath).rating, 3);
    QCOMPARE(db.fetchItemByPath(item2.filePath).rating, 3);

    QVERIFY(db.updateItemsFlagged({item1.filePath}, true));
    QCOMPARE(db.fetchItemByPath(item1.filePath).isFlagged, true);
    QCOMPARE(db.fetchItemByPath(item2.filePath).isFlagged, false);

    QVERIFY(db.addTagsToItems(paths, {QStringLiteral("Shared"), QStringLiteral("Beach")}));
    QCOMPARE(db.fetchItemByPath(item1.filePath).tags.size(), 2);
    QVERIFY(db.fetchItemByPath(item1.filePath).tags.contains(QStringLiteral("Shared")));
    QVERIFY(db.fetchItemByPath(item2.filePath).tags.contains(QStringLiteral("Beach")));

    QVERIFY(db.removeTagsFromItems(paths, {QStringLiteral("Beach")}));
    QVERIFY(!db.fetchItemByPath(item1.filePath).tags.contains(QStringLiteral("Beach")));
    QVERIFY(db.fetchItemByPath(item1.filePath).tags.contains(QStringLiteral("Shared")));
    QCOMPARE(db.fetchItemByPath(item2.filePath).tags.size(), 1);
}

void TestDatabase::testOfflineCollectionPreservation()
{
    CatalogDatabase db(QStringLiteral("test_conn_offline"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const QString removableRoot = QStringLiteral("E:/ExternalDrive/Photos");
    const int collId = db.addOrGetCollection(removableRoot, QStringLiteral("USB Backup"), QStringLiteral("removable"));

    CatalogItem item;
    item.id = QStringLiteral("usb-001");
    item.filePath = QStringLiteral("E:/ExternalDrive/Photos/photo1.jpg");
    item.fileName = QStringLiteral("photo1.jpg");
    item.rating = 5;
    item.tags = {QStringLiteral("Important")};
    db.insertOrUpdateItems({item}, collId);

    QCOMPARE(db.itemCount(), 1);
    QCOMPARE(db.fetchItemByPath(item.filePath).isMissing, false);

    // Simulate drive disconnection: mark collection offline
    QVERIFY(db.setCollectionAvailable(removableRoot, false));

    // Collection marked unavailable
    const auto coll = db.getCollectionByPath(removableRoot);
    QCOMPARE(coll.isAvailable, false);

    // Items are preserved (NOT deleted) but flagged as missing
    QCOMPARE(db.itemCount(), 1);
    const auto offlineItem = db.fetchItemByPath(item.filePath);
    QCOMPARE(offlineItem.isMissing, true);
    QCOMPARE(offlineItem.rating, 5);
    QVERIFY(offlineItem.tags.contains(QStringLiteral("Important")));

    // Reconnect drive: collection marked available again
    QVERIFY(db.setCollectionAvailable(removableRoot, true));
    QCOMPARE(db.getCollectionByPath(removableRoot).isAvailable, true);
    QCOMPARE(db.fetchItemByPath(item.filePath).isMissing, false);
}

QTEST_MAIN(TestDatabase)
#include "test_database.moc"
