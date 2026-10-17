// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/database/catalogdatabase.h"
#include "catalog/database/digikamdatabaseadapter.h"
#include "catalog/catalogitem.h"

#include <QDir>
#include <QSignalSpy>
#include <QSqlQuery>
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
    void testOfflineCollectionPreservation();
    void testPerformanceOptimizationAndNPlusOneElimination();
    void testPagingSupport();
    void testDigikamSchemaDetectionAndRead();
    void testDigikamMetadataUpdate();
    void testDigikamImportIntoCatalog();
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

void TestDatabase::testPerformanceOptimizationAndNPlusOneElimination()
{
    CatalogDatabase db(QStringLiteral("test_conn_perf"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const QString root = QStringLiteral("C:/PerfPhotos");
    const int collId = db.addOrGetCollection(root, QStringLiteral("PerfPhotos"));

    // Insert 50 items with multiple tags to exercise single-pass GROUP_CONCAT batch fetch
    QList<CatalogItem> items;
    for (int i = 0; i < 50; ++i)
    {
        CatalogItem item;
        item.filePath = QStringLiteral("C:/PerfPhotos/photo_%1.jpg").arg(i);
        item.fileName = QStringLiteral("photo_%1.jpg").arg(i);
        item.fileSizeBytes = 1000000 + i * 1000;
        item.dateTaken = QDateTime::currentDateTime().addSecs(-i * 60);
        item.rating = (i % 5) + 1;
        item.tags = {QStringLiteral("TagA"), QStringLiteral("TagB"), QStringLiteral("Tag_%1").arg(i % 5)};
        items.append(item);
    }

    QVERIFY(db.insertOrUpdateItems(items, collId));
    QCOMPARE(db.itemCount(), 50);

    // fetchAllItems executes exactly 1 optimized SQL query without N+1 subqueries
    const auto fetched = db.fetchAllItems();
    QCOMPARE(fetched.size(), 50);

    for (const auto &item : fetched)
    {
        QVERIFY(item.tags.size() >= 2);
        QVERIFY(item.tags.contains(QStringLiteral("TagA")));
        QVERIFY(item.tags.contains(QStringLiteral("TagB")));
    }
}

void TestDatabase::testPagingSupport()
{
    CatalogDatabase db(QStringLiteral("test_conn_paging"));
    QVERIFY(db.open(QStringLiteral(":memory:")));

    const int collId = db.addOrGetCollection(QStringLiteral("C:/PagingPhotos"));

    QList<CatalogItem> items;
    for (int i = 0; i < 30; ++i)
    {
        CatalogItem item;
        item.filePath = QStringLiteral("C:/PagingPhotos/img_%1.jpg").arg(i, 3, 10, QLatin1Char('0'));
        item.fileName = QStringLiteral("img_%1.jpg").arg(i, 3, 10, QLatin1Char('0'));
        item.fileSizeBytes = 50000;
        item.dateTaken = QDateTime::currentDateTime().addSecs(-i * 10);
        items.append(item);
    }
    db.insertOrUpdateItems(items, collId);

    // Fetch page 1: offset 0, limit 10
    const auto page1 = db.fetchItemsPaged(0, 10);
    QCOMPARE(page1.size(), 10);

    // Fetch page 2: offset 10, limit 10
    const auto page2 = db.fetchItemsPaged(10, 10);
    QCOMPARE(page2.size(), 10);

    // Ensure page 1 and page 2 do not overlap
    QVERIFY(page1.first().filePath != page2.first().filePath);

    // Fetch page 3: offset 20, limit 15 (only 10 remaining)
    const auto page3 = db.fetchItemsPaged(20, 15);
    QCOMPARE(page3.size(), 10);
}

void TestDatabase::testDigikamSchemaDetectionAndRead()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = QDir(tempDir.path()).filePath(QStringLiteral("digikam4.db"));

    // 1. Create a mock digiKam database file using DigikamDatabaseAdapter
    {
        QSqlDatabase digiDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("setup_digikam_mock"));
        digiDb.setDatabaseName(dbPath);
        QVERIFY(digiDb.open());
        QVERIFY(DigikamDatabaseAdapter::createDigikamSchema(digiDb));

        QSqlQuery q(digiDb);

        // Insert AlbumRoot
        QVERIFY(q.exec(QStringLiteral(
            "INSERT INTO AlbumRoots (id, label, status, type, identifier, specificPath) "
            "VALUES (1, 'Main Library', 0, 1, 'volumeid:?path=C:/DigikamPhotos', '/');"
        )));

        // Insert Album
        QVERIFY(q.exec(QStringLiteral(
            "INSERT INTO Albums (id, albumRoot, relativePath, caption) "
            "VALUES (10, 1, '/2026/Tokyo', 'Tokyo Trip 2026');"
        )));

        // Insert Images
        QVERIFY(q.exec(QStringLiteral(
            "INSERT INTO Images (id, album, name, status, category, modificationDate, fileSize) "
            "VALUES (100, 10, 'shibuya.jpg', 1, 1, '2026-10-14 18:30:00', 8400000);"
        )));

        // Insert ImageInformation
        QVERIFY(q.exec(QStringLiteral(
            "INSERT INTO ImageInformation (imageid, rating, creationDate, width, height, format) "
            "VALUES (100, 5, '2026-10-14 18:00:00', 6000, 4000, 'JPG');"
        )));

        // Insert ImageMetadata
        QVERIFY(q.exec(QStringLiteral(
            "INSERT INTO ImageMetadata (imageid, make, model, lens, aperture, focalLength, exposureTime, sensitivity) "
            "VALUES (100, 'Sony', 'ILCE-7M4', 'FE 24-70mm F2.8 GM', 2.8, 35.0, 0.004, 800);"
        )));

        // Insert Tags
        QVERIFY(q.exec(QStringLiteral("INSERT INTO Tags (id, name) VALUES (1, 'Travel'), (2, 'Japan');")));
        QVERIFY(q.exec(QStringLiteral("INSERT INTO ImageTags (imageid, tagid) VALUES (100, 1), (100, 2);")));

        digiDb.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("setup_digikam_mock"));

    // 2. Open with CatalogDatabase and verify automatic digiKam schema detection
    CatalogDatabase catalogDb(QStringLiteral("test_conn_digikam_detect"));
    QVERIFY(catalogDb.open(dbPath));
    QVERIFY(catalogDb.isDigikamDatabase());
    QCOMPARE(catalogDb.schemaType(), DatabaseSchemaType::Digikam);

    // Verify collections mapping
    const auto collections = catalogDb.getCollections();
    QCOMPARE(collections.size(), 1);
    QCOMPARE(collections.first().id, 10);
    QCOMPARE(collections.first().name, QStringLiteral("Tokyo Trip 2026"));
    QCOMPARE(collections.first().itemCount, 1);
    QCOMPARE(collections.first().rootPath, QStringLiteral("C:/DigikamPhotos/2026/Tokyo"));

    // Verify items reading
    QCOMPARE(catalogDb.itemCount(), 1);
    const auto items = catalogDb.fetchAllItems();
    QCOMPARE(items.size(), 1);

    const CatalogItem &item = items.first();
    QCOMPARE(item.id, QStringLiteral("100"));
    QCOMPARE(item.fileName, QStringLiteral("shibuya.jpg"));
    QCOMPARE(item.filePath, QStringLiteral("C:/DigikamPhotos/2026/Tokyo/shibuya.jpg"));
    QCOMPARE(item.dimensions, QSize(6000, 4000));
    QCOMPARE(item.rating, 5);
    QCOMPARE(item.cameraModel, QStringLiteral("Sony ILCE-7M4"));
    QCOMPARE(item.lensModel, QStringLiteral("FE 24-70mm F2.8 GM"));
    QCOMPARE(item.aperture, QStringLiteral("f/2.8"));
    QCOMPARE(item.focalLength, QStringLiteral("35 mm"));
    QCOMPARE(item.shutterSpeed, QStringLiteral("1/250s"));
    QCOMPARE(item.iso, 800);
    QCOMPARE(item.tags.size(), 2);
    QVERIFY(item.tags.contains(QStringLiteral("Travel")));
    QVERIFY(item.tags.contains(QStringLiteral("Japan")));

    // Verify allTags
    const auto allTags = catalogDb.allTags();
    QCOMPARE(allTags.size(), 2);
    QVERIFY(allTags.contains(QStringLiteral("Travel")));
}

