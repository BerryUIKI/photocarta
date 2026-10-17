// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalogmodel.h"
#include "database/catalogdatabase.h"
#include "metadata/exifparser.h"

#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QSet>
#include <QUuid>

namespace PhotoCarta
{

CatalogModel::CatalogModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CatalogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(m_items.size());
}

QVariant CatalogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
    {
        return {};
    }

    const auto &item = m_items.at(index.row());
    switch (role)
    {
    case IdRole:
        return item.id;
    case FilePathRole:
        return item.filePath;
    case FileNameRole:
        return item.fileName;
    case TitleRole:
        return item.title;
    case FileSizeRole:
        return item.fileSizeBytes;
    case FormattedFileSizeRole:
        return item.formattedFileSize;
    case MimeTypeRole:
        return item.mimeType;
    case DateTakenRole:
        return item.dateTaken;
    case FormattedDateRole:
        return item.formattedDate;
    case FileLastModifiedRole:
        return item.fileLastModified;
    case FormattedLastModifiedRole:
        return item.formattedLastModified;
    case WidthRole:
        return item.dimensions.width();
    case HeightRole:
        return item.dimensions.height();
    case DimensionsRole:
        return item.formattedDimensions;
    case CameraModelRole:
        return item.cameraModel;
    case LensModelRole:
        return item.lensModel;
    case FocalLengthRole:
        return item.focalLength;
    case ApertureRole:
        return item.aperture;
    case ShutterSpeedRole:
        return item.shutterSpeed;
    case IsoRole:
        return item.iso;
    case RatingRole:
        return item.rating;
    case IsFlaggedRole:
        return item.isFlagged;
    case TagsRole:
        return item.tags;
    case CaptionRole:
        return item.caption;
    case ColorLabelRole:
        return item.colorLabel;
    case ThumbnailUrlRole:
        return item.thumbnailUrl;
    case PreviewUrlRole:
        return item.previewUrl;
    case IsDemoFixtureRole:
        return item.isDemoFixture;
    case IsMissingRole:
        return item.isMissing;
    case CollectionIdRole:
        return item.collectionId;
    case ItemMapRole:
        return item.toVariantMap();
    case Qt::DisplayRole:
        return item.title.isEmpty() ? item.fileName : item.title;
    default:
        return {};
    }
}

QHash<int, QByteArray> CatalogModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {FilePathRole, "filePath"},
        {FileNameRole, "fileName"},
        {TitleRole, "title"},
        {CaptionRole, "caption"},
        {ColorLabelRole, "colorLabel"},
        {FileSizeRole, "fileSizeBytes"},
        {FormattedFileSizeRole, "formattedFileSize"},
        {MimeTypeRole, "mimeType"},
        {DateTakenRole, "dateTaken"},
        {FormattedDateRole, "formattedDate"},
        {FileLastModifiedRole, "fileLastModified"},
        {FormattedLastModifiedRole, "formattedLastModified"},
        {WidthRole, "width"},
        {HeightRole, "height"},
        {DimensionsRole, "formattedDimensions"},
        {CameraModelRole, "cameraModel"},
        {LensModelRole, "lensModel"},
        {FocalLengthRole, "focalLength"},
        {ApertureRole, "aperture"},
        {ShutterSpeedRole, "shutterSpeed"},
        {IsoRole, "iso"},
        {RatingRole, "rating"},
        {IsFlaggedRole, "isFlagged"},
        {TagsRole, "tags"},
        {ThumbnailUrlRole, "thumbnailUrl"},
        {PreviewUrlRole, "previewUrl"},
        {IsDemoFixtureRole, "isDemoFixture"},
        {IsMissingRole, "isMissing"},
        {CollectionIdRole, "collectionId"},
        {ItemMapRole, "itemMap"}
    };
}

int CatalogModel::count() const
{
    return static_cast<int>(m_items.size());
}

bool CatalogModel::hasDemoContent() const
{
    for (const auto &item : m_items)
    {
        if (item.isDemoFixture)
        {
            return true;
        }
    }
    return false;
}

const QList<CatalogItem> &CatalogModel::items() const
{
    return m_items;
}

const CatalogItem *CatalogModel::itemAt(int index) const
{
    if (index >= 0 && index < m_items.size())
    {
        return &m_items.at(index);
    }
    return nullptr;
}

