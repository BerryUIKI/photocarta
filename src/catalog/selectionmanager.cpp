// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "selectionmanager.h"
#include "catalogmodel.h"

namespace PhotoCarta
{

SelectionManager::SelectionManager(CatalogFilterModel *filterModel, QObject *parent)
    : QObject(parent)
    , m_filterModel(filterModel)
{
    if (m_filterModel)
    {
        connect(m_filterModel, &QAbstractItemModel::modelReset, this, &SelectionManager::onModelResetOrChanged);
        connect(m_filterModel, &QAbstractItemModel::layoutChanged, this, &SelectionManager::onModelResetOrChanged);
        connect(m_filterModel, &QAbstractItemModel::rowsRemoved, this, &SelectionManager::onModelResetOrChanged);
        connect(m_filterModel, &QAbstractItemModel::rowsInserted, this, &SelectionManager::onModelResetOrChanged);
        connect(m_filterModel, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
            const int curRow = currentIndex();
            if (curRow >= topLeft.row() && curRow <= bottomRight.row()) {
                updateSelectedItem();
                emit selectionChanged();
            }
        });
    }
}

int SelectionManager::currentIndex() const
{
    return findRowForId(m_currentId);
}

void SelectionManager::setCurrentIndex(int index)
{
    select(index);
}

QString SelectionManager::currentId() const
{
    return m_currentId;
}

const QSet<QString> &SelectionManager::selectedIds() const
{
    return m_selectedIds;
}

bool SelectionManager::hasSelection() const
{
    return !m_selectedIds.isEmpty();
}

int SelectionManager::selectedCount() const
{
    return static_cast<int>(m_selectedIds.size());
}

QVariantMap SelectionManager::selectedItem() const
{
    return m_selectedItem;
}

bool SelectionManager::isSelected(int index) const
{
    const QString id = idForRow(index);
    return !id.isEmpty() && m_selectedIds.contains(id);
}

bool SelectionManager::isSelectedId(const QString &id) const
{
    return !id.isEmpty() && m_selectedIds.contains(id);
}

void SelectionManager::selectById(const QString &id)
{
    if (id.isEmpty())
    {
        clearSelection();
        return;
    }

    m_selectedIds.clear();
    m_selectedIds.insert(id);
    m_currentId = id;
    m_anchorId = id;
    updateSelectedItem();
    emit selectionChanged();
}

void SelectionManager::select(int index)
{
    if (!m_filterModel)
    {
        return;
    }

    const int total = m_filterModel->rowCount();
    if (total == 0 || index < 0)
    {
        clearSelection();
        return;
    }

    if (index >= total)
    {
        index = total - 1;
    }

    const QString id = idForRow(index);
    selectById(id);
}

void SelectionManager::toggleSelectById(const QString &id)
{
    if (id.isEmpty())
    {
        return;
    }

    m_anchorId = id;

    if (m_selectedIds.contains(id))
    {
        m_selectedIds.remove(id);
        if (m_currentId == id)
        {
            if (m_selectedIds.isEmpty())
            {
                m_currentId.clear();
            }
            else
            {
                QString fallback;
                for (const auto &selId : m_selectedIds)
                {
                    if (findRowForId(selId) >= 0)
                    {
                        fallback = selId;
                        break;
                    }
                }
                m_currentId = fallback.isEmpty() ? *m_selectedIds.begin() : fallback;
            }
        }
    }
    else
    {
        m_selectedIds.insert(id);
        m_currentId = id;
    }

    updateSelectedItem();
    emit selectionChanged();
}

void SelectionManager::toggleSelect(int index)
{
    if (!m_filterModel)
    {
        return;
    }

    const int total = m_filterModel->rowCount();
    if (index < 0 || index >= total)
    {
        return;
    }

    const QString id = idForRow(index);
    if (!id.isEmpty())
    {
        toggleSelectById(id);
    }
}

