// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalogdatabase.h"
#include "digikamdatabaseadapter.h"

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
                           : connectionName),
      m_digikamAdapter(new DigikamDatabaseAdapter(this))
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

    if (!executePragmas())
    {
        close();
        return false;
    }

    // Inspect database schema to detect digiKam vs native PhotoCarta format
    if (DigikamDatabaseAdapter::isDigikamDatabase(db))
    {
        m_schemaType = DatabaseSchemaType::Digikam;
        qCInfo(lcDb) << "Detected existing digiKam database format at" << m_databasePath;
    }
    else
    {
        m_schemaType = DatabaseSchemaType::PhotoCarta;
        if (!createTables())
        {
            close();
            return false;
        }

        // Lightweight forward-compatible column migrations for existing databases
        QSqlQuery migrationQuery(db);
        migrationQuery.exec(QStringLiteral("ALTER TABLE items ADD COLUMN caption TEXT;"));
        migrationQuery.exec(QStringLiteral("ALTER TABLE items ADD COLUMN color_label INTEGER DEFAULT 0;"));
        migrationQuery.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_items_color_label ON items(color_label);"));

        loadTagCache();
        qCInfo(lcDb) << "Connected to PhotoCarta SQLite catalog at" << m_databasePath;
    }

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
    m_schemaType = DatabaseSchemaType::PhotoCarta;
    m_tagCache.clear();
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

DatabaseSchemaType CatalogDatabase::schemaType() const
{
    return m_schemaType;
}

bool CatalogDatabase::isDigikamDatabase() const
{
    return m_schemaType == DatabaseSchemaType::Digikam;
}

DigikamDatabaseAdapter *CatalogDatabase::digikamAdapter() const
{
    return m_digikamAdapter;
}

void CatalogDatabase::setDigikamRootPathMapping(int rootId, const QString &localPath)
{
    if (m_digikamAdapter)
    {
        m_digikamAdapter->setRootPathMapping(rootId, localPath);
    }
}

