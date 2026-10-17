// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogitem.h"
#include "catalog/catalogmodel.h"
#include "catalog/database/catalogdatabase.h"
#include "catalog/selectionmanager.h"
#include "catalog/sessioncontroller.h"

#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

class TestMetadataEditing : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void testItemTitleAndCaptionEditing();
    void testItemColorLabelEditing();
    void testBatchColorLabelEditing();
    void testFilterByColorLabel();
    void testFilterByCaptionSearch();
    void testDatabasePersistence();
    void testSessionControllerIntegration();

private:
    CatalogModel *m_catalogModel = nullptr;
    CatalogFilterModel *m_filterModel = nullptr;
    SelectionManager *m_selectionManager = nullptr;
    SessionController *m_sessionController = nullptr;
};

void TestMetadataEditing::init()
{
    m_catalogModel = new CatalogModel(this);
    m_catalogModel->loadDemoFixtures();

    m_filterModel = new CatalogFilterModel(this);
    m_filterModel->setSourceModel(m_catalogModel);

    m_selectionManager = new SelectionManager(m_filterModel, this);
    m_sessionController = new SessionController(m_catalogModel, m_filterModel, m_selectionManager, this);
}

void TestMetadataEditing::cleanup()
{
    delete m_sessionController;
    m_sessionController = nullptr;
    delete m_selectionManager;
    m_selectionManager = nullptr;
    delete m_filterModel;
    m_filterModel = nullptr;
    delete m_catalogModel;
    m_catalogModel = nullptr;
}

void TestMetadataEditing::testItemTitleAndCaptionEditing()
{
    QVERIFY(m_catalogModel->rowCount() > 0);
    const QModelIndex idx0 = m_catalogModel->index(0, 0);

    // Initial check
    const QString origTitle = m_catalogModel->data(idx0, CatalogModel::TitleRole).toString();
    QVERIFY(!origTitle.isEmpty());

    // Update title
    const QString newTitle = QStringLiteral("Sunset Over Snowy Alpine Peaks");
    m_catalogModel->updateItemTitle(0, newTitle);
    QCOMPARE(m_catalogModel->data(idx0, CatalogModel::TitleRole).toString(), newTitle);
    QVERIFY(m_catalogModel->itemAt(0) != nullptr);
    QCOMPARE(m_catalogModel->itemAt(0)->title, newTitle);

    // Update caption
    const QString newCaption = QStringLiteral("Golden hour shot captured with ND filter. ISO 100, f/8, 1/125s.");
    m_catalogModel->updateItemCaption(0, newCaption);
    QCOMPARE(m_catalogModel->data(idx0, CatalogModel::CaptionRole).toString(), newCaption);
    QCOMPARE(m_catalogModel->itemAt(0)->caption, newCaption);
}

void TestMetadataEditing::testItemColorLabelEditing()
{
    QVERIFY(m_catalogModel->rowCount() > 0);
    const QModelIndex idx0 = m_catalogModel->index(0, 0);

    // Set to Red (1)
    m_catalogModel->updateItemColorLabel(0, 1);
    QCOMPARE(m_catalogModel->data(idx0, CatalogModel::ColorLabelRole).toInt(), 1);
    QVERIFY(m_catalogModel->itemAt(0) != nullptr);
    QCOMPARE(m_catalogModel->itemAt(0)->colorLabel, 1);

    // Set to Green (3)
    m_catalogModel->updateItemColorLabel(0, 3);
    QCOMPARE(m_catalogModel->data(idx0, CatalogModel::ColorLabelRole).toInt(), 3);
    QCOMPARE(m_catalogModel->itemAt(0)->colorLabel, 3);

    // Clear (0)
    m_catalogModel->updateItemColorLabel(0, 0);
    QCOMPARE(m_catalogModel->data(idx0, CatalogModel::ColorLabelRole).toInt(), 0);
    QCOMPARE(m_catalogModel->itemAt(0)->colorLabel, 0);
}

void TestMetadataEditing::testBatchColorLabelEditing()
{
    QVERIFY(m_catalogModel->rowCount() >= 3);

    // Batch set color label to Blue (4)
    m_catalogModel->updateItemsColorLabel({0, 2}, 4);
    QVERIFY(m_catalogModel->itemAt(0) != nullptr);
    QVERIFY(m_catalogModel->itemAt(2) != nullptr);
    QCOMPARE(m_catalogModel->itemAt(0)->colorLabel, 4);
    QCOMPARE(m_catalogModel->itemAt(2)->colorLabel, 4);

    // Batch set title
    m_catalogModel->updateItemsTitle({0, 1}, QStringLiteral("Batch Edited Title"));
    QCOMPARE(m_catalogModel->itemAt(0)->title, QStringLiteral("Batch Edited Title"));
    QCOMPARE(m_catalogModel->itemAt(1)->title, QStringLiteral("Batch Edited Title"));
}

void TestMetadataEditing::testFilterByColorLabel()
{
    QVERIFY(m_catalogModel->rowCount() >= 4);

    // Explicitly configure color labels on items
    m_catalogModel->updateItemColorLabel(0, 1); // Red
    m_catalogModel->updateItemColorLabel(1, 2); // Yellow
    m_catalogModel->updateItemColorLabel(2, 1); // Red
    m_catalogModel->updateItemColorLabel(3, 4); // Blue

    // Filter by Red (1)
    m_filterModel->setColorLabelFilter(1);
    QCOMPARE(m_filterModel->colorLabelFilter(), 1);
    QCOMPARE(m_filterModel->count(), 2);

    // Filter by Yellow (2)
    m_filterModel->setColorLabelFilter(2);
    QCOMPARE(m_filterModel->count(), 1);

    // Filter by Green (3) - none
    m_filterModel->setColorLabelFilter(3);
    QCOMPARE(m_filterModel->count(), 0);

    // Reset filter (0)
    m_filterModel->setColorLabelFilter(0);
    QCOMPARE(m_filterModel->count(), m_catalogModel->rowCount());
}

