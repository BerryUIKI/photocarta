// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "catalogitem.h"

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

namespace PhotoCarta
{

struct CollectionRecord
{
    int id = 0;
    QString rootPath;
    QString name;
    QString collectionType = QStringLiteral("local"); // local, removable, network
    bool isAvailable = true;
    QDateTime createdAt;
    QDateTime lastScannedAt;
    int itemCount = 0;
};

class CatalogDatabase : public QObject
{
    Q_OBJECT

public:
    explicit CatalogDatabase(const QString &connectionName = QString(), QObject *parent = nullptr);
    ~CatalogDatabase() override;

    [[nodiscard]] static QString defaultDatabasePath();

    bool open(const QString &databasePath = QString());
    void close();
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] QString databasePath() const;
    [[nodiscard]] QString lastError() const;

    // Collection management
    int addOrGetCollection(const QString &rootPath,
                           const QString &name = QString(),
                           const QString &type = QStringLiteral("local"));
    [[nodiscard]] QList<CollectionRecord> getCollections() const;
    [[nodiscard]] CollectionRecord getCollectionByPath(const QString &rootPath) const;
    bool setCollectionAvailable(const QString &rootPath, bool available);
    bool updateCollectionScanTime(int collectionId);
    bool removeCollection(int collectionId);

    // Item management
    bool insertOrUpdateItems(const QList<CatalogItem> &items, int collectionId = 0);
    [[nodiscard]] QList<CatalogItem> fetchAllItems() const;
    [[nodiscard]] QList<CatalogItem> fetchItemsByCollection(int collectionId) const;
    [[nodiscard]] CatalogItem fetchItemByPath(const QString &filePath) const;
    [[nodiscard]] int itemCount() const;

    // Metadata persistence
    bool updateItemRating(const QString &filePath, int rating);
    bool updateItemFlagged(const QString &filePath, bool flagged);
    bool setItemTags(const QString &filePath, const QStringList &tags);
    [[nodiscard]] QStringList allTags() const;

signals:
    void databaseOpened(const QString &path);
    void databaseClosed();
    void itemsChanged();
    void collectionsChanged();

private:
    bool createTables();
    bool executePragmas();

    QString m_connectionName;
    QString m_databasePath;
    QString m_lastError;
};

} // namespace PhotoCarta
