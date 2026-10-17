// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "digikamdatabaseadapter.h"
#include "catalogdatabase.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <cmath>

namespace
{
Q_LOGGING_CATEGORY(lcDigikamDb, "photocarta.database.digikam")
}

namespace PhotoCarta
{

DigikamDatabaseAdapter::DigikamDatabaseAdapter(QObject *parent)
    : QObject(parent)
{
}

bool DigikamDatabaseAdapter::isDigikamDatabase(const QSqlDatabase &db)
{
    if (!db.isOpen())
    {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM sqlite_master "
        "WHERE type = 'table' AND name IN ('AlbumRoots', 'Albums', 'Images');"
    ));

    if (query.exec() && query.next())
    {
        return query.value(0).toInt() >= 3;
    }
    return false;
}

bool DigikamDatabaseAdapter::createDigikamSchema(QSqlDatabase &db)
{
    if (!db.isOpen())
    {
        return false;
    }

    const QString schemaSql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS AlbumRoots ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  label TEXT,"
        "  status INTEGER DEFAULT 0,"
        "  type INTEGER DEFAULT 1,"
        "  identifier TEXT,"
        "  specificPath TEXT"
        ");"

        "CREATE TABLE IF NOT EXISTS Albums ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  albumRoot INTEGER REFERENCES AlbumRoots(id) ON DELETE CASCADE,"
        "  relativePath TEXT NOT NULL,"
        "  date DATE,"
        "  caption TEXT,"
        "  collection TEXT,"
        "  icon INTEGER DEFAULT 0"
        ");"

        "CREATE TABLE IF NOT EXISTS Images ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  album INTEGER REFERENCES Albums(id) ON DELETE CASCADE,"
        "  name TEXT NOT NULL,"
        "  status INTEGER DEFAULT 1,"
        "  category INTEGER DEFAULT 1,"
        "  modificationDate DATETIME,"
        "  fileSize INTEGER DEFAULT 0,"
        "  uniqueHash TEXT"
        ");"

        "CREATE TABLE IF NOT EXISTS ImageInformation ("
        "  imageid INTEGER PRIMARY KEY REFERENCES Images(id) ON DELETE CASCADE,"
        "  rating INTEGER DEFAULT 0,"
        "  creationDate DATETIME,"
        "  digitizationDate DATETIME,"
        "  orientation INTEGER DEFAULT 1,"
        "  width INTEGER DEFAULT 0,"
        "  height INTEGER DEFAULT 0,"
        "  format TEXT,"
        "  colorModel INTEGER DEFAULT 0,"
        "  colorDepth INTEGER DEFAULT 8"
        ");"

        "CREATE TABLE IF NOT EXISTS ImageMetadata ("
        "  imageid INTEGER PRIMARY KEY REFERENCES Images(id) ON DELETE CASCADE,"
        "  make TEXT,"
        "  model TEXT,"
        "  lens TEXT,"
        "  aperture REAL,"
        "  focalLength REAL,"
        "  focalLength35 REAL,"
        "  exposureTime REAL,"
        "  exposureProgram INTEGER DEFAULT 0,"
        "  exposureMode INTEGER DEFAULT 0,"
        "  sensitivity INTEGER DEFAULT 0,"
        "  flash INTEGER DEFAULT 0,"
        "  whiteBalance INTEGER DEFAULT 0,"
        "  whiteBalanceColorTemperature INTEGER DEFAULT 0"
        ");"

        "CREATE TABLE IF NOT EXISTS Tags ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  pid INTEGER DEFAULT 0,"
        "  name TEXT NOT NULL,"
        "  icon INTEGER DEFAULT 0,"
        "  iconkde TEXT"
        ");"

        "CREATE TABLE IF NOT EXISTS ImageTags ("
        "  imageid INTEGER REFERENCES Images(id) ON DELETE CASCADE,"
        "  tagid INTEGER REFERENCES Tags(id) ON DELETE CASCADE,"
        "  PRIMARY KEY (imageid, tagid)"
        ");"

        "CREATE TABLE IF NOT EXISTS ImagePositions ("
        "  imageid INTEGER PRIMARY KEY REFERENCES Images(id) ON DELETE CASCADE,"
        "  latitude TEXT,"
        "  latitudeNumber REAL,"
        "  longitude TEXT,"
        "  longitudeNumber REAL,"
        "  altitude REAL"
        ");"

        "CREATE TABLE IF NOT EXISTS ImageComments ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  imageid INTEGER REFERENCES Images(id) ON DELETE CASCADE,"
        "  type INTEGER DEFAULT 1,"
        "  comment TEXT"
        ");"

        "CREATE INDEX IF NOT EXISTS idx_digikam_images_album ON Images(album);"
        "CREATE INDEX IF NOT EXISTS idx_digikam_images_name ON Images(name);"
        "CREATE INDEX IF NOT EXISTS idx_digikam_albums_root ON Albums(albumRoot);"
        "CREATE INDEX IF NOT EXISTS idx_digikam_imagetags_tag ON ImageTags(tagid, imageid);"
        "CREATE INDEX IF NOT EXISTS idx_digikam_info_rating ON ImageInformation(rating);"
        "CREATE INDEX IF NOT EXISTS idx_digikam_info_creation ON ImageInformation(creationDate);"
    );

    QSqlQuery query(db);
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
            qCCritical(lcDigikamDb) << "Failed to create digiKam schema statement:" << trimmed << ":" << query.lastError().text();
            return false;
        }
    }
    return true;
}