void TestMetadataEditing::testFilterByCaptionSearch()
{
    QVERIFY(m_catalogModel->rowCount() >= 2);

    // Assign a unique phrase in caption
    m_catalogModel->updateItemCaption(1, QStringLiteral("Long exposure waterfall in Icelandic canyon"));

    // Search for waterfall
    m_filterModel->setSearchQuery(QStringLiteral("waterfall"));
    QCOMPARE(m_filterModel->count(), 1);

    // Search for canyon
    m_filterModel->setSearchQuery(QStringLiteral("canyon"));
    QCOMPARE(m_filterModel->count(), 1);

    // Search for something not in caption or other fields
    m_filterModel->setSearchQuery(QStringLiteral("nonexistentxyz123"));
    QCOMPARE(m_filterModel->count(), 0);

    // Clear search
    m_filterModel->setSearchQuery(QString());
    QCOMPARE(m_filterModel->count(), m_catalogModel->rowCount());
}

void TestMetadataEditing::testDatabasePersistence()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = tempDir.filePath(QStringLiteral("test_metadata.db"));

    CatalogItem item;
    item.id = QStringLiteral("item-meta-1");
    item.filePath = QStringLiteral("/photos/landscape.jpg");
    item.fileName = QStringLiteral("landscape.jpg");
    item.title = QStringLiteral("Majestic Mountain Sunset");
    item.caption = QStringLiteral("Captured during summer solstice with telephoto lens.");
    item.colorLabel = 2; // Yellow
    item.rating = 5;
    item.isFlagged = true;
    item.fileSizeBytes = 10485760;

    {
        CatalogDatabase db(QStringLiteral("test_meta_conn"));
        QVERIFY(db.open(dbPath));

        const int collId = db.addOrGetCollection(QStringLiteral("/photos"), QStringLiteral("Photos"), QStringLiteral("local"));
        item.collectionId = collId;

        QVERIFY(db.insertOrUpdateItems({item}));

        // Fetch back and verify
        const CatalogItem fetched = db.fetchItemByPath(item.filePath);
        QCOMPARE(fetched.id, item.id);
        QCOMPARE(fetched.title, item.title);
        QCOMPARE(fetched.caption, item.caption);
        QCOMPARE(fetched.colorLabel, 2);

        // Update single fields
        QVERIFY(db.updateItemTitle(item.id, QStringLiteral("Updated Mountain Sunset")));
        QVERIFY(db.updateItemCaption(item.id, QStringLiteral("Updated caption text.")));
        QVERIFY(db.updateItemColorLabel(item.id, 5)); // Purple

        const CatalogItem updated = db.fetchItemByPath(item.filePath);
        QCOMPARE(updated.title, QStringLiteral("Updated Mountain Sunset"));
        QCOMPARE(updated.caption, QStringLiteral("Updated caption text."));
        QCOMPARE(updated.colorLabel, 5);

        db.close();
    }

    // Reopen and ensure persistence across database restart
    {
        CatalogDatabase db2(QStringLiteral("test_meta_conn_reopen"));
        QVERIFY(db2.open(dbPath));

        const CatalogItem reloaded = db2.fetchItemByPath(item.filePath);
        QCOMPARE(reloaded.id, item.id);
        QCOMPARE(reloaded.title, QStringLiteral("Updated Mountain Sunset"));
        QCOMPARE(reloaded.caption, QStringLiteral("Updated caption text."));
        QCOMPARE(reloaded.colorLabel, 5);

        db2.close();
    }
}

void TestMetadataEditing::testSessionControllerIntegration()
{
    QVERIFY(m_catalogModel->rowCount() > 0);

    // Select the first item
    m_selectionManager->select(0);
    QCOMPARE(m_selectionManager->currentIndex(), 0);

    // Edit Title via controller
    m_sessionController->setTitleForSelected(QStringLiteral("Controller Assigned Title"));
    QVERIFY(m_catalogModel->itemAt(0) != nullptr);
    QCOMPARE(m_catalogModel->itemAt(0)->title, QStringLiteral("Controller Assigned Title"));

    // Edit Caption via controller
    m_sessionController->setCaptionForSelected(QStringLiteral("Controller Assigned Caption"));
    QCOMPARE(m_catalogModel->itemAt(0)->caption, QStringLiteral("Controller Assigned Caption"));

    // Set Color Label via controller
    m_sessionController->setColorLabelForSelected(3); // Green
    QCOMPARE(m_catalogModel->itemAt(0)->colorLabel, 3);

    // Test Color Label Filter in controller
    m_sessionController->setColorLabelFilter(3);
    QCOMPARE(m_filterModel->colorLabelFilter(), 3);

    // Test Clear Color Label Filter
    m_sessionController->clearColorLabelFilter();
    QCOMPARE(m_filterModel->colorLabelFilter(), 0);

    // Test Clear All Filters
    m_sessionController->setColorLabelFilter(2);
    QCOMPARE(m_filterModel->colorLabelFilter(), 2);
    m_sessionController->clearAllFilters();
    QCOMPARE(m_filterModel->colorLabelFilter(), 0);
}

QTEST_MAIN(TestMetadataEditing)
#include "test_metadata_editing.moc"
