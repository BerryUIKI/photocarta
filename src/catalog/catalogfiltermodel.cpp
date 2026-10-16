// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalogfiltermodel.h"
#include "catalogmodel.h"

#include <QDateTime>

namespace PhotoCarta
{

CatalogFilterModel::CatalogFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    updateSorting();

    connect(this, &QAbstractItemModel::rowsInserted, this, &CatalogFilterModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &CatalogFilterModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &CatalogFilterModel::countChanged);
    connect(this, &QAbstractItemModel::layoutChanged, this, &CatalogFilterModel::countChanged);
}

QString CatalogFilterModel::searchQuery() const
{
    return m_searchQuery;
}

void CatalogFilterModel::setSearchQuery(const QString &query)
{
    if (m_searchQuery == query)
    {
        return;
    }
    m_searchQuery = query;
    emit searchQueryChanged();
    invalidateFilter();
    emit countChanged();
}

QString CatalogFilterModel::categoryFilter() const
{
    return m_categoryFilter;
}

void CatalogFilterModel::setCategoryFilter(const QString &category)
{
    if (m_categoryFilter == category)
    {
        return;
    }
    m_categoryFilter = category.toLower();
    emit categoryFilterChanged();
    invalidateFilter();
    emit countChanged();
}

QString CatalogFilterModel::tagFilter() const
{
    return m_tagFilter;
}

void CatalogFilterModel::setTagFilter(const QString &tag)
{
    if (m_tagFilter == tag)
    {
        return;
    }
    m_tagFilter = tag;
    emit tagFilterChanged();
    invalidateFilter();
    emit countChanged();
}

int CatalogFilterModel::minRating() const
{
    return m_minRating;
}

void CatalogFilterModel::setMinRating(int rating)
{
    if (m_minRating == rating)
    {
        return;
    }
    m_minRating = rating;
    emit minRatingChanged();
    invalidateFilter();
    emit countChanged();
}

QString CatalogFilterModel::sortCriterion() const
{
    return m_sortCriterion;
}

void CatalogFilterModel::setSortCriterion(const QString &criterion)
{
    if (m_sortCriterion == criterion)
    {
        return;
    }
    m_sortCriterion = criterion.toLower();
    emit sortCriterionChanged();
    updateSorting();
}

bool CatalogFilterModel::sortAscending() const
{
    return m_sortAscending;
}

void CatalogFilterModel::setSortAscending(bool ascending)
{
    if (m_sortAscending == ascending)
    {
        return;
    }
    m_sortAscending = ascending;
    emit sortAscendingChanged();
    updateSorting();
}

int CatalogFilterModel::count() const
{
    return rowCount();
}

int CatalogFilterModel::mapToSourceRow(int proxyRow) const
{
    const QModelIndex proxyIdx = index(proxyRow, 0);
    const QModelIndex srcIdx = mapToSource(proxyIdx);
    return srcIdx.isValid() ? srcIdx.row() : -1;
}

int CatalogFilterModel::mapFromSourceRow(int sourceRow) const
{
    if (!sourceModel())
    {
        return -1;
    }
    const QModelIndex srcIdx = sourceModel()->index(sourceRow, 0);
    const QModelIndex proxyIdx = mapFromSource(srcIdx);
    return proxyIdx.isValid() ? proxyIdx.row() : -1;
}

QVariantMap CatalogFilterModel::get(int proxyRow) const
{
    const QModelIndex idx = index(proxyRow, 0);
    if (!idx.isValid())
    {
        return {};
    }
    return idx.data(CatalogModel::ItemMapRole).toMap();
}

void CatalogFilterModel::updateSorting()
{
    const Qt::SortOrder order = m_sortAscending ? Qt::AscendingOrder : Qt::DescendingOrder;
    sort(0, order);
}

bool CatalogFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!sourceModel())
    {
        return false;
    }

    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!idx.isValid())
    {
        return false;
    }

    // 1. Min rating filter
    if (m_minRating > 0)
    {
        const int rating = idx.data(CatalogModel::RatingRole).toInt();
        if (rating < m_minRating)
        {
            return false;
        }
    }

    // 2. Category filter
    if (m_categoryFilter == QStringLiteral("demo"))
    {
        const bool isDemo = idx.data(CatalogModel::IsDemoFixtureRole).toBool();
        if (!isDemo)
        {
            return false;
        }
    }
    else if (m_categoryFilter == QStringLiteral("imported"))
    {
        const bool isDemo = idx.data(CatalogModel::IsDemoFixtureRole).toBool();
        if (isDemo)
        {
            return false;
        }
    }
    else if (m_categoryFilter == QStringLiteral("favorites"))
    {
        const int rating = idx.data(CatalogModel::RatingRole).toInt();
        const bool isFlagged = idx.data(CatalogModel::IsFlaggedRole).toBool();
        if (rating < 4 && !isFlagged)
        {
            return false;
        }
    }

    // 3. Tag filter
    if (!m_tagFilter.isEmpty())
    {
        const QStringList tags = idx.data(CatalogModel::TagsRole).toStringList();
        bool tagMatched = false;
        for (const auto &t : tags)
        {
            if (t.compare(m_tagFilter, Qt::CaseInsensitive) == 0)
            {
                tagMatched = true;
                break;
            }
        }
        if (!tagMatched)
        {
            return false;
        }
    }

    // 4. Search query filter
    if (!m_searchQuery.trimmed().isEmpty())
    {
        const QString query = m_searchQuery.trimmed();
        const QString fileName = idx.data(CatalogModel::FileNameRole).toString();
        const QString title = idx.data(CatalogModel::TitleRole).toString();
        const QString camera = idx.data(CatalogModel::CameraModelRole).toString();
        const QStringList tags = idx.data(CatalogModel::TagsRole).toStringList();

        if (fileName.contains(query, Qt::CaseInsensitive) ||
            title.contains(query, Qt::CaseInsensitive) ||
            camera.contains(query, Qt::CaseInsensitive))
        {
            return true;
        }

        for (const auto &t : tags)
        {
            if (t.contains(query, Qt::CaseInsensitive))
            {
                return true;
            }
        }

        return false;
    }

    return true;
}

bool CatalogFilterModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
    if (m_sortCriterion == QStringLiteral("name"))
    {
        const QString left = sourceLeft.data(CatalogModel::FileNameRole).toString();
        const QString right = sourceRight.data(CatalogModel::FileNameRole).toString();
        return QString::compare(left, right, Qt::CaseInsensitive) < 0;
    }
    if (m_sortCriterion == QStringLiteral("size"))
    {
        const qint64 left = sourceLeft.data(CatalogModel::FileSizeRole).toLongLong();
        const qint64 right = sourceRight.data(CatalogModel::FileSizeRole).toLongLong();
        return left < right;
    }
    if (m_sortCriterion == QStringLiteral("rating"))
    {
        const int left = sourceLeft.data(CatalogModel::RatingRole).toInt();
        const int right = sourceRight.data(CatalogModel::RatingRole).toInt();
        return left < right;
    }

    // Default sort criterion: date
    const QDateTime left = sourceLeft.data(CatalogModel::DateTakenRole).toDateTime();
    const QDateTime right = sourceRight.data(CatalogModel::DateTakenRole).toDateTime();
    return left < right;
}

} // namespace PhotoCarta
