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

struct CollectionRecord;
class CatalogDatabase;

/**
 * @brief Adapter for upstream digiKam SQLite database schema (digikam4.db).
 *
 * Provides bidirectional translation between digiKam's normalized relational
 * schema (AlbumRoots, Albums, Images, ImageInformation, ImageMetadata, Tags, ImageTags)
 * and PhotoCarta's presentation catalog items and collection records.
 *
 * Applies performance optimizations:
 * - Single-pass JOIN with GROUP_CONCAT to eliminate N+1 queries for tags
 * - Bounded paging for virtualized memory footprint
 * - High-speed path resolution with customizable volume root mappings
 */
class DigikamDatabaseAdapter : public QObject
{
    Q_OBJECT

public:
    explicit DigikamDatabaseAdapter(QObject *parent = nullptr);
    ~DigikamDatabaseAdapter() override = default;

    /// Checks if the given SQLite database contains the digiKam core schema tables.
    [[nodiscard]] static bool isDigikamDatabase(const QSqlDatabase &db);

    /// Creates a complete, valid digiKam 4.x core database schema in the given database.
    static bool createDigikamSchema(QSqlDatabase &db);

    /// Maps an AlbumRoot identifier or root ID to an absolute local folder path.
    void setRootPathMapping(int rootId, const QString &localPath);
    void setIdentifierMapping(const QString &identifier, const QString &localPath);
    [[nodiscard]] QString resolvedRootPath(int rootId, const QString &identifier, const QString &specificPath) const;

    /// Reconstructs the full filesystem path for an image in a digiKam database.
    [[nodiscard]] QString resolveImagePath(int rootId,
                                           const QString &identifier,
                                           const QString &specificPath,
                                           const QString &albumRelativePath,
                                           const QString &imageName) const;

    /// Reads digiKam AlbumRoots and Albums as PhotoCarta CollectionRecords.
    [[nodiscard]] QList<CollectionRecord> getCollections(const QSqlDatabase &db) const;

    /// Reads digiKam Images, metadata, and tags as CatalogItems.
    [[nodiscard]] QList<CatalogItem> fetchAllItems(const QSqlDatabase &db, int limit = -1, int offset = 0) const;
    [[nodiscard]] QList<CatalogItem> fetchItemsByAlbum(const QSqlDatabase &db, int albumId, int limit = -1, int offset = 0) const;
    [[nodiscard]] CatalogItem fetchItemById(const QSqlDatabase &db, int imageId) const;
    [[nodiscard]] CatalogItem fetchItemByPath(const QSqlDatabase &db, const QString &filePath) const;
    [[nodiscard]] int itemCount(const QSqlDatabase &db) const;

    /// Updates item metadata in digiKam tables.
    bool updateItemRating(const QSqlDatabase &db, int imageId, int rating);
    bool updateItemRatingByPath(const QSqlDatabase &db, const QString &filePath, int rating);
    bool setItemTags(const QSqlDatabase &db, int imageId, const QStringList &tags);
    bool setItemTagsByPath(const QSqlDatabase &db, const QString &filePath, const QStringList &tags);
    [[nodiscard]] QStringList allTags(const QSqlDatabase &db) const;

    /// Imports all collections, images, metadata, and tags from a digiKam database into a PhotoCarta database.
    bool importIntoCatalog(const QSqlDatabase &digikamDb, CatalogDatabase *targetDb);

    /// Formats exposure time from seconds (e.g. 0.004 -> "1/250s", 0.5 -> "0.5s", 2.0 -> "2.0s").
    [[nodiscard]] static QString formatExposureTime(double seconds);

    /// Formats aperture value (e.g. 2.8 -> "f/2.8").
    [[nodiscard]] static QString formatAperture(double aperture);

    /// Formats focal length (e.g. 50.0 -> "50 mm").
    [[nodiscard]] static QString formatFocalLength(double focalLength);

private:
    QHash<int, QString> m_rootIdMappings;
    QHash<QString, QString> m_identifierMappings;
};

} // namespace PhotoCarta