void DigikamDatabaseAdapter::setRootPathMapping(int rootId, const QString &localPath)
{
    m_rootIdMappings[rootId] = QDir::fromNativeSeparators(localPath);
}

void DigikamDatabaseAdapter::setIdentifierMapping(const QString &identifier, const QString &localPath)
{
    m_identifierMappings[identifier] = QDir::fromNativeSeparators(localPath);
}

QString DigikamDatabaseAdapter::resolvedRootPath(int rootId, const QString &identifier, const QString &specificPath) const
{
    if (m_rootIdMappings.contains(rootId))
    {
        return m_rootIdMappings.value(rootId);
    }
    if (m_identifierMappings.contains(identifier))
    {
        return m_identifierMappings.value(identifier);
    }

    // Try extracting path from identifier e.g. "volumeid:?path=/home/user/Pictures"
    static const QRegularExpression pathRegex(QStringLiteral(R"(path=([^&]+))"));
    const auto match = pathRegex.match(identifier);
    if (match.hasMatch())
    {
        const QString extracted = match.captured(1);
        if (!extracted.isEmpty())
        {
            return QDir::fromNativeSeparators(extracted);
        }
    }

    if (!specificPath.isEmpty() && specificPath != QStringLiteral("/"))
    {
        return QDir::fromNativeSeparators(specificPath);
    }

    return QString();
}

QString DigikamDatabaseAdapter::resolveImagePath(int rootId,
                                                const QString &identifier,
                                                const QString &specificPath,
                                                const QString &albumRelativePath,
                                                const QString &imageName) const
{
    const QString base = resolvedRootPath(rootId, identifier, specificPath);
    QString rel = albumRelativePath;
    if (rel.startsWith(QLatin1Char('/')))
    {
        rel.remove(0, 1);
    }
    if (rel.endsWith(QLatin1Char('/')))
    {
        rel.chop(1);
    }

    QString fullPath;
    if (!base.isEmpty())
    {
        if (!rel.isEmpty())
        {
            fullPath = base + QLatin1Char('/') + rel + QLatin1Char('/') + imageName;
        }
        else
        {
            fullPath = base + QLatin1Char('/') + imageName;
        }
    }
    else
    {
        if (!rel.isEmpty())
        {
            fullPath = rel + QLatin1Char('/') + imageName;
        }
        else
        {
            fullPath = imageName;
        }
    }

    return QDir::cleanPath(QDir::fromNativeSeparators(fullPath));
}

