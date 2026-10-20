// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalogdatabase.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUrl>

namespace
{
Q_LOGGING_CATEGORY(lcDb, "photocarta.database")
}

namespace PhotoCarta
{

CatalogDatabase::CatalogDatabase(const QString &connectionName, QObject *parent)
    : QObject(parent),
      m_connectionName(connectionName.isEmpty()
                           ? QString::asprintf("photocarta_db_%p", static_cast<const void *>(this))
                           : connectionName)
{
}

CatalogDatabase::~CatalogDatabase()
{
    close();
}

QString CatalogDatabase::defaultDatabasePath()
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    return QDir(appData).filePath(QStringLiteral("photocarta.db"));
}

bool CatalogDatabase::open(const QString &databasePath)
{
    if (isOpen())
    {
        close();
    }

    m_databasePath = databasePath.isEmpty() ? defaultDatabasePath() : databasePath;

    if (m_databasePath != QStringLiteral(":memory:"))
    {
        const QFileInfo fi(m_databasePath);
        QDir().mkpath(fi.absolutePath());
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_databasePath);

    if (!db.open())
    {
        m_lastError = db.lastError().text();
        qCCritical(lcDb) << "Failed to open database at" << m_databasePath << ":" << m_lastError;
        QSqlDatabase::removeDatabase(m_connectionName);
        return false;
    }

    if (!executePragmas() || !createTables())
    {
        close();
        return false;
    }

    qCInfo(lcDb) << "Connected to SQLite catalog at" << m_databasePath;
    emit databaseOpened(m_databasePath);
    return true;
}

void CatalogDatabase::close()
{
    if (QSqlDatabase::contains(m_connectionName))
    {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName);
            if (db.isOpen())
            {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(m_connectionName);
        emit databaseClosed();
    }
    m_databasePath.clear();
}

bool CatalogDatabase::isOpen() const
{
    if (!QSqlDatabase::contains(m_connectionName))
    {
        return false;
    }
    return QSqlDatabase::database(m_connectionName).isOpen();
}

QString CatalogDatabase::databasePath() const
{
    return m_databasePath;
}

QString CatalogDatabase::lastError() const
{
    return m_lastError;
}

bool CatalogDatabase::executePragmas()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    const QStringList pragmas = {
        QStringLiteral("PRAGMA journal_mode = WAL;"),
        QStringLiteral("PRAGMA synchronous = NORMAL;"),
        QStringLiteral("PRAGMA foreign_keys = ON;"),
    };

    for (const QString &pragma : pragmas)
    {
        if (!query.exec(pragma))
        {
            m_lastError = query.lastError().text();
            qCWarning(lcDb) << "Pragma execution failed:" << pragma << ":" << m_lastError;
        }
    }
    return true;
}

bool CatalogDatabase::createTables()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    const QString schemaSql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS collections ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  root_path TEXT UNIQUE NOT NULL,"
        "  name TEXT NOT NULL,"
        "  collection_type TEXT DEFAULT 'local',"
        "  is_available INTEGER DEFAULT 1,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  last_scanned_at DATETIME"
        ");"

        "CREATE TABLE IF NOT EXISTS items ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  collection_id INTEGER REFERENCES collections(id) ON DELETE SET NULL,"
        "  file_path TEXT UNIQUE NOT NULL,"
        "  file_name TEXT NOT NULL,"
        "  title TEXT,"
        "  file_size INTEGER NOT NULL,"
        "  mime_type TEXT,"
        "  date_taken DATETIME,"
        "  date_modified DATETIME NOT NULL,"
        "  width INTEGER DEFAULT 0,"
        "  height INTEGER DEFAULT 0,"
        "  camera_model TEXT,"
        "  lens_model TEXT,"
        "  focal_length TEXT,"
        "  aperture TEXT,"
        "  shutter_speed TEXT,"
        "  iso INTEGER DEFAULT 0,"
        "  rating INTEGER DEFAULT 0,"
        "  is_flagged INTEGER DEFAULT 0,"
        "  is_missing INTEGER DEFAULT 0"
        ");"

        "CREATE TABLE IF NOT EXISTS tags ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT UNIQUE NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS item_tags ("
        "  item_id INTEGER REFERENCES items(id) ON DELETE CASCADE,"
        "  tag_id INTEGER REFERENCES tags(id) ON DELETE CASCADE,"
        "  PRIMARY KEY (item_id, tag_id)"
        ");"

        "CREATE INDEX IF NOT EXISTS idx_items_collection ON items(collection_id);"
        "CREATE INDEX IF NOT EXISTS idx_items_date_taken ON items(date_taken);"
        "CREATE INDEX IF NOT EXISTS idx_items_rating ON items(rating);"
        "CREATE INDEX IF NOT EXISTS idx_items_file_path ON items(file_path);"
        "CREATE INDEX IF NOT EXISTS idx_collections_root ON collections(root_path);"
    );

    const QStringList statements = schemaSql.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &stmt : statements)
    {
        const QString trimmed = stmt.trimmed();
        if (trimmed.isEmpty())
        {
            continue;
        }
        if (!query.exec(trimmed))
        {
            m_lastError = query.lastError().text();
            qCCritical(lcDb) << "Failed to create schema:" << trimmed << ":" << m_lastError;
            return false;
        }
    }
    return true;
}

