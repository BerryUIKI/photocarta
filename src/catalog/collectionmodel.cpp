// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "collectionmodel.h"

#include <QDesktopServices>
#include <QUrl>

namespace PhotoCarta
{

CollectionModel::CollectionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CollectionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(m_collections.size());
}

QVariant CollectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_collections.size())
    {
        return {};
    }

    const CollectionRecord &rec = m_collections.at(index.row());

    switch (role)
    {
    case IdRole:
        return rec.id;
    case NameRole:
    case Qt::DisplayRole:
        return rec.name;
    case RootPathRole:
        return rec.rootPath;
    case CollectionTypeRole:
        return rec.collectionType;
    case IsAvailableRole:
        return rec.isAvailable;
    case ItemCountRole:
        return rec.itemCount;
    case LastScannedAtRole:
        return rec.lastScannedAt.isValid()
                   ? rec.lastScannedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm"))
                   : QString();
    case IsActiveRole:
        return rec.id == m_activeCollectionId;
    case RecordMapRole:
        return get(index.row());
    default:
        return {};
    }
}

QHash<int, QByteArray> CollectionModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {NameRole, "name"},
        {RootPathRole, "rootPath"},
        {CollectionTypeRole, "collectionType"},
        {IsAvailableRole, "isAvailable"},
        {ItemCountRole, "itemCount"},
        {LastScannedAtRole, "lastScannedAt"},
        {IsActiveRole, "isActive"},
        {RecordMapRole, "recordMap"}
    };
}

int CollectionModel::count() const
{
    return static_cast<int>(m_collections.size());
}

int CollectionModel::activeCollectionId() const
{
    return m_activeCollectionId;
}

void CollectionModel::setActiveCollectionId(int id)
{
    if (m_activeCollectionId == id)
    {
        return;
    }
    m_activeCollectionId = id;
    emit activeCollectionIdChanged();

    if (!m_collections.isEmpty())
    {
        emit dataChanged(index(0, 0), index(m_collections.size() - 1, 0), {IsActiveRole});
    }
}

void CollectionModel::setDatabase(CatalogDatabase *db)
{
    if (m_database == db)
    {
        return;
    }

    if (m_database)
    {
        disconnect(m_database, &CatalogDatabase::collectionsChanged, this, &CollectionModel::reload);
    }

    m_database = db;

    if (m_database)
    {
        connect(m_database, &CatalogDatabase::collectionsChanged, this, &CollectionModel::reload);
    }

    reload();
}

CatalogDatabase *CollectionModel::database() const
{
    return m_database;
}

void CollectionModel::reload()
{
    beginResetModel();
    if (m_database && m_database->isOpen())
    {
        m_collections = m_database->getCollections();
    }
    else
    {
        m_collections.clear();
    }
    endResetModel();
    emit countChanged();
}

QVariantMap CollectionModel::get(int row) const
{
    if (row < 0 || row >= m_collections.size())
    {
        return {};
    }

    const CollectionRecord &rec = m_collections.at(row);
    QVariantMap map;
    map.insert(QStringLiteral("id"), rec.id);
    map.insert(QStringLiteral("name"), rec.name);
    map.insert(QStringLiteral("rootPath"), rec.rootPath);
    map.insert(QStringLiteral("collectionType"), rec.collectionType);
    map.insert(QStringLiteral("isAvailable"), rec.isAvailable);
    map.insert(QStringLiteral("itemCount"), rec.itemCount);
    map.insert(QStringLiteral("lastScannedAt"),
               rec.lastScannedAt.isValid() ? rec.lastScannedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm")) : QString());
    map.insert(QStringLiteral("isActive"), rec.id == m_activeCollectionId);
    return map;
}

int CollectionModel::findCollectionIndex(int collectionId) const
{
    for (int i = 0; i < m_collections.size(); ++i)
    {
        if (m_collections.at(i).id == collectionId)
        {
            return i;
        }
    }
    return -1;
}

bool CollectionModel::toggleAvailability(int collectionId)
{
    if (!m_database || !m_database->isOpen())
    {
        return false;
    }

    const int idx = findCollectionIndex(collectionId);
    if (idx < 0)
    {
        return false;
    }

    const CollectionRecord &rec = m_collections.at(idx);
    const bool newAvailable = !rec.isAvailable;
    const bool ok = m_database->setCollectionAvailable(rec.rootPath, newAvailable);
    if (ok)
    {
        emit collectionAvailabilityChanged(collectionId, newAvailable);
    }
    return ok;
}

bool CollectionModel::removeCollection(int collectionId)
{
    if (!m_database || !m_database->isOpen())
    {
        return false;
    }

    if (m_activeCollectionId == collectionId)
    {
        setActiveCollectionId(0);
    }

    const bool ok = m_database->removeCollection(collectionId);
    if (ok)
    {
        emit collectionRemoved(collectionId);
    }
    return ok;
}

bool CollectionModel::openInFileManager(int collectionId) const
{
    const int idx = findCollectionIndex(collectionId);
    if (idx < 0)
    {
        return false;
    }

    const QString path = m_collections.at(idx).rootPath;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

} // namespace PhotoCarta
