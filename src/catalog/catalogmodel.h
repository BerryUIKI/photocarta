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
        ThumbnailUrlRole,
        PreviewUrlRole,
        IsDemoFixtureRole,
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

    Q_INVOKABLE [[nodiscard]] QVariantMap get(int index) const;
    Q_INVOKABLE void loadDemoFixtures();
    Q_INVOKABLE void openUrls(const QList<QUrl> &urls);
    Q_INVOKABLE void addFilePaths(const QStringList &filePaths);
    Q_INVOKABLE void clearSession();

signals:
    void countChanged();
    void sessionLoaded(int itemCount);
    void fileLoadError(const QString &fileName, const QString &errorMessage);

private:
    void appendItem(const CatalogItem &item);
    [[nodiscard]] static CatalogItem inspectLocalFile(const QString &filePath);

    QList<CatalogItem> m_items;
};

} // namespace PhotoCarta