void SelectionManager::selectRange(int toIndex)
{
    if (!m_filterModel)
    {
        return;
    }

    const int total = m_filterModel->rowCount();
    if (total == 0)
    {
        clearSelection();
        return;
    }

    int anchor = findRowForId(m_anchorId);
    if (anchor < 0)
    {
        anchor = currentIndex();
    }
    if (anchor < 0)
    {
        anchor = 0;
    }

    toIndex = std::clamp(toIndex, 0, total - 1);
    const int start = std::min(anchor, toIndex);
    const int end = std::max(anchor, toIndex);

    m_selectedIds.clear();
    for (int i = start; i <= end; ++i)
    {
        const QString id = idForRow(i);
        if (!id.isEmpty())
        {
            m_selectedIds.insert(id);
        }
    }

    m_currentId = idForRow(toIndex);
    updateSelectedItem();
    emit selectionChanged();
}

QList<int> SelectionManager::selectedProxyRows() const
{
    QList<int> rows;
    if (!m_filterModel || m_selectedIds.isEmpty())
    {
        return rows;
    }

    const int count = m_filterModel->rowCount();
    for (int i = 0; i < count; ++i)
    {
        const QString id = idForRow(i);
        if (m_selectedIds.contains(id))
        {
            rows.append(i);
        }
    }
    return rows;
}

QStringList SelectionManager::commonTags() const
{
    if (!m_filterModel || m_selectedIds.isEmpty())
    {
        return {};
    }

    QSet<QString> common;
    bool first = true;

    const int count = m_filterModel->rowCount();
    for (int i = 0; i < count; ++i)
    {
        const QString id = idForRow(i);
        if (m_selectedIds.contains(id))
        {
            const QModelIndex idx = m_filterModel->index(i, 0);
            const QStringList tags = idx.data(CatalogModel::TagsRole).toStringList();
            QSet<QString> itemTags;
            for (const auto &t : tags)
            {
                const QString trimmed = t.trimmed();
                if (!trimmed.isEmpty())
                {
                    itemTags.insert(trimmed);
                }
            }

            if (first)
            {
                common = itemTags;
                first = false;
            }
            else
            {
                common.intersect(itemTags);
            }

            if (common.isEmpty())
            {
                break;
            }
        }
    }

    QStringList list = common.values();
    list.sort(Qt::CaseInsensitive);
    return list;
}

QStringList SelectionManager::allSelectedTags() const
{
    if (!m_filterModel || m_selectedIds.isEmpty())
    {
        return {};
    }

    QSet<QString> unionTags;
    const int count = m_filterModel->rowCount();
    for (int i = 0; i < count; ++i)
    {
        const QString id = idForRow(i);
        if (m_selectedIds.contains(id))
        {
            const QModelIndex idx = m_filterModel->index(i, 0);
            const QStringList tags = idx.data(CatalogModel::TagsRole).toStringList();
            for (const auto &t : tags)
            {
                const QString trimmed = t.trimmed();
                if (!trimmed.isEmpty())
                {
                    unionTags.insert(trimmed);
                }
            }
        }
    }

    QStringList list = unionTags.values();
    list.sort(Qt::CaseInsensitive);
    return list;
}

QVariantList SelectionManager::selectedItemsSummary(int maxItems) const
{
    QVariantList list;
    if (!m_filterModel || m_selectedIds.isEmpty())
    {
        return list;
    }

    int added = 0;
    const int count = m_filterModel->rowCount();
    for (int i = 0; i < count && added < maxItems; ++i)
    {
        const QString id = idForRow(i);
        if (m_selectedIds.contains(id))
        {
            const QModelIndex idx = m_filterModel->index(i, 0);
            QVariantMap itemSummary;
            itemSummary[QStringLiteral("id")] = id;
            itemSummary[QStringLiteral("thumbnailUrl")] = idx.data(CatalogModel::ThumbnailUrlRole);
            itemSummary[QStringLiteral("fileName")] = idx.data(CatalogModel::FileNameRole);
            itemSummary[QStringLiteral("title")] = idx.data(CatalogModel::TitleRole);
            list.append(itemSummary);
            ++added;
        }
    }
    return list;
}

void SelectionManager::selectAll()
{
    if (!m_filterModel)
    {
        return;
    }

    const int total = m_filterModel->rowCount();
    m_selectedIds.clear();
    for (int i = 0; i < total; ++i)
    {
        const QString id = idForRow(i);
        if (!id.isEmpty())
        {
            m_selectedIds.insert(id);
        }
    }

    if (total > 0 && findRowForId(m_currentId) < 0)
    {
        m_currentId = idForRow(0);
    }

    updateSelectedItem();
    emit selectionChanged();
}