bool CatalogDatabase::executePragmas()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    const QStringList pragmas = {
        QStringLiteral("PRAGMA journal_mode = WAL;"),
        QStringLiteral("PRAGMA synchronous = NORMAL;"),
        QStringLiteral("PRAGMA foreign_keys = ON;"),
        QStringLiteral("PRAGMA cache_size = -64000;"),     // 64MB RAM page cache (vs 2MB default)
        QStringLiteral("PRAGMA temp_store = MEMORY;"),     // Store temporary tables and sorting structures in RAM
        QStringLiteral("PRAGMA mmap_size = 268435456;"),   // 256MB memory mapped I/O for high read throughput
        QStringLiteral("PRAGMA busy_timeout = 5000;"),     // 5000ms lock timeout to avoid concurrency lock errors
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
        "  caption TEXT,"
        "  color_label INTEGER DEFAULT 0,"
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
        "CREATE INDEX IF NOT EXISTS idx_items_color_label ON items(color_label);"
        "CREATE INDEX IF NOT EXISTS idx_items_file_path ON items(file_path);"
        "CREATE INDEX IF NOT EXISTS idx_collections_root ON collections(root_path);"
        // Expression index: allows instant ORDER BY COALESCE(date_taken, date_modified) DESC without temp B-Tree sort
        "CREATE INDEX IF NOT EXISTS idx_items_sort_date ON items(COALESCE(date_taken, date_modified) DESC);"
        // Composite covering index for collection-filtered date sorting
        "CREATE INDEX IF NOT EXISTS idx_items_coll_sort_date ON items(collection_id, COALESCE(date_taken, date_modified) DESC);"
        // Covering reverse index on item_tags for rapid tag-based item filtering
        "CREATE INDEX IF NOT EXISTS idx_item_tags_tag_id ON item_tags(tag_id, item_id);"
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

void CatalogDatabase::loadTagCache() const
{
    m_tagCache.clear();
    if (!isOpen())
    {
        return;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(QStringLiteral("SELECT id, name FROM tags;"), db);
    while (query.next())
    {
        m_tagCache.insert(query.value(1).toString(), query.value(0).toInt());
    }
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

    // If digiKam database, delegate to adapter
    if (m_schemaType == DatabaseSchemaType::Digikam && m_digikamAdapter)
    {
        return m_digikamAdapter->getCollections(db);
    }

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
    const auto collections = getCollections();
    for (const auto &c : collections)
    {
        if (c.rootPath == normalized)
        {
            return c;
        }
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
        "  collection_id, file_path, file_name, title, caption, color_label, file_size, mime_type, "
        "  date_taken, date_modified, width, height, camera_model, lens_model, "
        "  focal_length, aperture, shutter_speed, iso, rating, is_flagged, is_missing"
        ") VALUES ("
        "  :collection_id, :file_path, :file_name, :title, :caption, :color_label, :file_size, :mime_type, "
        "  :date_taken, :date_modified, :width, :height, :camera_model, :lens_model, "
        "  :focal_length, :aperture, :shutter_speed, :iso, :rating, :is_flagged, :is_missing"
        ") ON CONFLICT(file_path) DO UPDATE SET "
        "  file_size = excluded.file_size, "
        "  date_modified = excluded.date_modified, "
        "  width = excluded.width, "
        "  height = excluded.height, "
        "  is_missing = excluded.is_missing;"
    ));

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
        itemQuery.bindValue(QStringLiteral(":caption"), item.caption);
        itemQuery.bindValue(QStringLiteral(":color_label"), item.colorLabel);
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

        // Handle tags with in-memory caching to eliminate repeated SELECT queries
        if (!item.tags.isEmpty())
        {
            int insertedItemId = itemQuery.lastInsertId().toInt();
            if (insertedItemId <= 0)
            {
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

                    int tagId = m_tagCache.value(trimmedTag, 0);
                    if (tagId == 0)
                    {
                        tagInsertQuery.bindValue(QStringLiteral(":name"), trimmedTag);
                        if (tagInsertQuery.exec())
                        {
                            tagId = tagInsertQuery.lastInsertId().toInt();
                            if (tagId <= 0)
                            {
                                QSqlQuery selectQuery(db);
                                selectQuery.prepare(QStringLiteral("SELECT id FROM tags WHERE name = :name;"));
                                selectQuery.bindValue(QStringLiteral(":name"), trimmedTag);
                                if (selectQuery.exec() && selectQuery.next())
                                {
                                    tagId = selectQuery.value(0).toInt();
                                }
                            }
                            if (tagId > 0)
                            {
                                m_tagCache.insert(trimmedTag, tagId);
                            }
                        }
                    }

                    if (tagId > 0)
                    {
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
CatalogItem parseOptimizedItemRow(const QSqlQuery &query)
{
    CatalogItem item;
    const int itemId = query.value(0).toInt();
    item.id = QString::number(itemId);
    item.collectionId = query.value(1).toInt();
    item.filePath = query.value(2).toString();
    item.fileName = query.value(3).toString();
    item.title = query.value(4).toString();
    item.caption = query.value(5).toString();
    item.colorLabel = query.value(6).toInt();
    item.fileSizeBytes = query.value(7).toLongLong();
    item.formattedFileSize = CatalogItem::formatBytes(item.fileSizeBytes);
    item.mimeType = query.value(8).toString();
    item.dateTaken = query.value(9).toDateTime();
    if (item.dateTaken.isValid())
    {
        item.formattedDate = item.dateTaken.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
    }
    item.fileLastModified = query.value(10).toDateTime();
    if (item.fileLastModified.isValid())
    {
        item.formattedLastModified = item.fileLastModified.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
    }
    item.dimensions = QSize(query.value(11).toInt(), query.value(12).toInt());
    if (item.dimensions.isValid())
    {
        item.formattedDimensions = QStringLiteral("%1 × %2").arg(item.dimensions.width()).arg(item.dimensions.height());
    }
    item.cameraModel = query.value(13).toString();
    item.lensModel = query.value(14).toString();
    item.focalLength = query.value(15).toString();
    item.aperture = query.value(16).toString();
    item.shutterSpeed = query.value(17).toString();
    item.iso = query.value(18).toInt();
    item.rating = query.value(19).toInt();
    item.isFlagged = query.value(20).toBool();
    item.isMissing = query.value(21).toBool();
    item.isDemoFixture = false;

    // Direct O(1) tag assignment from GROUP_CONCAT, avoiding N+1 subqueries
    const QString tagsConcat = query.value(22).toString();
    if (!tagsConcat.isEmpty())
    {
        item.tags = tagsConcat.split(QStringLiteral("|||"), Qt::SkipEmptyParts);
    }

    // Local file URL endpoints
    const QUrl fileUrl = QUrl::fromLocalFile(item.filePath);
    item.previewUrl = fileUrl;
    item.thumbnailUrl = QUrl(QStringLiteral("image://thumbnail/") + item.filePath);

    return item;
}
} // namespace

QList<CatalogItem> CatalogDatabase::fetchAllItems() const
{
    return fetchItemsPaged(0, -1, 0);
}

QList<CatalogItem> CatalogDatabase::fetchItemsByCollection(int collectionId) const
{
    return fetchItemsPaged(0, -1, collectionId);
}

QList<CatalogItem> CatalogDatabase::fetchItemsPaged(int offset, int limit, int collectionId) const
{
    QList<CatalogItem> list;
    if (!isOpen())
    {
        return list;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    // If digiKam database, delegate to adapter
    if (m_schemaType == DatabaseSchemaType::Digikam && m_digikamAdapter)
    {
        if (collectionId > 0)
        {
            return m_digikamAdapter->fetchItemsByAlbum(db, collectionId, limit, offset);
        }
        return m_digikamAdapter->fetchAllItems(db, limit, offset);
    }

    // High performance single-pass query with GROUP_CONCAT to eliminate N+1 queries entirely
    QString sql = QStringLiteral(
        "SELECT i.id, i.collection_id, i.file_path, i.file_name, i.title, i.caption, i.color_label, i.file_size, i.mime_type, "
        "       i.date_taken, i.date_modified, i.width, i.height, i.camera_model, i.lens_model, "
        "       i.focal_length, i.aperture, i.shutter_speed, i.iso, i.rating, i.is_flagged, i.is_missing, "
        "       GROUP_CONCAT(t.name, '|||') AS tag_names "
        "FROM items i "
        "LEFT JOIN item_tags it ON i.id = it.item_id "
        "LEFT JOIN tags t ON it.tag_id = t.id "
    );

    if (collectionId > 0)
    {
        sql += QStringLiteral("WHERE i.collection_id = :cid ");
    }

    sql += QStringLiteral(
        "GROUP BY i.id "
        "ORDER BY COALESCE(i.date_taken, i.date_modified) DESC"
    );

    if (limit > 0)
    {
        sql += QStringLiteral(" LIMIT %1").arg(limit);
        if (offset > 0)
        {
            sql += QStringLiteral(" OFFSET %1").arg(offset);
        }
    }
    sql += QLatin1Char(';');

    QSqlQuery query(db);
    query.prepare(sql);
    if (collectionId > 0)
    {
        query.bindValue(QStringLiteral(":cid"), collectionId);
    }

    if (query.exec())
    {
        while (query.next())
        {
            list.append(parseOptimizedItemRow(query));
        }
    }
    else
    {
        qCWarning(lcDb) << "Failed to fetch items:" << query.lastError().text();
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

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (m_schemaType == DatabaseSchemaType::Digikam && m_digikamAdapter)
    {
        return m_digikamAdapter->fetchItemByPath(db, filePath);
    }

    const QString normalized = QDir::fromNativeSeparators(filePath);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT i.id, i.collection_id, i.file_path, i.file_name, i.title, i.caption, i.color_label, i.file_size, i.mime_type, "
        "       i.date_taken, i.date_modified, i.width, i.height, i.camera_model, i.lens_model, "
        "       i.focal_length, i.aperture, i.shutter_speed, i.iso, i.rating, i.is_flagged, i.is_missing, "
        "       GROUP_CONCAT(t.name, '|||') AS tag_names "
        "FROM items i "
        "LEFT JOIN item_tags it ON i.id = it.item_id "
        "LEFT JOIN tags t ON it.tag_id = t.id "
        "WHERE i.file_path = :path "
        "GROUP BY i.id LIMIT 1;"
    ));
    query.bindValue(QStringLiteral(":path"), normalized);

    if (query.exec() && query.next())
    {
        item = parseOptimizedItemRow(query);
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
    if (m_schemaType == DatabaseSchemaType::Digikam && m_digikamAdapter)
    {
        return m_digikamAdapter->itemCount(db);
    }

    QSqlQuery query(QStringLiteral("SELECT COUNT(*) FROM items;"), db);
    if (query.next())
    {
        return query.value(0).toInt();
    }
    return 0;
}

bool CatalogDatabase::updateItemPath(const QString &oldFilePath, const QString &newFilePath, const QString &newFileName)
{
    if (!isOpen() || oldFilePath.isEmpty() || newFilePath.isEmpty())
    {
        return false;
    }

    const QString oldNormalized = QDir::fromNativeSeparators(oldFilePath);
    const QString newNormalized = QDir::fromNativeSeparators(newFilePath);
    const QString finalFileName = newFileName.isEmpty() ? QFileInfo(newNormalized).fileName() : newFileName;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE items SET file_path = :new_path, file_name = :new_name WHERE file_path = :old_path;"));
    query.bindValue(QStringLiteral(":new_path"), newNormalized);
    query.bindValue(QStringLiteral(":new_name"), finalFileName);
    query.bindValue(QStringLiteral(":old_path"), oldNormalized);

    if (query.exec())
    {
        emit itemsChanged();
        return true;
    }
    m_lastError = query.lastError().text();
    return false;
}

bool CatalogDatabase::updateItemTitle(const QString &filePath, const QString &title)
{
    if (!isOpen() || filePath.isEmpty())
    {
        return false;
    }

    const QString normalized = QDir::fromNativeSeparators(filePath);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE items SET title = :title WHERE file_path = :path;"));
    query.bindValue(QStringLiteral(":title"), title);
    query.bindValue(QStringLiteral(":path"), normalized);

    if (query.exec())
    {
        emit itemsChanged();
        return true;
    }
    m_lastError = query.lastError().text();
    return false;
}

bool CatalogDatabase::updateItemCaption(const QString &filePath, const QString &caption)
{
    if (!isOpen() || filePath.isEmpty())
    {
        return false;
    }

    const QString normalized = QDir::fromNativeSeparators(filePath);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE items SET caption = :caption WHERE file_path = :path;"));
    query.bindValue(QStringLiteral(":caption"), caption);
    query.bindValue(QStringLiteral(":path"), normalized);

    if (query.exec())
    {
        emit itemsChanged();
        return true;
    }
    m_lastError = query.lastError().text();
    return false;
}

bool CatalogDatabase::updateItemColorLabel(const QString &filePath, int colorLabel)
{
    if (!isOpen() || filePath.isEmpty())
    {
        return false;
    }

    const QString normalized = QDir::fromNativeSeparators(filePath);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE items SET color_label = :color WHERE file_path = :path;"));
    query.bindValue(QStringLiteral(":color"), qBound(0, colorLabel, 5));
    query.bindValue(QStringLiteral(":path"), normalized);

    if (query.exec())
    {
        emit itemsChanged();
        return true;
    }
    m_lastError = query.lastError().text();
    return false;
}

bool CatalogDatabase::updateItemRating(const QString &filePath, int rating)
{
    if (!isOpen() || filePath.isEmpty())
    {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (m_schemaType == DatabaseSchemaType::Digikam && m_digikamAdapter)
    {
        const bool ok = m_digikamAdapter->updateItemRatingByPath(db, filePath, rating);
        if (ok)
        {
            emit itemsChanged();
        }
        return ok;
    }

    const QString normalized = QDir::fromNativeSeparators(filePath);
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

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (m_schemaType == DatabaseSchemaType::Digikam && m_digikamAdapter)
    {
        const bool ok = m_digikamAdapter->setItemTagsByPath(db, filePath, tags);
        if (ok)
        {
            emit itemsChanged();
        }
        return ok;
    }

    const QString normalized = QDir::fromNativeSeparators(filePath);

    // Get item ID
    QSqlQuery idQuery(db);
    idQuery.prepare(QStringLiteral("SELECT id FROM items WHERE file_path = :path;"));
    idQuery.bindValue(QStringLiteral(":path"), normalized);
    if (!idQuery.exec() || !idQuery.next())
    {
        return false;
    }
    const int itemId = idQuery.value(0).toInt();

    db.transaction();

    // Clear existing tag associations
    QSqlQuery clearQuery(db);
    clearQuery.prepare(QStringLiteral("DELETE FROM item_tags WHERE item_id = :id;"));
    clearQuery.bindValue(QStringLiteral(":id"), itemId);
    clearQuery.exec();

    // Re-link new tags with tag cache
    QSqlQuery tagInsertQuery(db);
    tagInsertQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO tags (name) VALUES (:name);"));

    QSqlQuery linkQuery(db);
    linkQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO item_tags (item_id, tag_id) VALUES (:item_id, :tag_id);"));

    for (const QString &tag : tags)
    {
        const QString trimmed = tag.trimmed();
        if (trimmed.isEmpty())
        {
            continue;
        }

        int tagId = m_tagCache.value(trimmed, 0);
        if (tagId == 0)
        {
            tagInsertQuery.bindValue(QStringLiteral(":name"), trimmed);
            tagInsertQuery.exec();

            QSqlQuery tagSelectQuery(db);
            tagSelectQuery.prepare(QStringLiteral("SELECT id FROM tags WHERE name = :name;"));
            tagSelectQuery.bindValue(QStringLiteral(":name"), trimmed);
            if (tagSelectQuery.exec() && tagSelectQuery.next())
            {
                tagId = tagSelectQuery.value(0).toInt();
                m_tagCache.insert(trimmed, tagId);
            }
        }

        if (tagId > 0)
        {
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
    const int boundedRating = qBound(0, rating, 5);

    if (m_schemaType == DatabaseSchemaType::Digikam && m_digikamAdapter)
    {
        if (!db.transaction())
        {
            m_lastError = db.lastError().text();
            return false;
        }

        for (const QString &filePath : filePaths)
        {
            if (filePath.isEmpty())
            {
                continue;
            }

            if (!m_digikamAdapter->updateItemRatingByPath(
                    db, QDir::fromNativeSeparators(filePath), boundedRating))
            {
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

    if (!db.transaction())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE items SET rating = :rating WHERE file_path = :path;"));

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

    if (m_schemaType == DatabaseSchemaType::Digikam && m_digikamAdapter)
    {
        QStringList requestedTags;
        for (const QString &tag : tags)
        {
            const QString trimmed = tag.trimmed();
            if (trimmed.isEmpty())
            {
                continue;
            }

            bool duplicate = false;
            for (const QString &existing : requestedTags)
            {
                if (existing.compare(trimmed, Qt::CaseInsensitive) == 0)
                {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate)
            {
                requestedTags.append(trimmed);
            }
        }

        if (requestedTags.isEmpty())
        {
            return true;
        }

        if (!db.transaction())
        {
            m_lastError = db.lastError().text();
            return false;
        }

        for (const QString &filePath : filePaths)
        {
            if (filePath.isEmpty())
            {
                continue;
            }

            const QString normalized = QDir::fromNativeSeparators(filePath);
            const CatalogItem item = m_digikamAdapter->fetchItemByPath(db, normalized);
            if (item.id.isEmpty())
            {
                db.rollback();
                return false;
            }

            QStringList updatedTags = item.tags;
            for (const QString &tag : requestedTags)
            {
                bool present = false;
                for (const QString &existing : updatedTags)
                {
                    if (existing.compare(tag, Qt::CaseInsensitive) == 0)
                    {
                        present = true;
                        break;
                    }
                }
                if (!present)
                {
                    updatedTags.append(tag);
                }
            }

            if (updatedTags.size() != item.tags.size()
                && !m_digikamAdapter->setItemTagsByPath(db, normalized, updatedTags))
            {
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

    if (m_schemaType == DatabaseSchemaType::Digikam && m_digikamAdapter)
    {
        QStringList requestedTags;
        for (const QString &tag : tags)
        {
            const QString trimmed = tag.trimmed();
            if (!trimmed.isEmpty())
            {
                requestedTags.append(trimmed);
            }
        }

        if (requestedTags.isEmpty())
        {
            return true;
        }

        if (!db.transaction())
        {
            m_lastError = db.lastError().text();
            return false;
        }

        for (const QString &filePath : filePaths)
        {
            if (filePath.isEmpty())
            {
                continue;
            }

            const QString normalized = QDir::fromNativeSeparators(filePath);
            const CatalogItem item = m_digikamAdapter->fetchItemByPath(db, normalized);
            if (item.id.isEmpty())
            {
                db.rollback();
                return false;
            }

            QStringList updatedTags;
            for (const QString &existing : item.tags)
            {
                bool remove = false;
                for (const QString &tag : requestedTags)
                {
                    if (existing.compare(tag, Qt::CaseInsensitive) == 0)
                    {
                        remove = true;
                        break;
                    }
                }
                if (!remove)
                {
                    updatedTags.append(existing);
                }
            }

            if (updatedTags.size() != item.tags.size()
                && !m_digikamAdapter->setItemTagsByPath(db, normalized, updatedTags))
            {
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

bool CatalogDatabase::removeItems(const QStringList &filePaths)
{
    if (!isOpen() || filePaths.isEmpty())
    {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    if (m_schemaType == DatabaseSchemaType::Digikam && m_digikamAdapter)
    {
        if (!db.transaction())
        {
            m_lastError = db.lastError().text();
            return false;
        }

        for (const QString &filePath : filePaths)
        {
            if (filePath.isEmpty())
            {
                continue;
            }

            const CatalogItem item = m_digikamAdapter->fetchItemByPath(db, QDir::fromNativeSeparators(filePath));
            if (item.id.isEmpty())
            {
                continue;
            }

            QSqlQuery deleteQuery(db);
            deleteQuery.prepare(QStringLiteral("DELETE FROM Images WHERE id = :id;"));
            deleteQuery.bindValue(QStringLiteral(":id"), item.id.toInt());
            if (!deleteQuery.exec())
            {
                m_lastError = deleteQuery.lastError().text();
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
        emit collectionsChanged();
        return true;
    }

    if (!db.transaction())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM items WHERE file_path = :path;"));

    for (const QString &filePath : filePaths)
    {
        if (filePath.isEmpty())
        {
            continue;
        }

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
    emit collectionsChanged();
    return true;
}

QStringList CatalogDatabase::allTags() const
{
    QStringList tags;
    if (!isOpen())
    {
        return tags;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (m_schemaType == DatabaseSchemaType::Digikam && m_digikamAdapter)
    {
        return m_digikamAdapter->allTags(db);
    }

    QSqlQuery query(QStringLiteral("SELECT DISTINCT name FROM tags ORDER BY name ASC;"), db);
    while (query.next())
    {
        tags.append(query.value(0).toString());
    }
    return tags;
}

bool CatalogDatabase::importFromDigikamDatabase(const QString &digikamDatabasePath)
{
    if (!isOpen() || digikamDatabasePath.isEmpty())
    {
        return false;
    }

    const QString extConnName = QString::asprintf("ext_digikam_%p", static_cast<const void *>(this));
    {
        QSqlDatabase digikamDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), extConnName);
        digikamDb.setDatabaseName(digikamDatabasePath);

        if (!digikamDb.open())
        {
            m_lastError = digikamDb.lastError().text();
            QSqlDatabase::removeDatabase(extConnName);
            return false;
        }

        if (!DigikamDatabaseAdapter::isDigikamDatabase(digikamDb))
        {
            m_lastError = QStringLiteral("File is not a valid digiKam SQLite database");
            digikamDb.close();
            QSqlDatabase::removeDatabase(extConnName);
            return false;
        }

        const bool imported = m_digikamAdapter->importIntoCatalog(digikamDb, this);
        digikamDb.close();
        QSqlDatabase::removeDatabase(extConnName);

        if (imported)
        {
            emit collectionsChanged();
            emit itemsChanged();
            return true;
        }
    }

    QSqlDatabase::removeDatabase(extConnName);
    return false;
}

} // namespace PhotoCarta