void TestDatabase::testDigikamMetadataUpdate()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = QDir(tempDir.path()).filePath(QStringLiteral("digikam_update.db"));

    {
        QSqlDatabase digiDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("setup_digikam_upd"));
        digiDb.setDatabaseName(dbPath);
        QVERIFY(digiDb.open());
        DigikamDatabaseAdapter::createDigikamSchema(digiDb);

        QSqlQuery q(digiDb);
        q.exec(QStringLiteral("INSERT INTO AlbumRoots (id, identifier) VALUES (1, 'volumeid:?path=C:/Photos');"));
        q.exec(QStringLiteral("INSERT INTO Albums (id, albumRoot, relativePath) VALUES (1, 1, '/album');"));
        q.exec(QStringLiteral("INSERT INTO Images (id, album, name, status) VALUES (5, 1, 'flower.jpg', 1);"));
        q.exec(QStringLiteral("INSERT INTO ImageInformation (imageid, rating) VALUES (5, 2);"));
        digiDb.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("setup_digikam_upd"));

    CatalogDatabase catalogDb(QStringLiteral("test_conn_digikam_upd"));
    QVERIFY(catalogDb.open(dbPath));
    QVERIFY(catalogDb.isDigikamDatabase());

    const QString filePath = QStringLiteral("C:/Photos/album/flower.jpg");

    // Update rating
    QVERIFY(catalogDb.updateItemRating(filePath, 4));
    QCOMPARE(catalogDb.fetchItemByPath(filePath).rating, 4);

    // Set tags
    QVERIFY(catalogDb.setItemTags(filePath, {QStringLiteral("Macro"), QStringLiteral("Flora")}));
    const auto fetched = catalogDb.fetchItemByPath(filePath);
    QCOMPARE(fetched.tags.size(), 2);
    QVERIFY(fetched.tags.contains(QStringLiteral("Macro")));
    QVERIFY(fetched.tags.contains(QStringLiteral("Flora")));
}