QVariantMap CatalogModel::get(int index) const
{
    if (index >= 0 && index < m_items.size())
    {
        return m_items.at(index).toVariantMap();
    }
    return {};
}

void CatalogModel::appendItem(const CatalogItem &item)
{
    const int newRow = static_cast<int>(m_items.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_items.append(item);
    endInsertRows();
}

void CatalogModel::loadDemoFixtures()
{
    beginResetModel();
    m_items.clear();

    CatalogItem item1;
    item1.id = QStringLiteral("demo-001");
    item1.filePath = QStringLiteral("qrc:/assets/demo/demo_landscape.svg");
    item1.fileName = QStringLiteral("Alpine_Sunrise_Oeschinen.jpg");
    item1.title = QStringLiteral("Alpine Sunrise at Lake Oeschinen");
    item1.caption = QStringLiteral("Panoramic morning light over turquoise alpine lake waters surrounded by sheer limestone cliffs in the Bernese Alps.");
    item1.colorLabel = 2; // Yellow
    item1.fileSizeBytes = 28430000;
    item1.formattedFileSize = CatalogItem::formatBytes(item1.fileSizeBytes);
    item1.mimeType = QStringLiteral("image/jpeg");
    item1.dateTaken = QDateTime::fromString(QStringLiteral("2026-10-12 06:45:10"), QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    item1.formattedDate = item1.dateTaken.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    item1.dimensions = QSize(7008, 4672);
    item1.formattedDimensions = QStringLiteral("7008 x 4672 (32.7 MP)");
    item1.cameraModel = QStringLiteral("Sony Alpha 7 IV");
    item1.lensModel = QStringLiteral("FE 24-70mm F2.8 GM II");
    item1.focalLength = QStringLiteral("28 mm");
    item1.aperture = QStringLiteral("f/8.0");
    item1.shutterSpeed = QStringLiteral("1/250s");
    item1.iso = 100;
    item1.rating = 5;
    item1.isFlagged = true;
    item1.tags = {QStringLiteral("Landscape"), QStringLiteral("Mountain"), QStringLiteral("Sunrise"), QStringLiteral("Demo")};
    item1.thumbnailUrl = QUrl(QStringLiteral("qrc:/assets/demo/demo_landscape.svg"));
    item1.previewUrl = QUrl(QStringLiteral("qrc:/assets/demo/demo_landscape.svg"));
    item1.isDemoFixture = true;
    m_items.append(item1);

    CatalogItem item2;
    item2.id = QStringLiteral("demo-002");
    item2.filePath = QStringLiteral("qrc:/assets/demo/demo_portrait.svg");
    item2.fileName = QStringLiteral("Studio_Profile_AmberRim.jpg");
    item2.title = QStringLiteral("Studio Portrait in Amber & Cyan");
    item2.caption = QStringLiteral("High-fashion editorial portrait with complementary warm amber edge lighting and cool cyan ambient fill.");
    item2.colorLabel = 1; // Red
    item2.fileSizeBytes = 35120000;
    item2.formattedFileSize = CatalogItem::formatBytes(item2.fileSizeBytes);
    item2.mimeType = QStringLiteral("image/jpeg");
    item2.dateTaken = QDateTime::fromString(QStringLiteral("2026-10-13 14:15:32"), QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    item2.formattedDate = item2.dateTaken.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    item2.dimensions = QSize(8192, 5464);
    item2.formattedDimensions = QStringLiteral("8192 x 5464 (44.8 MP)");
    item2.cameraModel = QStringLiteral("Canon EOS R5");
    item2.lensModel = QStringLiteral("RF 85mm F1.2 L USM");
    item2.focalLength = QStringLiteral("85 mm");
    item2.aperture = QStringLiteral("f/1.4");
    item2.shutterSpeed = QStringLiteral("1/500s");
    item2.iso = 100;
    item2.rating = 5;
    item2.isFlagged = false;
    item2.tags = {QStringLiteral("Portrait"), QStringLiteral("Studio"), QStringLiteral("Silhouette"), QStringLiteral("Demo")};
    item2.thumbnailUrl = QUrl(QStringLiteral("qrc:/assets/demo/demo_portrait.svg"));
    item2.previewUrl = QUrl(QStringLiteral("qrc:/assets/demo/demo_portrait.svg"));
    item2.isDemoFixture = true;
    m_items.append(item2);

    CatalogItem item3;
    item3.id = QStringLiteral("demo-003");
    item3.filePath = QStringLiteral("qrc:/assets/demo/demo_architecture.svg");
    item3.fileName = QStringLiteral("Geometric_Concrete_Pavilion.jpg");
    item3.title = QStringLiteral("Geometric Concrete Pavilion");
    item3.caption = QStringLiteral("Brutalist architecture study emphasizing diagonal cantilever lines and monolithic concrete textures.");
    item3.colorLabel = 3; // Green
    item3.fileSizeBytes = 42680000;
    item3.formattedFileSize = CatalogItem::formatBytes(item3.fileSizeBytes);
    item3.mimeType = QStringLiteral("image/jpeg");
    item3.dateTaken = QDateTime::fromString(QStringLiteral("2026-10-14 11:20:45"), QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    item3.formattedDate = item3.dateTaken.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    item3.dimensions = QSize(8256, 5504);
    item3.formattedDimensions = QStringLiteral("8256 x 5504 (45.4 MP)");
    item3.cameraModel = QStringLiteral("Nikon Z8");
    item3.lensModel = QStringLiteral("NIKKOR Z 14-30mm f/4 S");
    item3.focalLength = QStringLiteral("18 mm");
    item3.aperture = QStringLiteral("f/11.0");
    item3.shutterSpeed = QStringLiteral("1/160s");
    item3.iso = 64;
    item3.rating = 4;
    item3.isFlagged = true;
    item3.tags = {QStringLiteral("Architecture"), QStringLiteral("Minimalism"), QStringLiteral("Urban"), QStringLiteral("Demo")};
    item3.thumbnailUrl = QUrl(QStringLiteral("qrc:/assets/demo/demo_architecture.svg"));
    item3.previewUrl = QUrl(QStringLiteral("qrc:/assets/demo/demo_architecture.svg"));
    item3.isDemoFixture = true;
    m_items.append(item3);

    CatalogItem item4;
    item4.id = QStringLiteral("demo-004");
    item4.filePath = QStringLiteral("qrc:/assets/demo/demo_street.svg");
    item4.fileName = QStringLiteral("Neon_Rain_Alleyway.jpg");
    item4.title = QStringLiteral("Neon Rain in Shinjuku Alleyway");
    item4.caption = QStringLiteral("Atmospheric rainy night in Tokyo reflecting glowing cybernetic signage off wet pavement.");
    item4.colorLabel = 4; // Blue
    item4.fileSizeBytes = 22840000;
    item4.formattedFileSize = CatalogItem::formatBytes(item4.fileSizeBytes);
    item4.mimeType = QStringLiteral("image/jpeg");
    item4.dateTaken = QDateTime::fromString(QStringLiteral("2026-10-14 21:05:19"), QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    item4.formattedDate = item4.dateTaken.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    item4.dimensions = QSize(7728, 5152);
    item4.formattedDimensions = QStringLiteral("7728 x 5152 (39.8 MP)");
    item4.cameraModel = QStringLiteral("Fujifilm X-T5");
    item4.lensModel = QStringLiteral("XF 33mm F1.4 R LM WR");
    item4.focalLength = QStringLiteral("33 mm");
    item4.aperture = QStringLiteral("f/1.4");
    item4.shutterSpeed = QStringLiteral("1/125s");
    item4.iso = 1600;
    item4.rating = 4;
    item4.isFlagged = false;
    item4.tags = {QStringLiteral("Street"), QStringLiteral("Night"), QStringLiteral("Rain"), QStringLiteral("Demo")};
    item4.thumbnailUrl = QUrl(QStringLiteral("qrc:/assets/demo/demo_street.svg"));
    item4.previewUrl = QUrl(QStringLiteral("qrc:/assets/demo/demo_street.svg"));
    item4.isDemoFixture = true;
    m_items.append(item4);

    CatalogItem item5;
    item5.id = QStringLiteral("demo-005");
    item5.filePath = QStringLiteral("qrc:/assets/demo/demo_macro.svg");
    item5.fileName = QStringLiteral("Monstera_Dew_Drops_Macro.jpg");
    item5.title = QStringLiteral("Monstera Dew Drops Macro");
    item5.caption = QStringLiteral("Extreme macro detail capturing pristine condensation droplets along the deep green vein structure of a tropical leaf.");
    item5.colorLabel = 5; // Purple
    item5.fileSizeBytes = 54200000;
    item5.formattedFileSize = CatalogItem::formatBytes(item5.fileSizeBytes);
    item5.mimeType = QStringLiteral("image/jpeg");
    item5.dateTaken = QDateTime::fromString(QStringLiteral("2026-10-15 09:12:04"), QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    item5.formattedDate = item5.dateTaken.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    item5.dimensions = QSize(9504, 6336);
    item5.formattedDimensions = QStringLiteral("9504 x 6336 (60.2 MP)");
    item5.cameraModel = QStringLiteral("Sony Alpha 7R V");
    item5.lensModel = QStringLiteral("FE 90mm F2.8 Macro G OSS");
    item5.focalLength = QStringLiteral("90 mm");
    item5.aperture = QStringLiteral("f/5.6");
    item5.shutterSpeed = QStringLiteral("1/200s");
    item5.iso = 200;
    item5.rating = 3;
    item5.isFlagged = false;
    item5.tags = {QStringLiteral("Macro"), QStringLiteral("Nature"), QStringLiteral("Botanical"), QStringLiteral("Demo")};
    item5.thumbnailUrl = QUrl(QStringLiteral("qrc:/assets/demo/demo_macro.svg"));
    item5.previewUrl = QUrl(QStringLiteral("qrc:/assets/demo/demo_macro.svg"));
    item5.isDemoFixture = true;
    m_items.append(item5);

    CatalogItem item6;
    item6.id = QStringLiteral("demo-006");
    item6.filePath = QStringLiteral("qrc:/assets/demo/demo_astro.svg");
    item6.fileName = QStringLiteral("Milky_Way_Arc_Canyon.jpg");
    item6.title = QStringLiteral("Milky Way Arc over Arch Canyon");
    item6.caption = QStringLiteral("Untouched dark sky panoramic long exposure revealing the galactic core framed through natural desert sandstone arches.");
    item6.colorLabel = 2; // Yellow
    item6.fileSizeBytes = 18950000;
    item6.formattedFileSize = CatalogItem::formatBytes(item6.fileSizeBytes);
    item6.mimeType = QStringLiteral("image/jpeg");
    item6.dateTaken = QDateTime::fromString(QStringLiteral("2026-10-15 23:48:12"), QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    item6.formattedDate = item6.dateTaken.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    item6.dimensions = QSize(4240, 2832);
    item6.formattedDimensions = QStringLiteral("4240 x 2832 (12.0 MP)");
    item6.cameraModel = QStringLiteral("Sony Alpha 7S III");
    item6.lensModel = QStringLiteral("FE 20mm F1.8 G");
    item6.focalLength = QStringLiteral("20 mm");
    item6.aperture = QStringLiteral("f/1.8");
    item6.shutterSpeed = QStringLiteral("20s");
    item6.iso = 3200;
    item6.rating = 5;
    item6.isFlagged = true;
    item6.tags = {QStringLiteral("Astro"), QStringLiteral("Night"), QStringLiteral("Landscape"), QStringLiteral("Demo")};
    item6.thumbnailUrl = QUrl(QStringLiteral("qrc:/assets/demo/demo_astro.svg"));
    item6.previewUrl = QUrl(QStringLiteral("qrc:/assets/demo/demo_astro.svg"));
    item6.isDemoFixture = true;
    m_items.append(item6);

    for (auto &item : m_items)
    {
        item.fileLastModified = item.dateTaken;
        item.formattedLastModified = item.formattedDate;
    }

    endResetModel();
    emit countChanged();
    emit availableTagsChanged();
    emit sessionLoaded(static_cast<int>(m_items.size()));
}

CatalogItem CatalogModel::inspectLocalFile(const QString &filePath)
{
    CatalogItem item;
    QFileInfo info(filePath);
    item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item.filePath = info.absoluteFilePath();
    item.fileName = info.fileName();
    item.title = info.completeBaseName();
    item.fileSizeBytes = info.size();
    item.formattedFileSize = CatalogItem::formatBytes(item.fileSizeBytes);

    QMimeDatabase mimeDb;
    item.mimeType = mimeDb.mimeTypeForFile(info).name();

    item.fileLastModified = info.lastModified();
    item.formattedLastModified = item.fileLastModified.isValid()
        ? item.fileLastModified.toString(QStringLiteral("yyyy-MM-dd HH:mm"))
        : QStringLiteral("-");

    // Capture date: Unknown without EXIF parser in prototype
    item.dateTaken = QDateTime();
    item.formattedDate = QString();

    // Bounded header-only reading without full image decoding on GUI thread
    QImageReader reader(item.filePath);
    const QSize sz = reader.size();
    if (sz.isValid() && sz.width() > 0 && sz.height() > 0)
    {
        item.dimensions = sz;
        const double megapixels = (static_cast<double>(sz.width()) * static_cast<double>(sz.height())) / 1000000.0;
        item.formattedDimensions = QString::asprintf("%d x %d (%.1f MP)", sz.width(), sz.height(), megapixels);
    }
    else
    {
        item.dimensions = QSize(0, 0);
        item.formattedDimensions = QStringLiteral("Unknown dimensions");
    }

    item.rating = 0;
    item.isFlagged = false;
    item.tags = {QStringLiteral("Imported")};
    item.thumbnailUrl = QUrl(QStringLiteral("image://thumbnail/") + item.filePath);
    item.previewUrl = QUrl::fromLocalFile(item.filePath);
    item.isDemoFixture = false; // Real local file!

    // Parse EXIF metadata
    const ExifData exif = ExifParser::parseFile(item.filePath);
    if (exif.isValid)
    {
        if (!exif.cameraModel.isEmpty())
        {
            item.cameraModel = exif.cameraModel;
        }
        if (!exif.lensModel.isEmpty())
        {
            item.lensModel = exif.lensModel;
        }
        if (!exif.focalLength.isEmpty())
        {
            item.focalLength = exif.focalLength;
        }
        if (!exif.aperture.isEmpty())
        {
            item.aperture = exif.aperture;
        }
        if (!exif.shutterSpeed.isEmpty())
        {
            item.shutterSpeed = exif.shutterSpeed;
        }
        if (exif.iso > 0)
        {
            item.iso = exif.iso;
        }
        if (exif.dateTaken.isValid())
        {
            item.dateTaken = exif.dateTaken;
            item.formattedDate = exif.dateTaken.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
        }
        if (item.dimensions.isEmpty() && exif.dimensions.isValid())
        {
            item.dimensions = exif.dimensions;
            item.formattedDimensions = QString::asprintf("%d × %d", exif.dimensions.width(), exif.dimensions.height());
        }
    }

    return item;
}

void CatalogModel::setDatabase(CatalogDatabase *database)
{
    m_database = database;
}

CatalogDatabase *CatalogModel::database() const
{
    return m_database;
}

void CatalogModel::openUrls(const QList<QUrl> &urls)
{
    QStringList paths;
    paths.reserve(urls.size());
    for (const auto &url : urls)
    {
        if (url.isLocalFile())
        {
            paths.append(url.toLocalFile());
        }
    }
    addFilePaths(paths);
}

void CatalogModel::addItems(const QList<CatalogItem> &items)
{
    if (items.isEmpty())
    {
        return;
    }

    const int firstRow = static_cast<int>(m_items.size());
    const int lastRow = firstRow + static_cast<int>(items.size()) - 1;

    beginInsertRows(QModelIndex(), firstRow, lastRow);
    m_items.append(items);
    endInsertRows();

    if (m_database && m_database->isOpen())
    {
        m_database->insertOrUpdateItems(items);
    }

    emit countChanged();
    emit availableTagsChanged();
    emit sessionLoaded(static_cast<int>(m_items.size()));
}

void CatalogModel::addFilePaths(const QStringList &filePaths)
{
    if (filePaths.isEmpty())
    {
        return;
    }

    QList<CatalogItem> newItems;
    newItems.reserve(filePaths.size());

    for (const auto &path : filePaths)
    {
        QFileInfo fi(path);
        if (!fi.exists() || !fi.isFile())
        {
            emit fileLoadError(path, QStringLiteral("File not found or inaccessible"));
            continue;
        }

        CatalogItem item = inspectLocalFile(path);
        newItems.append(item);
    }

    addItems(newItems);
}

void CatalogModel::updateItemTitle(int index, const QString &title)
{
    if (index < 0 || index >= m_items.size())
    {
        return;
    }

    if (m_items[index].title == title)
    {
        return;
    }

    m_items[index].title = title;
    const QModelIndex modelIdx = this->index(index, 0);
    emit dataChanged(modelIdx, modelIdx, {TitleRole, Qt::DisplayRole, ItemMapRole});

    if (m_database && m_database->isOpen() && !m_items[index].isDemoFixture)
    {
        m_database->updateItemTitle(m_items[index].filePath, title);
    }
}

void CatalogModel::updateItemCaption(int index, const QString &caption)
{
    if (index < 0 || index >= m_items.size())
    {
        return;
    }

    if (m_items[index].caption == caption)
    {
        return;
    }

    m_items[index].caption = caption;
    const QModelIndex modelIdx = this->index(index, 0);
    emit dataChanged(modelIdx, modelIdx, {CaptionRole, ItemMapRole});

    if (m_database && m_database->isOpen() && !m_items[index].isDemoFixture)
    {
        m_database->updateItemCaption(m_items[index].filePath, caption);
    }
}

void CatalogModel::updateItemColorLabel(int index, int colorLabel)
{
    if (index < 0 || index >= m_items.size())
    {
        return;
    }

    const int clamped = qBound(0, colorLabel, 5);
    if (m_items[index].colorLabel == clamped)
    {
        return;
    }

    m_items[index].colorLabel = clamped;
    const QModelIndex modelIdx = this->index(index, 0);
    emit dataChanged(modelIdx, modelIdx, {ColorLabelRole, ItemMapRole});

    if (m_database && m_database->isOpen() && !m_items[index].isDemoFixture)
    {
        m_database->updateItemColorLabel(m_items[index].filePath, clamped);
    }
}

void CatalogModel::updateItemRating(int index, int rating)
{
    if (index < 0 || index >= m_items.size())
    {
        return;
    }

    if (m_items[index].rating == rating)
    {
        return;
    }

    m_items[index].rating = rating;
    const QModelIndex modelIdx = this->index(index, 0);
    emit dataChanged(modelIdx, modelIdx, {RatingRole, ItemMapRole});

    if (m_database && m_database->isOpen() && !m_items[index].isDemoFixture)
    {
        m_database->updateItemRating(m_items[index].filePath, rating);
    }
}

void CatalogModel::updateItemFlagged(int index, bool flagged)
{
    if (index < 0 || index >= m_items.size())
    {
        return;
    }

    if (m_items[index].isFlagged == flagged)
    {
        return;
    }

    m_items[index].isFlagged = flagged;
    const QModelIndex modelIdx = this->index(index, 0);
    emit dataChanged(modelIdx, modelIdx, {IsFlaggedRole, ItemMapRole});

    if (m_database && m_database->isOpen() && !m_items[index].isDemoFixture)
    {
        m_database->updateItemFlagged(m_items[index].filePath, flagged);
    }
}

void CatalogModel::updateItemTags(int index, const QStringList &tags)
{
    if (index < 0 || index >= m_items.size())
    {
        return;
    }

    m_items[index].tags = tags;
    const QModelIndex modelIdx = this->index(index, 0);
    emit dataChanged(modelIdx, modelIdx, {TagsRole, ItemMapRole});
    emit availableTagsChanged();

    if (m_database && m_database->isOpen() && !m_items[index].isDemoFixture)
    {
        m_database->setItemTags(m_items[index].filePath, tags);
    }
}

void CatalogModel::addTagToItem(int index, const QString &tag)
{
    if (index < 0 || index >= m_items.size())
    {
        return;
    }

    const QString trimmed = tag.trimmed();
    if (trimmed.isEmpty())
    {
        return;
    }

    for (const auto &existing : m_items[index].tags)
    {
        if (existing.compare(trimmed, Qt::CaseInsensitive) == 0)
        {
            return;
        }
    }

    QStringList updated = m_items[index].tags;
    updated.append(trimmed);
    updateItemTags(index, updated);
}

void CatalogModel::removeTagFromItem(int index, const QString &tag)
{
    if (index < 0 || index >= m_items.size())
    {
        return;
    }

    const QString trimmed = tag.trimmed();
    QStringList updated;
    bool found = false;
    for (const auto &existing : m_items[index].tags)
    {
        if (existing.compare(trimmed, Qt::CaseInsensitive) == 0)
        {
            found = true;
        }
        else
        {
            updated.append(existing);
        }
    }

    if (found)
    {
        updateItemTags(index, updated);
    }
}

void CatalogModel::updateItemsTitle(const QList<int> &indices, const QString &title)
{
    for (int idx : indices)
    {
        updateItemTitle(idx, title);
    }
}

void CatalogModel::updateItemsCaption(const QList<int> &indices, const QString &caption)
{
    for (int idx : indices)
    {
        updateItemCaption(idx, caption);
    }
}

void CatalogModel::updateItemsColorLabel(const QList<int> &indices, int colorLabel)
{
    for (int idx : indices)
    {
        updateItemColorLabel(idx, colorLabel);
    }
}

void CatalogModel::updateItemsRating(const QList<int> &indices, int rating)
{
    const int clamped = qBound(0, rating, 5);
    for (int idx : indices)
    {
        updateItemRating(idx, clamped);
    }
}

void CatalogModel::updateItemsFlagged(const QList<int> &indices, bool flagged)
{
    for (int idx : indices)
    {
        updateItemFlagged(idx, flagged);
    }
}

void CatalogModel::addTagToItems(const QList<int> &indices, const QString &tag)
{
    const QString trimmed = tag.trimmed();
    if (trimmed.isEmpty())
    {
        return;
    }

    for (int idx : indices)
    {
        addTagToItem(idx, trimmed);
    }
}

void CatalogModel::removeTagFromItems(const QList<int> &indices, const QString &tag)
{
    const QString trimmed = tag.trimmed();
    if (trimmed.isEmpty())
    {
        return;
    }

    for (int idx : indices)
    {
        removeTagFromItem(idx, trimmed);
    }
}

QStringList CatalogModel::availableTags() const
{
    QSet<QString> unique;
    for (const auto &item : m_items)
    {
        for (const auto &t : item.tags)
        {
            const QString trimmed = t.trimmed();
            if (!trimmed.isEmpty())
            {
                unique.insert(trimmed);
            }
        }
    }

    if (m_database && m_database->isOpen())
    {
        const QStringList dbTags = m_database->allTags();
        for (const auto &t : dbTags)
        {
            const QString trimmed = t.trimmed();
            if (!trimmed.isEmpty())
            {
                unique.insert(trimmed);
            }
        }
    }

    QStringList list = unique.values();
    list.sort(Qt::CaseInsensitive);
    return list;
}

void CatalogModel::loadFromDatabase(int collectionId)
{
    if (!m_database || !m_database->isOpen())
    {
        return;
    }

    const QList<CatalogItem> dbItems = (collectionId > 0)
                                           ? m_database->fetchItemsByCollection(collectionId)
                                           : m_database->fetchAllItems();

    beginResetModel();
    m_items = dbItems;
    endResetModel();

    emit countChanged();
    emit availableTagsChanged();
    emit sessionLoaded(static_cast<int>(m_items.size()));
}

void CatalogModel::setCollectionAvailable(const QString &rootPath, bool available)
{
    if (m_database && m_database->isOpen())
    {
        m_database->setCollectionAvailable(rootPath, available);
    }

    for (int i = 0; i < m_items.size(); ++i)
    {
        if (m_items[i].filePath.startsWith(rootPath))
        {
            if (m_items[i].isMissing == available)
            {
                m_items[i].isMissing = !available;
                const QModelIndex idx = index(i, 0);
                emit dataChanged(idx, idx, {IsMissingRole, ItemMapRole});
            }
        }
    }
}

void CatalogModel::clearSession()
{
    if (m_items.isEmpty())
    {
        return;
    }

    beginResetModel();
    m_items.clear();
    endResetModel();

    emit countChanged();
    emit availableTagsChanged();
    emit sessionLoaded(0);
}

} // namespace PhotoCarta
