// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogmodel.h"
#include "catalog/selectionmanager.h"

#include <QSignalSpy>
#include <QTest>

using namespace PhotoCarta;

class TestSelectionManager : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void testInitialSelection();
    void testSingleSelect();
    void testToggleSelect();
    void testSelectAllAndClear();
    void testNavigation();
    void testModelResetAdjustment();
    void testSelectionPreservedOnSortAndFilter();
    void testSelectRange();
    void testCommonAndAllTags();
    void testSelectedProxyRowsAndSummary();

private:
    CatalogModel *m_catalogModel = nullptr;
    CatalogFilterModel *m_filterModel = nullptr;
    SelectionManager *m_selectionManager = nullptr;
};

void TestSelectionManager::init()
{
    m_catalogModel = new CatalogModel(this);
    m_catalogModel->loadDemoFixtures();

    m_filterModel = new CatalogFilterModel(this);
    m_filterModel->setSourceModel(m_catalogModel);

    m_selectionManager = new SelectionManager(m_filterModel, this);
}

void TestSelectionManager::cleanup()
{
    delete m_selectionManager;
    m_selectionManager = nullptr;
    delete m_filterModel;
    m_filterModel = nullptr;
    delete m_catalogModel;
    m_catalogModel = nullptr;
}

void TestSelectionManager::testInitialSelection()
{
    QCOMPARE(m_selectionManager->currentIndex(), -1);
    QCOMPARE(m_selectionManager->hasSelection(), false);
    QCOMPARE(m_selectionManager->selectedCount(), 0);

    m_selectionManager->select(0);
    QCOMPARE(m_selectionManager->currentIndex(), 0);
    QCOMPARE(m_selectionManager->hasSelection(), true);
    QCOMPARE(m_selectionManager->selectedCount(), 1);

    const QVariantMap selected = m_selectionManager->selectedItem();
    QVERIFY(!selected.isEmpty());
    const QString expectedId = m_filterModel->index(0, 0).data(CatalogModel::IdRole).toString();
    QCOMPARE(selected.value(QStringLiteral("id")).toString(), expectedId);
}

void TestSelectionManager::testSingleSelect()
{
    m_selectionManager->select(2);
    QCOMPARE(m_selectionManager->currentIndex(), 2);
    QVERIFY(m_selectionManager->isSelected(2));
    QVERIFY(!m_selectionManager->isSelected(1));

    // Clamping past end
    m_selectionManager->select(100);
    QCOMPARE(m_selectionManager->currentIndex(), 5);

    // Negative index clears selection
    m_selectionManager->select(-1);
    QCOMPARE(m_selectionManager->currentIndex(), -1);
    QCOMPARE(m_selectionManager->hasSelection(), false);
}

void TestSelectionManager::testToggleSelect()
{
    m_selectionManager->select(1);
    QCOMPARE(m_selectionManager->selectedCount(), 1);

    // Toggle add row 3
    m_selectionManager->toggleSelect(3);
    QCOMPARE(m_selectionManager->selectedCount(), 2);
    QVERIFY(m_selectionManager->isSelected(1));
    QVERIFY(m_selectionManager->isSelected(3));

    // Toggle remove row 1
    m_selectionManager->toggleSelect(1);
    QCOMPARE(m_selectionManager->selectedCount(), 1);
    QVERIFY(!m_selectionManager->isSelected(1));
    QVERIFY(m_selectionManager->isSelected(3));
}

void TestSelectionManager::testSelectAllAndClear()
{
    m_selectionManager->selectAll();
    QCOMPARE(m_selectionManager->selectedCount(), 6);

    for (int i = 0; i < 6; ++i)
    {
        QVERIFY(m_selectionManager->isSelected(i));
    }

    m_selectionManager->clearSelection();
    QCOMPARE(m_selectionManager->selectedCount(), 0);
    QCOMPARE(m_selectionManager->currentIndex(), -1);
    QCOMPARE(m_selectionManager->hasSelection(), false);
}

void TestSelectionManager::testNavigation()
{
    m_selectionManager->select(0);
    m_selectionManager->selectNext();
    QCOMPARE(m_selectionManager->currentIndex(), 1);

    m_selectionManager->selectPrevious();
    QCOMPARE(m_selectionManager->currentIndex(), 0);

    // Cannot go before 0
    m_selectionManager->selectPrevious();
    QCOMPARE(m_selectionManager->currentIndex(), 0);

    m_selectionManager->selectLast();
    QCOMPARE(m_selectionManager->currentIndex(), 5);

    // Cannot go past last
    m_selectionManager->selectNext();
    QCOMPARE(m_selectionManager->currentIndex(), 5);

    m_selectionManager->selectFirst();
    QCOMPARE(m_selectionManager->currentIndex(), 0);
}

