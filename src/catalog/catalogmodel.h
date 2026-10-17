// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "catalogitem.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QStringList>
#include <QUrl>
#include <QVector>

namespace PhotoCarta
{

class CatalogModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool hasDemoContent READ hasDemoContent NOTIFY countChanged)
    Q_PROPERTY(QStringList availableTags READ availableTags NOTIFY availableTagsChanged)

public:
    enum CatalogRoles
    {
        IdRole = Qt::UserRole + 1,
        FilePathRole,
        FileNameRole,
        TitleRole,
        FileSizeRole,
        FormattedFileSizeRole,
        MimeTypeRole,
        DateTakenRole,
        FormattedDateRole,
        FileLastModifiedRole,
        FormattedLastModifiedRole,
        WidthRole,
        HeightRole,
        DimensionsRole,
        CameraModelRole,
        LensModelRole,
        FocalLengthRole,
        ApertureRole,
        ShutterSpeedRole,
        IsoRole,
        RatingRole,
        IsFlaggedRole,
        TagsRole,
        CaptionRole,
        ColorLabelRole,
        ThumbnailUrlRole,
        PreviewUrlRole,
        IsDemoFixtureRole,
        IsMissingRole,
        CollectionIdRole,
        ItemMapRole
    };
    Q_ENUM(CatalogRoles)

    explicit CatalogModel(QObject *parent = nullptr);
    ~CatalogModel() override = default;

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int count() const;
    [[nodiscard]] bool hasDemoContent() const;
    [[nodiscard]] const QList<CatalogItem> &items() const;
    [[nodiscard]] const CatalogItem *itemAt(int index) const;
    [[nodiscard]] QStringList availableTags() const;

    void setDatabase(class CatalogDatabase *database);
    [[nodiscard]] class CatalogDatabase *database() const;

    Q_INVOKABLE [[nodiscard]] QVariantMap get(int index) const;
    Q_INVOKABLE void loadDemoFixtures();
    Q_INVOKABLE void openUrls(const QList<QUrl> &urls);
    Q_INVOKABLE void addFilePaths(const QStringList &filePaths);
    Q_INVOKABLE void addItems(const QList<CatalogItem> &items);
    Q_INVOKABLE void updateItemTitle(int index, const QString &title);
    Q_INVOKABLE void updateItemCaption(int index, const QString &caption);
    Q_INVOKABLE void updateItemColorLabel(int index, int colorLabel);
    Q_INVOKABLE void updateItemRating(int index, int rating);
    Q_INVOKABLE void updateItemFlagged(int index, bool flagged);
    Q_INVOKABLE void updateItemTags(int index, const QStringList &tags);
    Q_INVOKABLE void addTagToItem(int index, const QString &tag);
    Q_INVOKABLE void removeTagFromItem(int index, const QString &tag);
    Q_INVOKABLE void updateItemsTitle(const QList<int> &indices, const QString &title);
    Q_INVOKABLE void updateItemsCaption(const QList<int> &indices, const QString &caption);
    Q_INVOKABLE void updateItemsColorLabel(const QList<int> &indices, int colorLabel);
    Q_INVOKABLE void updateItemsRating(const QList<int> &indices, int rating);
    Q_INVOKABLE void updateItemsFlagged(const QList<int> &indices, bool flagged);
    Q_INVOKABLE void addTagToItems(const QList<int> &indices, const QString &tag);
    Q_INVOKABLE void removeTagFromItems(const QList<int> &indices, const QString &tag);
    Q_INVOKABLE void loadFromDatabase(int collectionId = 0);
    Q_INVOKABLE void setCollectionAvailable(const QString &rootPath, bool available);
    Q_INVOKABLE void clearSession();

signals:
    void countChanged();
    void availableTagsChanged();
    void sessionLoaded(int itemCount);
    void fileLoadError(const QString &fileName, const QString &errorMessage);

private:
    void appendItem(const CatalogItem &item);
    [[nodiscard]] static CatalogItem inspectLocalFile(const QString &filePath);

    QList<CatalogItem> m_items;
    class CatalogDatabase *m_database = nullptr;
};

} // namespace PhotoCarta
