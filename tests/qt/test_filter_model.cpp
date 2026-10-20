// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogmodel.h"

#include <QSignalSpy>
#include <QTest>

using namespace PhotoCarta;

class TestFilterModel : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void testSearchQuery();
    void testCategoryFilter();
    void testRatingFilter();
    void testTagFilter();
    void testSorting();
    void testRowMapping();

private:
    CatalogModel *m_catalogModel = nullptr;
    CatalogFilterModel *m_filterModel = nullptr;
};

void TestFilterModel::init()
{
    m_catalogModel = new CatalogModel(this);
    m_catalogModel->loadDemoFixtures();

    m_filterModel = new CatalogFilterModel(this);
    m_filterModel->setSourceModel(m_catalogModel);
}

void TestFilterModel::cleanup()
{
    delete m_filterModel;
    m_filterModel = nullptr;
    delete m_catalogModel;
    m_catalogModel = nullptr;
}

void TestFilterModel::testSearchQuery()
{
    QCOMPARE(m_filterModel->count(), 6);

    // Search by camera brand
    m_filterModel->setSearchQuery(QStringLiteral("Sony"));
    QCOMPARE(m_filterModel->count(), 3); // A7 IV, A7R V, A7S III

    // Search by tag
    m_filterModel->setSearchQuery(QStringLiteral("Macro"));
    QCOMPARE(m_filterModel->count(), 1);

    // Search by part of filename (case-insensitive)
    m_filterModel->setSearchQuery(QStringLiteral("neon"));
    QCOMPARE(m_filterModel->count(), 1);

    // Search with no matches
    m_filterModel->setSearchQuery(QStringLiteral("xyz_unmatched_query"));
    QCOMPARE(m_filterModel->count(), 0);

    // Reset search
    m_filterModel->setSearchQuery(QString());
    QCOMPARE(m_filterModel->count(), 6);
}

void TestFilterModel::testCategoryFilter()
{
    // Demo category
    m_filterModel->setCategoryFilter(QStringLiteral("demo"));
    QCOMPARE(m_filterModel->count(), 6);

    // Imported category (no local files imported yet)
    m_filterModel->setCategoryFilter(QStringLiteral("imported"));
    QCOMPARE(m_filterModel->count(), 0);

    // Favorites category (4 or 5 stars or flagged)
    m_filterModel->setCategoryFilter(QStringLiteral("favorites"));
    // Demo items: item1 (5 stars), item2 (5 stars), item3 (4 stars), item4 (4 stars), item5 (3 stars), item6 (5 stars)
    // Favorites (rating >= 4 or flagged) => item1, item2, item3, item4, item6 = 5 items
    QCOMPARE(m_filterModel->count(), 5);

    // Reset
    m_filterModel->setCategoryFilter(QStringLiteral("all"));
    QCOMPARE(m_filterModel->count(), 6);
}

void TestFilterModel::testRatingFilter()
{
    m_filterModel->setMinRating(5);
    QCOMPARE(m_filterModel->count(), 3); // 3 demo items have rating 5

    m_filterModel->setMinRating(4);
    QCOMPARE(m_filterModel->count(), 5); // 5 demo items have rating >= 4

    m_filterModel->setMinRating(0);
    QCOMPARE(m_filterModel->count(), 6);
}

void TestFilterModel::testTagFilter()
{
    m_filterModel->setTagFilter(QStringLiteral("Landscape"));
    QCOMPARE(m_filterModel->count(), 2);

    // Tag matching is case-insensitive
    m_filterModel->setTagFilter(QStringLiteral("landscape"));
    QCOMPARE(m_filterModel->count(), 2);

    // Tag and rating filters combine
    m_filterModel->setMinRating(5);
    QCOMPARE(m_filterModel->count(), 2);

    m_filterModel->setTagFilter(QStringLiteral("Night"));
    QCOMPARE(m_filterModel->count(), 1); // Only the 5-star astro item has Night

    m_filterModel->setTagFilter(QStringLiteral("NoSuchTag"));
    QCOMPARE(m_filterModel->count(), 0);

    m_filterModel->setTagFilter(QString());
    m_filterModel->setMinRating(0);
    QCOMPARE(m_filterModel->count(), 6);
}

void TestFilterModel::testSorting()
{
    m_filterModel->setSortCriterion(QStringLiteral("name"));
    m_filterModel->setSortAscending(true);

    const QString first = m_filterModel->index(0, 0).data(CatalogModel::FileNameRole).toString();
    const QString last = m_filterModel->index(m_filterModel->count() - 1, 0).data(CatalogModel::FileNameRole).toString();
    QVERIFY(QString::compare(first, last, Qt::CaseInsensitive) < 0);

    m_filterModel->setSortAscending(false);
    const QString newFirst = m_filterModel->index(0, 0).data(CatalogModel::FileNameRole).toString();
    QCOMPARE(newFirst, last);
}

void TestFilterModel::testRowMapping()
{
    m_filterModel->setSearchQuery(QStringLiteral("Macro"));
    QCOMPARE(m_filterModel->count(), 1);

    const int srcRow = m_filterModel->mapToSourceRow(0);
    QCOMPARE(srcRow, 4); // Demo item 5 (index 4) is Monstera Dew Drops Macro

    const int proxyRow = m_filterModel->mapFromSourceRow(4);
    QCOMPARE(proxyRow, 0);

    const int unmappedRow = m_filterModel->mapFromSourceRow(0); // Item 0 is filtered out
    QCOMPARE(unmappedRow, -1);
}

QTEST_MAIN(TestFilterModel)
#include "test_filter_model.moc"