int CatalogDatabase::addOrGetCollection(const QString &rootPath, const QString &name, const QString &type)
{
    if (!isOpen())
    {
        return 0;
    }

    const QString normalizedPath = QDir::fromNativeSeparators(rootPath);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    // Check if collection already exists
    QSqlQuery checkQuery(db);
    checkQuery.prepare(QStringLiteral("SELECT id FROM collections WHERE root_path = :path;"));
    checkQuery.bindValue(QStringLiteral(":path"), normalizedPath);
    if (checkQuery.exec() && checkQuery.next())
    {
        return checkQuery.value(0).toInt();
    }

    // Insert new collection
    const QString collectionName = name.isEmpty() ? QFileInfo(normalizedPath).fileName() : name;
    QSqlQuery insertQuery(db);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO collections (root_path, name, collection_type, is_available) "
        "VALUES (:path, :name, :type, 1);"
    ));
    insertQuery.bindValue(QStringLiteral(":path"), normalizedPath);
    insertQuery.bindValue(QStringLiteral(":name"), collectionName);
    insertQuery.bindValue(QStringLiteral(":type"), type);

    if (!insertQuery.exec())
    {
        m_lastError = insertQuery.lastError().text();
        qCWarning(lcDb) << "Failed to add collection:" << m_lastError;
        return 0;
    }

    emit collectionsChanged();
    return insertQuery.lastInsertId().toInt();
}

QList<CollectionRecord> CatalogDatabase::getCollections() const
{
    QList<CollectionRecord> list;
    if (!isOpen())
    {
        return list;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(QStringLiteral(
        "SELECT c.id, c.root_path, c.name, c.collection_type, c.is_available, c.created_at, c.last_scanned_at, COUNT(i.id) "
        "FROM collections c "
        "LEFT JOIN items i ON c.id = i.collection_id "
        "GROUP BY c.id, c.root_path, c.name, c.collection_type, c.is_available, c.created_at, c.last_scanned_at "
        "ORDER BY c.name ASC;"
    ), db);

    while (query.next())
    {
        CollectionRecord rec;
        rec.id = query.value(0).toInt();
        rec.rootPath = query.value(1).toString();
        rec.name = query.value(2).toString();
        rec.collectionType = query.value(3).toString();
        rec.isAvailable = query.value(4).toBool();
        rec.createdAt = query.value(5).toDateTime();
        rec.lastScannedAt = query.value(6).toDateTime();
        rec.itemCount = query.value(7).toInt();
        list.append(rec);
    }
    return list;
}

CollectionRecord CatalogDatabase::getCollectionByPath(const QString &rootPath) const
{
    CollectionRecord rec;
    if (!isOpen())
    {
        return rec;
    }

    const QString normalized = QDir::fromNativeSeparators(rootPath);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT c.id, c.root_path, c.name, c.collection_type, c.is_available, c.created_at, c.last_scanned_at, COUNT(i.id) "
        "FROM collections c "
        "LEFT JOIN items i ON c.id = i.collection_id "
        "WHERE c.root_path = :path "
        "GROUP BY c.id, c.root_path, c.name, c.collection_type, c.is_available, c.created_at, c.last_scanned_at "
        "LIMIT 1;"
    ));
    query.bindValue(QStringLiteral(":path"), normalized);

    if (query.exec() && query.next())
    {
        rec.id = query.value(0).toInt();
        rec.rootPath = query.value(1).toString();
        rec.name = query.value(2).toString();
        rec.collectionType = query.value(3).toString();
        rec.isAvailable = query.value(4).toBool();
        rec.createdAt = query.value(5).toDateTime();
        rec.lastScannedAt = query.value(6).toDateTime();
        rec.itemCount = query.value(7).toInt();
    }
    return rec;
}