void TestDatabase::testDigikamImportIntoCatalog()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString digiDbPath = QDir(tempDir.path()).filePath(QStringLiteral("digikam_src.db"));

    // Create source digiKam DB
    {
        QSqlDatabase digiDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("setup_digikam_import_src"));
        digiDb.setDatabaseName(digiDbPath);
        QVERIFY(digiDb.open());
        DigikamDatabaseAdapter::createDigikamSchema(digiDb);

        QSqlQuery q(digiDb);
        q.exec(QStringLiteral("INSERT INTO AlbumRoots (id, identifier, label) VALUES (1, 'volumeid:?path=D:/Imports', 'ExtLib');"));
        q.exec(QStringLiteral("INSERT INTO Albums (id, albumRoot, relativePath, caption) VALUES (2, 1, '/Wildlife', 'Wildlife 2026');"));
        q.exec(QStringLiteral("INSERT INTO Images (id, album, name, status, fileSize) VALUES (20, 2, 'eagle.jpg', 1, 9000000);"));
        q.exec(QStringLiteral("INSERT INTO ImageInformation (imageid, rating, width, height) VALUES (20, 5, 5000, 3500);"));
        q.exec(QStringLiteral("INSERT INTO ImageMetadata (imageid, make, model) VALUES (20, 'Canon', 'EOS R5');"));
        q.exec(QStringLiteral("INSERT INTO Tags (id, name) VALUES (10, 'Birds');"));
        q.exec(QStringLiteral("INSERT INTO ImageTags (imageid, tagid) VALUES (20, 10);"));
        digiDb.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("setup_digikam_import_src"));

    // Target native PhotoCarta database
    CatalogDatabase targetDb(QStringLiteral("test_conn_import_target"));
    QVERIFY(targetDb.open(QStringLiteral(":memory:")));
    QCOMPARE(targetDb.schemaType(), DatabaseSchemaType::PhotoCarta);

    // Import digiKam database into PhotoCarta
    QVERIFY(targetDb.importFromDigikamDatabase(digiDbPath));
    QCOMPARE(targetDb.itemCount(), 1);

    const auto collections = targetDb.getCollections();
    QCOMPARE(collections.size(), 1);
    QCOMPARE(collections.first().name, QStringLiteral("Wildlife 2026"));

    const auto item = targetDb.fetchItemByPath(QStringLiteral("D:/Imports/Wildlife/eagle.jpg"));
    QCOMPARE(item.fileName, QStringLiteral("eagle.jpg"));
    QCOMPARE(item.rating, 5);
    QCOMPARE(item.cameraModel, QStringLiteral("Canon EOS R5"));
    QCOMPARE(item.tags, QStringList{QStringLiteral("Birds")});
}

QTEST_MAIN(TestDatabase)
#include "test_database.moc"
