// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogmodel.h"
#include "catalog/selectionmanager.h"
#include "catalog/sessioncontroller.h"

#include <QSignalSpy>
#include <QTest>

using namespace PhotoCarta;

class TestCompareMode : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void testInitialState();
    void testEnterCompareModeWithMultipleSelection();
    void testEnterCompareModeWithSingleSelection();
    void testSyncZoomAndPan();
    void testRatingAndFlaggingInCompare();
    void testWinnerSelection();
    void testSwapAndRemoveSlots();
    void testAddAdjacentToCompare();
    void testMutualExclusionWithPreviewMode();

private:
    CatalogModel *m_catalogModel = nullptr;
    CatalogFilterModel *m_filterModel = nullptr;
    SelectionManager *m_selectionManager = nullptr;
    SessionController *m_sessionController = nullptr;
};

void TestCompareMode::init()
{
    m_catalogModel = new CatalogModel(this);
    m_catalogModel->loadDemoFixtures();

    m_filterModel = new CatalogFilterModel(this);
    m_filterModel->setSourceModel(m_catalogModel);

    m_selectionManager = new SelectionManager(m_filterModel, this);
    m_sessionController = new SessionController(m_catalogModel, m_filterModel, m_selectionManager, this);
}

void TestCompareMode::cleanup()
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

void TestCompareMode::testInitialState()
{
    QCOMPARE(m_sessionController->compareMode(), false);
    QCOMPARE(m_sessionController->syncZoomPan(), true);
    QCOMPARE(m_sessionController->compareCount(), 0);
    QVERIFY(m_sessionController->compareItems().isEmpty());
    QCOMPARE(m_sessionController->compareZoom(), 1.0);
    QCOMPARE(m_sessionController->winnerId(), QString());
}

void TestCompareMode::testEnterCompareModeWithMultipleSelection()
{
    // Select items 1 and 3
    m_selectionManager->select(1);
    m_selectionManager->toggleSelect(3);
    QCOMPARE(m_selectionManager->selectedCount(), 2);

    QSignalSpy modeSpy(m_sessionController, &SessionController::compareModeChanged);
    QSignalSpy itemsSpy(m_sessionController, &SessionController::compareItemsChanged);

    m_sessionController->setCompareMode(true);

    QCOMPARE(m_sessionController->compareMode(), true);
    QCOMPARE(modeSpy.count(), 1);
    QCOMPARE(itemsSpy.count(), 1);
    QCOMPARE(m_sessionController->compareCount(), 2);

    const QVariantList items = m_sessionController->compareItems();
    QCOMPARE(items.size(), 2);

    const QVariantMap itemA = items.at(0).toMap();
    const QVariantMap itemB = items.at(1).toMap();

    QCOMPARE(itemA.value(QStringLiteral("slotIndex")).toInt(), 0);
    QCOMPARE(itemA.value(QStringLiteral("slotLabel")).toString(), QStringLiteral("A"));
    QCOMPARE(itemA.value(QStringLiteral("proxyRow")).toInt(), 1);

    QCOMPARE(itemB.value(QStringLiteral("slotIndex")).toInt(), 1);
    QCOMPARE(itemB.value(QStringLiteral("slotLabel")).toString(), QStringLiteral("B"));
    QCOMPARE(itemB.value(QStringLiteral("proxyRow")).toInt(), 3);
}

void TestCompareMode::testEnterCompareModeWithSingleSelection()
{
    // Select single item 2
    m_selectionManager->select(2);
    QCOMPARE(m_selectionManager->selectedCount(), 1);

    m_sessionController->setCompareMode(true);

    QCOMPARE(m_sessionController->compareMode(), true);
    QCOMPARE(m_sessionController->compareCount(), 2);

    const QVariantList items = m_sessionController->compareItems();
    QCOMPARE(items.size(), 2);
    QCOMPARE(items.at(0).toMap().value(QStringLiteral("proxyRow")).toInt(), 2);
    QCOMPARE(items.at(1).toMap().value(QStringLiteral("proxyRow")).toInt(), 3);
}

void TestCompareMode::testSyncZoomAndPan()
{
    m_selectionManager->select(0);
    m_selectionManager->toggleSelect(1);
    m_sessionController->setCompareMode(true);

    // Sync zoom/pan toggle
    QSignalSpy syncSpy(m_sessionController, &SessionController::syncZoomPanChanged);
    m_sessionController->toggleSyncZoomPan();
    QCOMPARE(m_sessionController->syncZoomPan(), false);
    QCOMPARE(syncSpy.count(), 1);

    m_sessionController->setSyncZoomPan(true);
    QCOMPARE(m_sessionController->syncZoomPan(), true);

    // Zoom adjustments
    QSignalSpy zoomSpy(m_sessionController, &SessionController::compareZoomChanged);
    m_sessionController->setCompareZoom(2.5);
    QCOMPARE(m_sessionController->compareZoom(), 2.5);
    QCOMPARE(zoomSpy.count(), 1);

    m_sessionController->zoomInCompare(0.5);
    QCOMPARE(m_sessionController->compareZoom(), 3.0);

    m_sessionController->zoomOutCompare(1.0);
    QCOMPARE(m_sessionController->compareZoom(), 2.0);

    // Pan adjustments
    QSignalSpy panSpy(m_sessionController, &SessionController::comparePanChanged);
    m_sessionController->setComparePan(120.0, -80.0);
    QCOMPARE(m_sessionController->comparePanX(), 120.0);
    QCOMPARE(m_sessionController->comparePanY(), -80.0);
    QCOMPARE(panSpy.count(), 1);

    // Reset zoom & pan
    m_sessionController->resetCompareZoom();
    QCOMPARE(m_sessionController->compareZoom(), 1.0);
    QCOMPARE(m_sessionController->comparePanX(), 0.0);
    QCOMPARE(m_sessionController->comparePanY(), 0.0);
}