QList<CollectionRecord> DigikamDatabaseAdapter::getCollections(const QSqlDatabase &db) const
{
    QList<CollectionRecord> list;
    if (!db.isOpen())
    {
        return list;
    }

    QSqlQuery query(QStringLiteral(
        "SELECT a.id, a.albumRoot, r.identifier, r.specificPath, r.label, a.relativePath, a.caption, r.type, COUNT(i.id) "
        "FROM Albums a "
        "JOIN AlbumRoots r ON a.albumRoot = r.id "
        "LEFT JOIN Images i ON a.id = i.album AND i.status = 1 "
        "GROUP BY a.id, a.albumRoot, r.identifier, r.specificPath, r.label, a.relativePath, a.caption, r.type "
        "ORDER BY r.label ASC, a.relativePath ASC;"
    ), db);

    while (query.next())
    {
        CollectionRecord rec;
        rec.id = query.value(0).toInt();
        const int rootId = query.value(1).toInt();
        const QString identifier = query.value(2).toString();
        const QString specificPath = query.value(3).toString();
        const QString rootLabel = query.value(4).toString();
        const QString relPath = query.value(5).toString();
        const QString caption = query.value(6).toString();
        const int rootType = query.value(7).toInt();
        rec.itemCount = query.value(8).toInt();

        // Map root type: 1 = hardwired/local, 2 = removable, 3 = network
        if (rootType == 2)
        {
            rec.collectionType = QStringLiteral("removable");
        }
        else if (rootType == 3)
        {
            rec.collectionType = QStringLiteral("network");
        }
        else
        {
            rec.collectionType = QStringLiteral("local");
        }

        const QString rootBase = resolvedRootPath(rootId, identifier, specificPath);
        if (!rootBase.isEmpty())
        {
            QString cleanRel = relPath;
            if (cleanRel.startsWith(QLatin1Char('/')))
            {
                cleanRel.remove(0, 1);
            }
            rec.rootPath = cleanRel.isEmpty() ? rootBase : QDir::cleanPath(rootBase + QLatin1Char('/') + cleanRel);
        }
        else
        {
            rec.rootPath = relPath;
        }

        if (!caption.isEmpty())
        {
            rec.name = caption;
        }
        else if (!relPath.isEmpty() && relPath != QStringLiteral("/"))
        {
            rec.name = QFileInfo(relPath).fileName();
        }
        else if (!rootLabel.isEmpty())
        {
            rec.name = rootLabel;
        }
        else
        {
            rec.name = QStringLiteral("Album #%1").arg(rec.id);
        }

        rec.isAvailable = true;
        rec.createdAt = QDateTime::currentDateTime();
        list.append(rec);
    }

    return list;
}

