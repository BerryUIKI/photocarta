// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "database/catalogdatabase.h"

#include <QAbstractListModel>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace PhotoCarta
{

class CollectionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int activeCollectionId READ activeCollectionId WRITE setActiveCollectionId NOTIFY activeCollectionIdChanged)

public:
    enum CollectionRoles
    {
        IdRole = Qt::UserRole + 1,
        NameRole,
        RootPathRole,
        CollectionTypeRole,
        IsAvailableRole,
        ItemCountRole,
        LastScannedAtRole,
        IsActiveRole,
        RecordMapRole
    };
    Q_ENUM(CollectionRoles)

    explicit CollectionModel(QObject *parent = nullptr);
    ~CollectionModel() override = default;

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int count() const;
    [[nodiscard]] int activeCollectionId() const;
    void setActiveCollectionId(int id);

    void setDatabase(CatalogDatabase *db);
    [[nodiscard]] CatalogDatabase *database() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE [[nodiscard]] QVariantMap get(int row) const;
    Q_INVOKABLE [[nodiscard]] int findCollectionIndex(int collectionId) const;
    Q_INVOKABLE bool toggleAvailability(int collectionId);
    Q_INVOKABLE bool removeCollection(int collectionId);
    Q_INVOKABLE bool openInFileManager(int collectionId) const;

signals:
    void countChanged();
    void activeCollectionIdChanged();
    void collectionAvailabilityChanged(int collectionId, bool isAvailable);
    void collectionRemoved(int collectionId);

private:
    CatalogDatabase *m_database = nullptr;
    QList<CollectionRecord> m_collections;
    int m_activeCollectionId = 0;
};

} // namespace PhotoCarta
