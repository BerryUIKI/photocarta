// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSortFilterProxyModel>
#include <QString>
#include <QVariantMap>

namespace PhotoCarta
{

class CatalogFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY categoryFilterChanged)
    Q_PROPERTY(int collectionFilter READ collectionFilter WRITE setCollectionFilter NOTIFY collectionFilterChanged)
    Q_PROPERTY(QString tagFilter READ tagFilter WRITE setTagFilter NOTIFY tagFilterChanged)
    Q_PROPERTY(int minRating READ minRating WRITE setMinRating NOTIFY minRatingChanged)
    Q_PROPERTY(QString sortCriterion READ sortCriterion WRITE setSortCriterion NOTIFY sortCriterionChanged)
    Q_PROPERTY(bool sortAscending READ sortAscending WRITE setSortAscending NOTIFY sortAscendingChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit CatalogFilterModel(QObject *parent = nullptr);
    ~CatalogFilterModel() override = default;

    [[nodiscard]] QString searchQuery() const;
    void setSearchQuery(const QString &query);

    [[nodiscard]] QString categoryFilter() const;
    void setCategoryFilter(const QString &category);

    [[nodiscard]] int collectionFilter() const;
    void setCollectionFilter(int collectionId);

    [[nodiscard]] QString tagFilter() const;
    void setTagFilter(const QString &tag);

    [[nodiscard]] int minRating() const;
    void setMinRating(int rating);

    [[nodiscard]] QString sortCriterion() const;
    void setSortCriterion(const QString &criterion);

    [[nodiscard]] bool sortAscending() const;
    void setSortAscending(bool ascending);

    [[nodiscard]] int count() const;

    Q_INVOKABLE [[nodiscard]] int mapToSourceRow(int proxyRow) const;
    Q_INVOKABLE [[nodiscard]] int mapFromSourceRow(int sourceRow) const;
    Q_INVOKABLE [[nodiscard]] QVariantMap get(int proxyRow) const;

signals:
    void searchQueryChanged();
    void categoryFilterChanged();
    void collectionFilterChanged();
    void tagFilterChanged();
    void minRatingChanged();
    void sortCriterionChanged();
    void sortAscendingChanged();
    void countChanged();

protected:
    [[nodiscard]] bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    [[nodiscard]] bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;

private:
    void updateSorting();

    QString m_searchQuery;
    QString m_categoryFilter = QStringLiteral("all"); // "all", "demo", "imported", "favorites"
    int m_collectionFilter = 0; // 0 = all collections
    QString m_tagFilter;
    int m_minRating = 0;
    QString m_sortCriterion = QStringLiteral("date"); // "date", "name", "size", "rating"
    bool m_sortAscending = false;
};

} // namespace PhotoCarta