QList<CatalogItem> DigikamDatabaseAdapter::fetchAllItems(const QSqlDatabase &db, int limit, int offset) const
{
    QList<CatalogItem> list;
    if (!db.isOpen())
    {
        return list;
    }

    QString sql = QStringLiteral(
        "SELECT i.id, i.album, a.albumRoot, r.identifier, r.specificPath, a.relativePath, i.name, "
        "       i.fileSize, i.modificationDate, "
        "       info.creationDate, info.digitizationDate, info.width, info.height, info.rating, info.orientation, info.format, "
        "       meta.make, meta.model, meta.lens, meta.aperture, meta.focalLength, meta.exposureTime, meta.sensitivity, "
        "       GROUP_CONCAT(t.name, '|||') AS tag_names "
        "FROM Images i "
        "JOIN Albums a ON i.album = a.id "
        "JOIN AlbumRoots r ON a.albumRoot = r.id "
        "LEFT JOIN ImageInformation info ON i.id = info.imageid "
        "LEFT JOIN ImageMetadata meta ON i.id = meta.imageid "
        "LEFT JOIN ImageTags it ON i.id = it.imageid "
        "LEFT JOIN Tags t ON it.tagid = t.id "
        "WHERE i.status = 1 "
        "GROUP BY i.id "
        "ORDER BY COALESCE(info.creationDate, i.modificationDate) DESC"
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

    QSqlQuery query(sql, db);
    while (query.next())
    {
        CatalogItem item;
        const int imageId = query.value(0).toInt();
        const int albumId = query.value(1).toInt();
        const int rootId = query.value(2).toInt();
        const QString identifier = query.value(3).toString();
        const QString specificPath = query.value(4).toString();
        const QString relPath = query.value(5).toString();
        const QString imageName = query.value(6).toString();

        item.id = QString::number(imageId);
        item.collectionId = albumId;
        item.filePath = resolveImagePath(rootId, identifier, specificPath, relPath, imageName);
        item.fileName = imageName;
        item.title = QFileInfo(imageName).completeBaseName();
        item.fileSizeBytes = query.value(7).toLongLong();
        item.formattedFileSize = CatalogItem::formatBytes(item.fileSizeBytes);
        item.fileLastModified = query.value(8).toDateTime();
        if (item.fileLastModified.isValid())
        {
            item.formattedLastModified = item.fileLastModified.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
        }

        item.dateTaken = query.value(9).toDateTime();
        if (item.dateTaken.isValid())
        {
            item.formattedDate = item.dateTaken.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
        }

        const int width = query.value(11).toInt();
        const int height = query.value(12).toInt();
        item.dimensions = QSize(width, height);
        if (item.dimensions.isValid() && width > 0 && height > 0)
        {
            item.formattedDimensions = QStringLiteral("%1 × %2").arg(width).arg(height);
        }

        const int rawRating = query.value(13).toInt();
        item.rating = qBound(0, rawRating, 5);

        const QString format = query.value(15).toString().trimmed().toLower();
        item.mimeType = format.isEmpty() ? QStringLiteral("image/jpeg") : (QStringLiteral("image/") + format);

        const QString make = query.value(16).toString().trimmed();
        const QString model = query.value(17).toString().trimmed();
        if (!make.isEmpty() && !model.isEmpty())
        {
            item.cameraModel = model.startsWith(make, Qt::CaseInsensitive) ? model : (make + QLatin1Char(' ') + model);
        }
        else if (!model.isEmpty())
        {
            item.cameraModel = model;
        }
        else
        {
            item.cameraModel = make;
        }

        item.lensModel = query.value(18).toString().trimmed();
        item.aperture = formatAperture(query.value(19).toDouble());
        item.focalLength = formatFocalLength(query.value(20).toDouble());
        item.shutterSpeed = formatExposureTime(query.value(21).toDouble());
        item.iso = query.value(22).toInt();

        // Process concatenated tags
        const QString tagConcat = query.value(23).toString();
        if (!tagConcat.isEmpty())
        {
            item.tags = tagConcat.split(QStringLiteral("|||"), Qt::SkipEmptyParts);
        }

        item.previewUrl = QUrl::fromLocalFile(item.filePath);
        item.thumbnailUrl = QUrl(QStringLiteral("image://thumbnail/") + item.filePath);
        item.isDemoFixture = false;
        item.isMissing = false;

        list.append(item);
    }

    return list;
}

QList<CatalogItem> DigikamDatabaseAdapter::fetchItemsByAlbum(const QSqlDatabase &db, int albumId, int limit, int offset) const
{
    QList<CatalogItem> list;
    if (!db.isOpen() || albumId <= 0)
    {
        return list;
    }

    QString sql = QStringLiteral(
        "SELECT i.id, i.album, a.albumRoot, r.identifier, r.specificPath, a.relativePath, i.name, "
        "       i.fileSize, i.modificationDate, "
        "       info.creationDate, info.digitizationDate, info.width, info.height, info.rating, info.orientation, info.format, "
        "       meta.make, meta.model, meta.lens, meta.aperture, meta.focalLength, meta.exposureTime, meta.sensitivity, "
        "       GROUP_CONCAT(t.name, '|||') AS tag_names "
        "FROM Images i "
        "JOIN Albums a ON i.album = a.id "
        "JOIN AlbumRoots r ON a.albumRoot = r.id "
        "LEFT JOIN ImageInformation info ON i.id = info.imageid "
        "LEFT JOIN ImageMetadata meta ON i.id = meta.imageid "
        "LEFT JOIN ImageTags it ON i.id = it.imageid "
        "LEFT JOIN Tags t ON it.tagid = t.id "
        "WHERE i.status = 1 AND i.album = :albumId "
        "GROUP BY i.id "
        "ORDER BY COALESCE(info.creationDate, i.modificationDate) DESC"
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
    query.bindValue(QStringLiteral(":albumId"), albumId);

    if (query.exec())
    {
        while (query.next())
        {
            CatalogItem item;
            const int imageId = query.value(0).toInt();
            const int rootId = query.value(2).toInt();
            const QString identifier = query.value(3).toString();
            const QString specificPath = query.value(4).toString();
            const QString relPath = query.value(5).toString();
            const QString imageName = query.value(6).toString();

            item.id = QString::number(imageId);
            item.collectionId = albumId;
            item.filePath = resolveImagePath(rootId, identifier, specificPath, relPath, imageName);
            item.fileName = imageName;
            item.title = QFileInfo(imageName).completeBaseName();
            item.fileSizeBytes = query.value(7).toLongLong();
            item.formattedFileSize = CatalogItem::formatBytes(item.fileSizeBytes);
            item.fileLastModified = query.value(8).toDateTime();
            if (item.fileLastModified.isValid())
            {
                item.formattedLastModified = item.fileLastModified.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
            }

            item.dateTaken = query.value(9).toDateTime();
            if (item.dateTaken.isValid())
            {
                item.formattedDate = item.dateTaken.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
            }

            const int width = query.value(11).toInt();
            const int height = query.value(12).toInt();
            item.dimensions = QSize(width, height);
            if (item.dimensions.isValid() && width > 0 && height > 0)
            {
                item.formattedDimensions = QStringLiteral("%1 × %2").arg(width).arg(height);
            }

            const int rawRating = query.value(13).toInt();
            item.rating = qBound(0, rawRating, 5);

            const QString format = query.value(15).toString().trimmed().toLower();
            item.mimeType = format.isEmpty() ? QStringLiteral("image/jpeg") : (QStringLiteral("image/") + format);

            const QString make = query.value(16).toString().trimmed();
            const QString model = query.value(17).toString().trimmed();
            if (!make.isEmpty() && !model.isEmpty())
            {
                item.cameraModel = model.startsWith(make, Qt::CaseInsensitive) ? model : (make + QLatin1Char(' ') + model);
            }
            else if (!model.isEmpty())
            {
                item.cameraModel = model;
            }
            else
            {
                item.cameraModel = make;
            }

            item.lensModel = query.value(18).toString().trimmed();
            item.aperture = formatAperture(query.value(19).toDouble());
            item.focalLength = formatFocalLength(query.value(20).toDouble());
            item.shutterSpeed = formatExposureTime(query.value(21).toDouble());
            item.iso = query.value(22).toInt();

            const QString tagConcat = query.value(23).toString();
            if (!tagConcat.isEmpty())
            {
                item.tags = tagConcat.split(QStringLiteral("|||"), Qt::SkipEmptyParts);
            }

            item.previewUrl = QUrl::fromLocalFile(item.filePath);
            item.thumbnailUrl = QUrl(QStringLiteral("image://thumbnail/") + item.filePath);
            item.isDemoFixture = false;
            item.isMissing = false;

            list.append(item);
        }
    }

    return list;
}

CatalogItem DigikamDatabaseAdapter::fetchItemById(const QSqlDatabase &db, int imageId) const
{
    CatalogItem item;
    if (!db.isOpen() || imageId <= 0)
    {
        return item;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT i.id, i.album, a.albumRoot, r.identifier, r.specificPath, a.relativePath, i.name, "
        "       i.fileSize, i.modificationDate, "
        "       info.creationDate, info.digitizationDate, info.width, info.height, info.rating, info.orientation, info.format, "
        "       meta.make, meta.model, meta.lens, meta.aperture, meta.focalLength, meta.exposureTime, meta.sensitivity, "
        "       GROUP_CONCAT(t.name, '|||') AS tag_names "
        "FROM Images i "
        "JOIN Albums a ON i.album = a.id "
        "JOIN AlbumRoots r ON a.albumRoot = r.id "
        "LEFT JOIN ImageInformation info ON i.id = info.imageid "
        "LEFT JOIN ImageMetadata meta ON i.id = meta.imageid "
        "LEFT JOIN ImageTags it ON i.id = it.imageid "
        "LEFT JOIN Tags t ON it.tagid = t.id "
        "WHERE i.id = :id "
        "GROUP BY i.id LIMIT 1;"
    ));
    query.bindValue(QStringLiteral(":id"), imageId);

    if (query.exec() && query.next())
    {
        const int albumId = query.value(1).toInt();
        const int rootId = query.value(2).toInt();
        const QString identifier = query.value(3).toString();
        const QString specificPath = query.value(4).toString();
        const QString relPath = query.value(5).toString();
        const QString imageName = query.value(6).toString();

        item.id = QString::number(imageId);
        item.collectionId = albumId;
        item.filePath = resolveImagePath(rootId, identifier, specificPath, relPath, imageName);
        item.fileName = imageName;
        item.title = QFileInfo(imageName).completeBaseName();
        item.fileSizeBytes = query.value(7).toLongLong();
        item.formattedFileSize = CatalogItem::formatBytes(item.fileSizeBytes);
        item.fileLastModified = query.value(8).toDateTime();
        if (item.fileLastModified.isValid())
        {
            item.formattedLastModified = item.fileLastModified.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
        }

        item.dateTaken = query.value(9).toDateTime();
        if (item.dateTaken.isValid())
        {
            item.formattedDate = item.dateTaken.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
        }

        const int width = query.value(11).toInt();
        const int height = query.value(12).toInt();
        item.dimensions = QSize(width, height);
        if (item.dimensions.isValid() && width > 0 && height > 0)
        {
            item.formattedDimensions = QStringLiteral("%1 × %2").arg(width).arg(height);
        }

        const int rawRating = query.value(13).toInt();
        item.rating = qBound(0, rawRating, 5);

        const QString format = query.value(15).toString().trimmed().toLower();
        item.mimeType = format.isEmpty() ? QStringLiteral("image/jpeg") : (QStringLiteral("image/") + format);

        const QString make = query.value(16).toString().trimmed();
        const QString model = query.value(17).toString().trimmed();
        if (!make.isEmpty() && !model.isEmpty())
        {
            item.cameraModel = model.startsWith(make, Qt::CaseInsensitive) ? model : (make + QLatin1Char(' ') + model);
        }
        else if (!model.isEmpty())
        {
            item.cameraModel = model;
        }
        else
        {
            item.cameraModel = make;
        }

        item.lensModel = query.value(18).toString().trimmed();
        item.aperture = formatAperture(query.value(19).toDouble());
        item.focalLength = formatFocalLength(query.value(20).toDouble());
        item.shutterSpeed = formatExposureTime(query.value(21).toDouble());
        item.iso = query.value(22).toInt();

        const QString tagConcat = query.value(23).toString();
        if (!tagConcat.isEmpty())
        {
            item.tags = tagConcat.split(QStringLiteral("|||"), Qt::SkipEmptyParts);
        }

        item.previewUrl = QUrl::fromLocalFile(item.filePath);
        item.thumbnailUrl = QUrl(QStringLiteral("image://thumbnail/") + item.filePath);
    }

    return item;
}

CatalogItem DigikamDatabaseAdapter::fetchItemByPath(const QSqlDatabase &db, const QString &filePath) const
{
    CatalogItem item;
    if (!db.isOpen() || filePath.isEmpty())
    {
        return item;
    }

    const QFileInfo fi(filePath);
    const QString fileName = fi.fileName();

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id FROM Images WHERE name = :name LIMIT 10;"
    ));
    query.bindValue(QStringLiteral(":name"), fileName);

    if (query.exec())
    {
        const QString normalizedTarget = QDir::cleanPath(QDir::fromNativeSeparators(filePath));
        while (query.next())
        {
            const int id = query.value(0).toInt();
            CatalogItem candidate = fetchItemById(db, id);
            if (candidate.filePath.isEmpty() || candidate.filePath == normalizedTarget)
            {
                return candidate;
            }
        }
    }

    return item;
}