bool CatalogDatabase::setCollectionAvailable(const QString &rootPath, bool available)
{
    if (!isOpen())
    {
        return false;
    }

    const QString normalized = QDir::fromNativeSeparators(rootPath);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    db.transaction();

    QSqlQuery collQuery(db);
    collQuery.prepare(QStringLiteral("UPDATE collections SET is_available = :avail WHERE root_path = :path;"));
    collQuery.bindValue(QStringLiteral(":avail"), available ? 1 : 0);
    collQuery.bindValue(QStringLiteral(":path"), normalized);
    if (!collQuery.exec())
    {
        m_lastError = collQuery.lastError().text();
        db.rollback();
        return false;
    }

    // Also mark contained items as missing/re-available without deleting records
    QSqlQuery itemQuery(db);
    itemQuery.prepare(QStringLiteral(
        "UPDATE items SET is_missing = :missing "
        "WHERE collection_id IN (SELECT id FROM collections WHERE root_path = :path);"
    ));
    itemQuery.bindValue(QStringLiteral(":missing"), available ? 0 : 1);
    itemQuery.bindValue(QStringLiteral(":path"), normalized);
    if (!itemQuery.exec())
    {
        m_lastError = itemQuery.lastError().text();
        db.rollback();
        return false;
    }

    db.commit();
    emit collectionsChanged();
    emit itemsChanged();
    return true;
}