void SelectionManager::clearSelection()
{
    if (m_currentId.isEmpty() && m_selectedIds.isEmpty() && m_selectedItem.isEmpty())
    {
        return;
    }

    m_currentId.clear();
    m_selectedIds.clear();
    m_selectedItem.clear();
    emit selectionChanged();
}

void SelectionManager::selectNext()
{
    if (!m_filterModel)
    {
        return;
    }
    const int total = m_filterModel->rowCount();
    if (total == 0)
    {
        return;
    }
    const int current = currentIndex();
    if (current < 0)
    {
        select(0);
    }
    else if (current + 1 < total)
    {
        select(current + 1);
    }
}

void SelectionManager::selectPrevious()
{
    if (!m_filterModel)
    {
        return;
    }
    const int total = m_filterModel->rowCount();
    if (total == 0)
    {
        return;
    }
    const int current = currentIndex();
    if (current <= 0)
    {
        select(0);
    }
    else
    {
        select(current - 1);
    }
}

void SelectionManager::selectFirst()
{
    if (!m_filterModel || m_filterModel->rowCount() == 0)
    {
        return;
    }
    select(0);
}

void SelectionManager::selectLast()
{
    if (!m_filterModel)
    {
        return;
    }
    const int total = m_filterModel->rowCount();
    if (total > 0)
    {
        select(total - 1);
    }
}

void SelectionManager::onModelResetOrChanged()
{
    if (!m_filterModel)
    {
        clearSelection();
        return;
    }

    const int total = m_filterModel->rowCount();
    if (total == 0)
    {
        if (!m_filterModel->sourceModel() || m_filterModel->sourceModel()->rowCount() == 0)
        {
            clearSelection();
            return;
        }
        updateSelectedItem();
        emit selectionChanged();
        return;
    }

    if (m_currentId.isEmpty() && m_selectedIds.isEmpty())
    {
        const QString firstId = idForRow(0);
        if (!firstId.isEmpty())
        {
            m_currentId = firstId;
            m_selectedIds.insert(firstId);
        }
    }
    else
    {
        const int currentRow = findRowForId(m_currentId);
        if (currentRow < 0)
        {
            QString fallback;
            for (const auto &selId : m_selectedIds)
            {
                if (findRowForId(selId) >= 0)
                {
                    fallback = selId;
                    break;
                }
            }
            if (!fallback.isEmpty())
            {
                m_currentId = fallback;
            }
        }
    }

    if (const auto *catModel = qobject_cast<const CatalogModel *>(m_filterModel->sourceModel()))
    {
        QSet<QString> validIds;
        for (const auto &item : catModel->items())
        {
            validIds.insert(item.id);
        }
        const auto copySel = m_selectedIds;
        for (const auto &selId : copySel)
        {
            if (!validIds.contains(selId))
            {
                m_selectedIds.remove(selId);
            }
        }
        if (!validIds.contains(m_currentId))
        {
            m_currentId.clear();
            if (!m_selectedIds.isEmpty())
            {
                m_currentId = *m_selectedIds.begin();
            }
        }
    }

    updateSelectedItem();
    emit selectionChanged();
}

int SelectionManager::findRowForId(const QString &id) const
{
    if (!m_filterModel || id.isEmpty())
    {
        return -1;
    }
    const int total = m_filterModel->rowCount();
    for (int i = 0; i < total; ++i)
    {
        const QModelIndex idx = m_filterModel->index(i, 0);
        if (m_filterModel->data(idx, CatalogModel::IdRole).toString() == id)
        {
            return i;
        }
    }
    return -1;
}

QString SelectionManager::idForRow(int row) const
{
    if (!m_filterModel || row < 0 || row >= m_filterModel->rowCount())
    {
        return {};
    }
    const QModelIndex idx = m_filterModel->index(row, 0);
    return m_filterModel->data(idx, CatalogModel::IdRole).toString();
}

void SelectionManager::updateSelectedItem()
{
    const int row = findRowForId(m_currentId);
    if (m_filterModel && row >= 0 && row < m_filterModel->rowCount())
    {
        m_selectedItem = m_filterModel->get(row);
    }
    else
    {
        m_selectedItem.clear();
    }
}

} // namespace PhotoCarta