int DigikamDatabaseAdapter::itemCount(const QSqlDatabase &db) const
{
    if (!db.isOpen())
    {
        return 0;
    }

    QSqlQuery query(QStringLiteral("SELECT COUNT(*) FROM Images WHERE status = 1;"), db);
    if (query.next())
    {
        return query.value(0).toInt();
    }
    return 0;
}

bool DigikamDatabaseAdapter::updateItemRating(const QSqlDatabase &db, int imageId, int rating)
{
    if (!db.isOpen() || imageId <= 0)
    {
        return false;
    }

    const int clampedRating = qBound(0, rating, 5);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO ImageInformation (imageid, rating) "
        "VALUES (:id, :rating) "
        "ON CONFLICT(imageid) DO UPDATE SET rating = excluded.rating;"
    ));
    query.bindValue(QStringLiteral(":id"), imageId);
    query.bindValue(QStringLiteral(":rating"), clampedRating);

    return query.exec();
}

bool DigikamDatabaseAdapter::updateItemRatingByPath(const QSqlDatabase &db, const QString &filePath, int rating)
{
    const CatalogItem item = fetchItemByPath(db, filePath);
    if (item.id.isEmpty())
    {
        return false;
    }
    return updateItemRating(db, item.id.toInt(), rating);
}

bool DigikamDatabaseAdapter::setItemTags(const QSqlDatabase &db, int imageId, const QStringList &tags)
{
    if (!db.isOpen() || imageId <= 0)
    {
        return false;
    }

    QSqlQuery deleteQuery(db);
    deleteQuery.prepare(QStringLiteral("DELETE FROM ImageTags WHERE imageid = :id;"));
    deleteQuery.bindValue(QStringLiteral(":id"), imageId);
    if (!deleteQuery.exec())
    {
        return false;
    }

    QSqlQuery tagSelectQuery(db);
    tagSelectQuery.prepare(QStringLiteral("SELECT id FROM Tags WHERE name = :name;"));

    QSqlQuery tagInsertQuery(db);
    tagInsertQuery.prepare(QStringLiteral("INSERT INTO Tags (name) VALUES (:name);"));

    QSqlQuery linkQuery(db);
    linkQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO ImageTags (imageid, tagid) VALUES (:imageid, :tagid);"));

    for (const QString &tag : tags)
    {
        const QString trimmed = tag.trimmed();
        if (trimmed.isEmpty())
        {
            continue;
        }

        int tagId = 0;
        tagSelectQuery.bindValue(QStringLiteral(":name"), trimmed);
        if (tagSelectQuery.exec() && tagSelectQuery.next())
        {
            tagId = tagSelectQuery.value(0).toInt();
        }
        else
        {
            tagInsertQuery.bindValue(QStringLiteral(":name"), trimmed);
            if (tagInsertQuery.exec())
            {
                tagId = tagInsertQuery.lastInsertId().toInt();
            }
        }

        if (tagId > 0)
        {
            linkQuery.bindValue(QStringLiteral(":imageid"), imageId);
            linkQuery.bindValue(QStringLiteral(":tagid"), tagId);
            linkQuery.exec();
        }
    }

    return true;
}