void TestSelectionManager::testModelResetAdjustment()
{
    m_selectionManager->select(4);
    QCOMPARE(m_selectionManager->currentIndex(), 4);

    // Clear underlying catalog items
    m_catalogModel->clearSession();
    QCOMPARE(m_selectionManager->currentIndex(), -1);
    QCOMPARE(m_selectionManager->hasSelection(), false);

    // Reload fixtures: selection should select index 0
    m_catalogModel->loadDemoFixtures();
    QCOMPARE(m_selectionManager->currentIndex(), 0);
    QCOMPARE(m_selectionManager->hasSelection(), true);
}

void TestSelectionManager::testSelectionPreservedOnSortAndFilter()
{
    m_selectionManager->selectById(QStringLiteral("demo-005"));
    QCOMPARE(m_selectionManager->currentId(), QStringLiteral("demo-005"));

    // Sort by name ascending
    m_filterModel->setSortCriterion(QStringLiteral("name"));
    m_filterModel->setSortAscending(true);
    // The photo identity should remain demo-005 despite sort order changing!
    QCOMPARE(m_selectionManager->currentId(), QStringLiteral("demo-005"));
    QVERIFY(m_selectionManager->currentIndex() >= 0);

    // Filter to category where demo-005 is included (all, demo)
    m_filterModel->setCategoryFilter(QStringLiteral("demo"));
    QCOMPARE(m_selectionManager->currentId(), QStringLiteral("demo-005"));

    // Filter to an unmatched search query: item is hidden in proxy view
    m_filterModel->setSearchQuery(QStringLiteral("non_existent_search_query"));
    QCOMPARE(m_selectionManager->currentIndex(), -1);
    // But identity is preserved!
    QCOMPARE(m_selectionManager->currentId(), QStringLiteral("demo-005"));

    // Clear search: item reappears and is selected
    m_filterModel->setSearchQuery(QString());
    QCOMPARE(m_selectionManager->currentId(), QStringLiteral("demo-005"));
    QVERIFY(m_selectionManager->currentIndex() >= 0);
}

void TestSelectionManager::testSelectRange()
{
    m_selectionManager->select(1);
    QCOMPARE(m_selectionManager->currentIndex(), 1);
    QCOMPARE(m_selectionManager->selectedCount(), 1);

    // Range forward from 1 to 4
    m_selectionManager->selectRange(4);
    QCOMPARE(m_selectionManager->selectedCount(), 4);
    QVERIFY(!m_selectionManager->isSelected(0));
    QVERIFY(m_selectionManager->isSelected(1));
    QVERIFY(m_selectionManager->isSelected(2));
    QVERIFY(m_selectionManager->isSelected(3));
    QVERIFY(m_selectionManager->isSelected(4));
    QVERIFY(!m_selectionManager->isSelected(5));
    QCOMPARE(m_selectionManager->currentIndex(), 4);

    // Range backward: select 3, then range to 1
    m_selectionManager->select(3);
    m_selectionManager->selectRange(1);
    QCOMPARE(m_selectionManager->selectedCount(), 3);
    QVERIFY(m_selectionManager->isSelected(1));
    QVERIFY(m_selectionManager->isSelected(2));
    QVERIFY(m_selectionManager->isSelected(3));
}

void TestSelectionManager::testCommonAndAllTags()
{
    // Demo fixtures:
    // Item 0 (demo-001): Landscape, Mountain, Sunrise, Demo
    // Item 5 (demo-006): Astro, Night, Landscape, Demo
    m_selectionManager->clearSelection();
    m_selectionManager->select(0);
    m_selectionManager->toggleSelect(5);
    QCOMPARE(m_selectionManager->selectedCount(), 2);

    const QStringList common = m_selectionManager->commonTags();
    QVERIFY(common.contains(QStringLiteral("Landscape")));
    QVERIFY(common.contains(QStringLiteral("Demo")));
    QVERIFY(!common.contains(QStringLiteral("Astro")));
    QVERIFY(!common.contains(QStringLiteral("Mountain")));

    const QStringList allTags = m_selectionManager->allSelectedTags();
    QVERIFY(allTags.contains(QStringLiteral("Landscape")));
    QVERIFY(allTags.contains(QStringLiteral("Demo")));
    QVERIFY(allTags.contains(QStringLiteral("Astro")));
    QVERIFY(allTags.contains(QStringLiteral("Mountain")));
    QVERIFY(allTags.contains(QStringLiteral("Sunrise")));
    QVERIFY(allTags.contains(QStringLiteral("Night")));
}

void TestSelectionManager::testSelectedProxyRowsAndSummary()
{
    m_selectionManager->clearSelection();
    m_selectionManager->select(1);
    m_selectionManager->toggleSelect(3);

    const QList<int> rows = m_selectionManager->selectedProxyRows();
    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows.at(0), 1);
    QCOMPARE(rows.at(1), 3);

    const QVariantList summary = m_selectionManager->selectedItemsSummary(4);
    QCOMPARE(summary.size(), 2);
    QVERIFY(!summary.at(0).toMap().value(QStringLiteral("id")).toString().isEmpty());
    QVERIFY(!summary.at(1).toMap().value(QStringLiteral("id")).toString().isEmpty());
}

QTEST_MAIN(TestSelectionManager)
#include "test_selection_manager.moc"