void TestCompareMode::testRatingAndFlaggingInCompare()
{
    m_selectionManager->select(0);
    m_selectionManager->toggleSelect(1);
    m_sessionController->setCompareMode(true);

    // Rate slot 0 to 5 stars
    m_sessionController->setCompareRating(0, 5);
    QVariantList items = m_sessionController->compareItems();
    QCOMPARE(items.at(0).toMap().value(QStringLiteral("rating")).toInt(), 5);

    // Flag slot 1
    const bool wasFlagged = items.at(1).toMap().value(QStringLiteral("isFlagged")).toBool();
    m_sessionController->toggleCompareFlagged(1);
    items = m_sessionController->compareItems();
    QCOMPARE(items.at(1).toMap().value(QStringLiteral("isFlagged")).toBool(), !wasFlagged);
}

void TestCompareMode::testWinnerSelection()
{
    m_selectionManager->select(0);
    m_selectionManager->toggleSelect(1);
    m_sessionController->setCompareMode(true);

    QSignalSpy winnerSpy(m_sessionController, &SessionController::winnerIdChanged);

    // Mark slot 0 as winner
    m_sessionController->setAsWinner(0);
    QCOMPARE(winnerSpy.count(), 1);

    QVariantList items = m_sessionController->compareItems();
    const QString slot0Id = items.at(0).toMap().value(QStringLiteral("id")).toString();
    QCOMPARE(m_sessionController->winnerId(), slot0Id);
    QCOMPARE(items.at(0).toMap().value(QStringLiteral("isWinner")).toBool(), true);
    QCOMPARE(items.at(1).toMap().value(QStringLiteral("isWinner")).toBool(), false);

    // Marking slot 0 again toggles winner off
    m_sessionController->setAsWinner(0);
    QCOMPARE(m_sessionController->winnerId(), QString());
    items = m_sessionController->compareItems();
    QCOMPARE(items.at(0).toMap().value(QStringLiteral("isWinner")).toBool(), false);

    // Clear winner explicitly
    m_sessionController->setAsWinner(1);
    QVERIFY(!m_sessionController->winnerId().isEmpty());
    m_sessionController->clearWinner();
    QCOMPARE(m_sessionController->winnerId(), QString());
}

void TestCompareMode::testSwapAndRemoveSlots()
{
    m_selectionManager->select(0);
    m_selectionManager->toggleSelect(1);
    m_selectionManager->toggleSelect(2);
    m_sessionController->setCompareMode(true);

    QCOMPARE(m_sessionController->compareCount(), 3);
    QVariantList items = m_sessionController->compareItems();
    const int row0 = items.at(0).toMap().value(QStringLiteral("proxyRow")).toInt();
    const int row1 = items.at(1).toMap().value(QStringLiteral("proxyRow")).toInt();

    // Swap slots 0 and 1
    m_sessionController->swapCompareItems(0, 1);
    items = m_sessionController->compareItems();
    QCOMPARE(items.at(0).toMap().value(QStringLiteral("proxyRow")).toInt(), row1);
    QCOMPARE(items.at(1).toMap().value(QStringLiteral("proxyRow")).toInt(), row0);

    // Remove slot 0 -> count becomes 2
    m_sessionController->removeCompareItem(0);
    QCOMPARE(m_sessionController->compareCount(), 2);
    QCOMPARE(m_sessionController->compareMode(), true);

    // Remove slot 0 again -> count falls below 2 -> exits compare mode
    m_sessionController->removeCompareItem(0);
    QCOMPARE(m_sessionController->compareMode(), false);
}

void TestCompareMode::testAddAdjacentToCompare()
{
    m_selectionManager->select(0);
    m_selectionManager->toggleSelect(1);
    m_sessionController->setCompareMode(true);
    QCOMPARE(m_sessionController->compareCount(), 2);

    m_sessionController->addAdjacentToCompare(true);
    QCOMPARE(m_sessionController->compareCount(), 3);

    m_sessionController->addAdjacentToCompare(true);
    QCOMPARE(m_sessionController->compareCount(), 4);

    // Cannot add beyond 4 slots
    m_sessionController->addAdjacentToCompare(true);
    QCOMPARE(m_sessionController->compareCount(), 4);
}

void TestCompareMode::testMutualExclusionWithPreviewMode()
{
    m_sessionController->setPreviewMode(true);
    QCOMPARE(m_sessionController->previewMode(), true);
    QCOMPARE(m_sessionController->compareMode(), false);

    // Entering compare mode disables preview mode
    m_sessionController->setCompareMode(true);
    QCOMPARE(m_sessionController->previewMode(), false);
    QCOMPARE(m_sessionController->compareMode(), true);

    // Entering preview mode disables compare mode
    m_sessionController->setPreviewMode(true);
    QCOMPARE(m_sessionController->previewMode(), true);
    QCOMPARE(m_sessionController->compareMode(), false);
}

QTEST_MAIN(TestCompareMode)
#include "test_compare_mode.moc"