bool DigikamDatabaseAdapter::setItemTagsByPath(const QSqlDatabase &db, const QString &filePath, const QStringList &tags)
{
    const CatalogItem item = fetchItemByPath(db, filePath);
    if (item.id.isEmpty())
    {
        return false;
    }
    return setItemTags(db, item.id.toInt(), tags);
}

QStringList DigikamDatabaseAdapter::allTags(const QSqlDatabase &db) const
{
    QStringList tags;
    if (!db.isOpen())
    {
        return tags;
    }

    QSqlQuery query(QStringLiteral("SELECT DISTINCT name FROM Tags WHERE name IS NOT NULL AND name != '' ORDER BY name ASC;"), db);
    while (query.next())
    {
        tags.append(query.value(0).toString());
    }
    return tags;
}

bool DigikamDatabaseAdapter::importIntoCatalog(const QSqlDatabase &digikamDb, CatalogDatabase *targetDb)
{
    if (!digikamDb.isOpen() || !targetDb || !targetDb->isOpen())
    {
        return false;
    }

    const auto collections = getCollections(digikamDb);
    for (const auto &coll : collections)
    {
        const int newCollId = targetDb->addOrGetCollection(coll.rootPath, coll.name, coll.collectionType);
        const auto items = fetchItemsByAlbum(digikamDb, coll.id);
        if (!items.isEmpty())
        {
            targetDb->insertOrUpdateItems(items, newCollId);
        }
    }
    return true;
}

QString DigikamDatabaseAdapter::formatExposureTime(double seconds)
{
    if (seconds <= 0.0)
    {
        return QString();
    }

    if (seconds < 1.0)
    {
        const int denominator = static_cast<int>(std::round(1.0 / seconds));
        return QStringLiteral("1/%1s").arg(denominator);
    }

    return QString::asprintf("%.1fs", seconds);
}

QString DigikamDatabaseAdapter::formatAperture(double aperture)
{
    if (aperture <= 0.0)
    {
        return QString();
    }
    return QString::asprintf("f/%.1f", aperture);
}

QString DigikamDatabaseAdapter::formatFocalLength(double focalLength)
{
    if (focalLength <= 0.0)
    {
        return QString();
    }
    return QString::asprintf("%.0f mm", focalLength);
}

} // namespace PhotoCarta