bool CatalogDatabase::updateCollectionScanTime(int collectionId)
{
    if (!isOpen() || collectionId <= 0)
    {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE collections SET last_scanned_at = CURRENT_TIMESTAMP WHERE id = :id;"));
    query.bindValue(QStringLiteral(":id"), collectionId);
    return query.exec();
}

bool CatalogDatabase::removeCollection(int collectionId)
{
    if (!isOpen() || collectionId <= 0)
    {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery itemQuery(db);
    itemQuery.prepare(QStringLiteral("DELETE FROM items WHERE collection_id = :id;"));
    itemQuery.bindValue(QStringLiteral(":id"), collectionId);
    if (!itemQuery.exec())
    {
        m_lastError = itemQuery.lastError().text();
        db.rollback();
        return false;
    }

    QSqlQuery collQuery(db);
    collQuery.prepare(QStringLiteral("DELETE FROM collections WHERE id = :id;"));
    collQuery.bindValue(QStringLiteral(":id"), collectionId);
    if (!collQuery.exec())
    {
        m_lastError = collQuery.lastError().text();
        db.rollback();
        return false;
    }

    db.commit();
    emit collectionsChanged();
    emit itemsChanged();
    return true;
}

bool CatalogDatabase::insertOrUpdateItems(const QList<CatalogItem> &items, int collectionId)
{
    if (!isOpen() || items.isEmpty())
    {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery itemQuery(db);
    itemQuery.prepare(QStringLiteral(
        "INSERT INTO items ("
        "  collection_id, file_path, file_name, title, file_size, mime_type, "
        "  date_taken, date_modified, width, height, camera_model, lens_model, "
        "  focal_length, aperture, shutter_speed, iso, rating, is_flagged, is_missing"
        ") VALUES ("
        "  :collection_id, :file_path, :file_name, :title, :file_size, :mime_type, "
        "  :date_taken, :date_modified, :width, :height, :camera_model, :lens_model, "
        "  :focal_length, :aperture, :shutter_speed, :iso, :rating, :is_flagged, :is_missing"
        ") ON CONFLICT(file_path) DO UPDATE SET "
        "  file_size = excluded.file_size, "
        "  date_modified = excluded.date_modified, "
        "  width = excluded.width, "
        "  height = excluded.height, "
        "  is_missing = excluded.is_missing;"
    ));

    QSqlQuery tagSelectQuery(db);
    tagSelectQuery.prepare(QStringLiteral("SELECT id FROM tags WHERE name = :name;"));

    QSqlQuery tagInsertQuery(db);
    tagInsertQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO tags (name) VALUES (:name);"));

    QSqlQuery itemTagQuery(db);
    itemTagQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO item_tags (item_id, tag_id) VALUES (:item_id, :tag_id);"));

    for (const CatalogItem &item : items)
    {
        const QString normalized = QDir::fromNativeSeparators(item.filePath);
        itemQuery.bindValue(QStringLiteral(":collection_id"), collectionId > 0 ? QVariant(collectionId) : QVariant());
        itemQuery.bindValue(QStringLiteral(":file_path"), normalized);
        itemQuery.bindValue(QStringLiteral(":file_name"), item.fileName.isEmpty() ? QFileInfo(normalized).fileName() : item.fileName);
        itemQuery.bindValue(QStringLiteral(":title"), item.title);
        itemQuery.bindValue(QStringLiteral(":file_size"), item.fileSizeBytes);
        itemQuery.bindValue(QStringLiteral(":mime_type"), item.mimeType);
        itemQuery.bindValue(QStringLiteral(":date_taken"), item.dateTaken.isValid() ? item.dateTaken : QVariant());
        itemQuery.bindValue(QStringLiteral(":date_modified"), item.fileLastModified.isValid() ? item.fileLastModified : QDateTime::currentDateTime());
        itemQuery.bindValue(QStringLiteral(":width"), item.dimensions.width());
        itemQuery.bindValue(QStringLiteral(":height"), item.dimensions.height());
        itemQuery.bindValue(QStringLiteral(":camera_model"), item.cameraModel);
        itemQuery.bindValue(QStringLiteral(":lens_model"), item.lensModel);
        itemQuery.bindValue(QStringLiteral(":focal_length"), item.focalLength);
        itemQuery.bindValue(QStringLiteral(":aperture"), item.aperture);
        itemQuery.bindValue(QStringLiteral(":shutter_speed"), item.shutterSpeed);
        itemQuery.bindValue(QStringLiteral(":iso"), item.iso);
        itemQuery.bindValue(QStringLiteral(":rating"), item.rating);
        itemQuery.bindValue(QStringLiteral(":is_flagged"), item.isFlagged ? 1 : 0);
        itemQuery.bindValue(QStringLiteral(":is_missing"), item.isMissing ? 1 : 0);

        if (!itemQuery.exec())
        {
            m_lastError = itemQuery.lastError().text();
            qCWarning(lcDb) << "Failed to insert/update item:" << normalized << ":" << m_lastError;
            db.rollback();
            return false;
        }

        // Handle tags
        if (!item.tags.isEmpty())
        {
            int insertedItemId = itemQuery.lastInsertId().toInt();
            if (insertedItemId <= 0)
            {
                // If it was an update, query the item id
                QSqlQuery idQuery(db);
                idQuery.prepare(QStringLiteral("SELECT id FROM items WHERE file_path = :path;"));
                idQuery.bindValue(QStringLiteral(":path"), normalized);
                if (idQuery.exec() && idQuery.next())
                {
                    insertedItemId = idQuery.value(0).toInt();
                }
            }

            if (insertedItemId > 0)
            {
                for (const QString &tag : item.tags)
                {
                    const QString trimmedTag = tag.trimmed();
                    if (trimmedTag.isEmpty())
                    {
                        continue;
                    }

                    tagInsertQuery.bindValue(QStringLiteral(":name"), trimmedTag);
                    tagInsertQuery.exec();

                    tagSelectQuery.bindValue(QStringLiteral(":name"), trimmedTag);
                    if (tagSelectQuery.exec() && tagSelectQuery.next())
                    {
                        const int tagId = tagSelectQuery.value(0).toInt();
                        itemTagQuery.bindValue(QStringLiteral(":item_id"), insertedItemId);
                        itemTagQuery.bindValue(QStringLiteral(":tag_id"), tagId);
                        itemTagQuery.exec();
                    }
                }
            }
        }
    }

    if (!db.commit())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    emit itemsChanged();
    return true;
}

namespace
{
CatalogItem parseItemRow(const QSqlQuery &query, QSqlDatabase &db)
{
    CatalogItem item;
    const int itemId = query.value(0).toInt();
    item.id = QString::number(itemId);
    item.collectionId = query.value(1).toInt();
    item.filePath = query.value(2).toString();
    item.fileName = query.value(3).toString();
    item.title = query.value(4).toString();
    item.fileSizeBytes = query.value(5).toLongLong();
    item.formattedFileSize = CatalogItem::formatBytes(item.fileSizeBytes);
    item.mimeType = query.value(6).toString();
    item.dateTaken = query.value(7).toDateTime();
    if (item.dateTaken.isValid())
    {
        item.formattedDate = item.dateTaken.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
    }
    item.fileLastModified = query.value(8).toDateTime();
    if (item.fileLastModified.isValid())
    {
        item.formattedLastModified = item.fileLastModified.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
    }
    item.dimensions = QSize(query.value(9).toInt(), query.value(10).toInt());
    if (item.dimensions.isValid())
    {
        item.formattedDimensions = QStringLiteral("%1 × %2").arg(item.dimensions.width()).arg(item.dimensions.height());
    }
    item.cameraModel = query.value(11).toString();
    item.lensModel = query.value(12).toString();
    item.focalLength = query.value(13).toString();
    item.aperture = query.value(14).toString();
    item.shutterSpeed = query.value(15).toString();
    item.iso = query.value(16).toInt();
    item.rating = query.value(17).toInt();
    item.isFlagged = query.value(18).toBool();
    item.isMissing = query.value(19).toBool();
    item.isDemoFixture = false;

    // Use local file URL for preview and thumbnail endpoints
    const QUrl fileUrl = QUrl::fromLocalFile(item.filePath);
    item.previewUrl = fileUrl;
    item.thumbnailUrl = QUrl(QStringLiteral("image://thumbnail/") + item.filePath);

    // Fetch tags
    QSqlQuery tagQuery(db);
    tagQuery.prepare(QStringLiteral(
        "SELECT t.name FROM tags t "
        "JOIN item_tags it ON t.id = it.tag_id "
        "WHERE it.item_id = :item_id ORDER BY t.name ASC;"
    ));
    tagQuery.bindValue(QStringLiteral(":item_id"), itemId);
    if (tagQuery.exec())
    {
        while (tagQuery.next())
        {
            item.tags.append(tagQuery.value(0).toString());
        }
    }

    return item;
}
} // namespace

QList<CatalogItem> CatalogDatabase::fetchAllItems() const
{
    QList<CatalogItem> list;
    if (!isOpen())
    {
        return list;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(QStringLiteral(
        "SELECT id, collection_id, file_path, file_name, title, file_size, mime_type, "
        "       date_taken, date_modified, width, height, camera_model, lens_model, "
        "       focal_length, aperture, shutter_speed, iso, rating, is_flagged, is_missing "
        "FROM items ORDER BY COALESCE(date_taken, date_modified) DESC;"
    ), db);

    while (query.next())
    {
        list.append(parseItemRow(query, db));
    }
    return list;
}

QList<CatalogItem> CatalogDatabase::fetchItemsByCollection(int collectionId) const
{
    QList<CatalogItem> list;
    if (!isOpen() || collectionId <= 0)
    {
        return list;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, collection_id, file_path, file_name, title, file_size, mime_type, "
        "       date_taken, date_modified, width, height, camera_model, lens_model, "
        "       focal_length, aperture, shutter_speed, iso, rating, is_flagged, is_missing "
        "FROM items WHERE collection_id = :cid ORDER BY COALESCE(date_taken, date_modified) DESC;"
    ));
    query.bindValue(QStringLiteral(":cid"), collectionId);

    if (query.exec())
    {
        while (query.next())
        {
            list.append(parseItemRow(query, db));
        }
    }
    return list;
}

CatalogItem CatalogDatabase::fetchItemByPath(const QString &filePath) const
{
    CatalogItem item;
    if (!isOpen() || filePath.isEmpty())
    {
        return item;
    }

    const QString normalized = QDir::fromNativeSeparators(filePath);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, collection_id, file_path, file_name, title, file_size, mime_type, "
        "       date_taken, date_modified, width, height, camera_model, lens_model, "
        "       focal_length, aperture, shutter_speed, iso, rating, is_flagged, is_missing "
        "FROM items WHERE file_path = :path LIMIT 1;"
    ));
    query.bindValue(QStringLiteral(":path"), normalized);

    if (query.exec() && query.next())
    {
        item = parseItemRow(query, db);
    }
    return item;
}

int CatalogDatabase::itemCount() const
{
    if (!isOpen())
    {
        return 0;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(QStringLiteral("SELECT COUNT(*) FROM items;"), db);
    if (query.next())
    {
        return query.value(0).toInt();
    }
    return 0;
}

bool CatalogDatabase::updateItemRating(const QString &filePath, int rating)
{
    if (!isOpen() || filePath.isEmpty())
    {
        return false;
    }

    const QString normalized = QDir::fromNativeSeparators(filePath);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE items SET rating = :rating WHERE file_path = :path;"));
    query.bindValue(QStringLiteral(":rating"), qBound(0, rating, 5));
    query.bindValue(QStringLiteral(":path"), normalized);

    if (query.exec())
    {
        emit itemsChanged();
        return true;
    }
    m_lastError = query.lastError().text();
    return false;
}

bool CatalogDatabase::updateItemFlagged(const QString &filePath, bool flagged)
{
    if (!isOpen() || filePath.isEmpty())
    {
        return false;
    }

    const QString normalized = QDir::fromNativeSeparators(filePath);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE items SET is_flagged = :flag WHERE file_path = :path;"));
    query.bindValue(QStringLiteral(":flag"), flagged ? 1 : 0);
    query.bindValue(QStringLiteral(":path"), normalized);

    if (query.exec())
    {
        emit itemsChanged();
        return true;
    }
    m_lastError = query.lastError().text();
    return false;
}

bool CatalogDatabase::setItemTags(const QString &filePath, const QStringList &tags)
{
    if (!isOpen() || filePath.isEmpty())
    {
        return false;
    }

    const QString normalized = QDir::fromNativeSeparators(filePath);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    // Find item ID
    QSqlQuery idQuery(db);
    idQuery.prepare(QStringLiteral("SELECT id FROM items WHERE file_path = :path;"));
    idQuery.bindValue(QStringLiteral(":path"), normalized);
    if (!idQuery.exec() || !idQuery.next())
    {
        return false;
    }
    const int itemId = idQuery.value(0).toInt();

    db.transaction();

    // Delete existing tags for item
    QSqlQuery deleteQuery(db);
    deleteQuery.prepare(QStringLiteral("DELETE FROM item_tags WHERE item_id = :id;"));
    deleteQuery.bindValue(QStringLiteral(":id"), itemId);
    if (!deleteQuery.exec())
    {
        db.rollback();
        return false;
    }

    // Insert new tags
    QSqlQuery tagInsertQuery(db);
    tagInsertQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO tags (name) VALUES (:name);"));

    QSqlQuery tagSelectQuery(db);
    tagSelectQuery.prepare(QStringLiteral("SELECT id FROM tags WHERE name = :name;"));

    QSqlQuery linkQuery(db);
    linkQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO item_tags (item_id, tag_id) VALUES (:item_id, :tag_id);"));

    for (const QString &tag : tags)
    {
        const QString trimmed = tag.trimmed();
        if (trimmed.isEmpty())
        {
            continue;
        }

        tagInsertQuery.bindValue(QStringLiteral(":name"), trimmed);
        tagInsertQuery.exec();

        tagSelectQuery.bindValue(QStringLiteral(":name"), trimmed);
        if (tagSelectQuery.exec() && tagSelectQuery.next())
        {
            const int tagId = tagSelectQuery.value(0).toInt();
            linkQuery.bindValue(QStringLiteral(":item_id"), itemId);
            linkQuery.bindValue(QStringLiteral(":tag_id"), tagId);
            linkQuery.exec();
        }
    }

    db.commit();
    emit itemsChanged();
    return true;
}

bool CatalogDatabase::updateItemsRating(const QStringList &filePaths, int rating)
{
    if (!isOpen() || filePaths.isEmpty())
    {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE items SET rating = :rating WHERE file_path = :path;"));
    const int boundedRating = qBound(0, rating, 5);

    for (const QString &filePath : filePaths)
    {
        if (filePath.isEmpty())
        {
            continue;
        }

        query.bindValue(QStringLiteral(":rating"), boundedRating);
        query.bindValue(QStringLiteral(":path"), QDir::fromNativeSeparators(filePath));
        if (!query.exec())
        {
            m_lastError = query.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (!db.commit())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    emit itemsChanged();
    return true;
}

bool CatalogDatabase::updateItemsFlagged(const QStringList &filePaths, bool flagged)
{
    if (!isOpen() || filePaths.isEmpty())
    {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE items SET is_flagged = :flag WHERE file_path = :path;"));

    for (const QString &filePath : filePaths)
    {
        if (filePath.isEmpty())
        {
            continue;
        }

        query.bindValue(QStringLiteral(":flag"), flagged ? 1 : 0);
        query.bindValue(QStringLiteral(":path"), QDir::fromNativeSeparators(filePath));
        if (!query.exec())
        {
            m_lastError = query.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (!db.commit())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    emit itemsChanged();
    return true;
}

bool CatalogDatabase::addTagsToItems(const QStringList &filePaths, const QStringList &tags)
{
    if (!isOpen() || filePaths.isEmpty() || tags.isEmpty())
    {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery tagInsertQuery(db);
    tagInsertQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO tags (name) VALUES (:name);"));

    QSqlQuery tagSelectQuery(db);
    tagSelectQuery.prepare(QStringLiteral("SELECT id FROM tags WHERE name = :name;"));

    QSqlQuery linkQuery(db);
    linkQuery.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO item_tags (item_id, tag_id) "
        "SELECT id, :tag_id FROM items WHERE file_path = :path;"));

    for (const QString &tag : tags)
    {
        const QString trimmed = tag.trimmed();
        if (trimmed.isEmpty())
        {
            continue;
        }

        tagInsertQuery.bindValue(QStringLiteral(":name"), trimmed);
        if (!tagInsertQuery.exec())
        {
            m_lastError = tagInsertQuery.lastError().text();
            db.rollback();
            return false;
        }

        tagSelectQuery.bindValue(QStringLiteral(":name"), trimmed);
        if (!tagSelectQuery.exec() || !tagSelectQuery.next())
        {
            m_lastError = tagSelectQuery.lastError().text();
            db.rollback();
            return false;
        }

        const int tagId = tagSelectQuery.value(0).toInt();
        for (const QString &filePath : filePaths)
        {
            if (filePath.isEmpty())
            {
                continue;
            }

            linkQuery.bindValue(QStringLiteral(":tag_id"), tagId);
            linkQuery.bindValue(QStringLiteral(":path"), QDir::fromNativeSeparators(filePath));
            if (!linkQuery.exec())
            {
                m_lastError = linkQuery.lastError().text();
                db.rollback();
                return false;
            }
        }
    }

    if (!db.commit())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    emit itemsChanged();
    return true;
}

bool CatalogDatabase::removeTagsFromItems(const QStringList &filePaths, const QStringList &tags)
{
    if (!isOpen() || filePaths.isEmpty() || tags.isEmpty())
    {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery tagSelectQuery(db);
    tagSelectQuery.prepare(QStringLiteral("SELECT id FROM tags WHERE name = :name;"));

    QSqlQuery unlinkQuery(db);
    unlinkQuery.prepare(QStringLiteral(
        "DELETE FROM item_tags WHERE tag_id = :tag_id "
        "AND item_id = (SELECT id FROM items WHERE file_path = :path);"));

    for (const QString &tag : tags)
    {
        const QString trimmed = tag.trimmed();
        if (trimmed.isEmpty())
        {
            continue;
        }

        tagSelectQuery.bindValue(QStringLiteral(":name"), trimmed);
        if (!tagSelectQuery.exec())
        {
            m_lastError = tagSelectQuery.lastError().text();
            db.rollback();
            return false;
        }

        if (!tagSelectQuery.next())
        {
            continue;
        }

        const int tagId = tagSelectQuery.value(0).toInt();
        for (const QString &filePath : filePaths)
        {
            if (filePath.isEmpty())
            {
                continue;
            }

            unlinkQuery.bindValue(QStringLiteral(":tag_id"), tagId);
            unlinkQuery.bindValue(QStringLiteral(":path"), QDir::fromNativeSeparators(filePath));
            if (!unlinkQuery.exec())
            {
                m_lastError = unlinkQuery.lastError().text();
                db.rollback();
                return false;
            }
        }
    }

    if (!db.commit())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    emit itemsChanged();
    return true;
}

QStringList CatalogDatabase::allTags() const
{
    QStringList list;
    if (!isOpen())
    {
        return list;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(QStringLiteral("SELECT name FROM tags ORDER BY name ASC;"), db);
    while (query.next())
    {
        list.append(query.value(0).toString());
    }
    return list;
}

} // namespace PhotoCarta
