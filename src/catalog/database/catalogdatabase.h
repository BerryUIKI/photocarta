// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "catalogitem.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

namespace PhotoCarta
{

class DigikamDatabaseAdapter;

enum class DatabaseSchemaType
{
    PhotoCarta,
    Digikam,
    Unknown
};

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

    // Schema detection & digiKam adapter
    [[nodiscard]] DatabaseSchemaType schemaType() const;
    [[nodiscard]] bool isDigikamDatabase() const;
    [[nodiscard]] DigikamDatabaseAdapter *digikamAdapter() const;
    void setDigikamRootPathMapping(int rootId, const QString &localPath);

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
    [[nodiscard]] QList<CatalogItem> fetchItemsPaged(int offset, int limit, int collectionId = 0) const;
    [[nodiscard]] CatalogItem fetchItemByPath(const QString &filePath) const;
    [[nodiscard]] int itemCount() const;

    // Metadata persistence
    bool updateItemRating(const QString &filePath, int rating);
    bool updateItemFlagged(const QString &filePath, bool flagged);
    bool setItemTags(const QString &filePath, const QStringList &tags);
    [[nodiscard]] QStringList allTags() const;

    // Migration / Synchronization
    bool importFromDigikamDatabase(const QString &digikamDatabasePath);

signals:
    void databaseOpened(const QString &path);
    void databaseClosed();
    void itemsChanged();
    void collectionsChanged();

private:
    bool createTables();
    bool executePragmas();
    void loadTagCache() const;

    QString m_connectionName;
    QString m_databasePath;
    QString m_lastError;
    DatabaseSchemaType m_schemaType = DatabaseSchemaType::PhotoCarta;
    DigikamDatabaseAdapter *m_digikamAdapter = nullptr;
    mutable QHash<QString, int> m_tagCache;
};

} // namespace PhotoCarta
