// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "catalogfiltermodel.h"

#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVariantMap>

namespace PhotoCarta
{

class SelectionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY selectionChanged)
    Q_PROPERTY(QString currentId READ currentId NOTIFY selectionChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap selectedItem READ selectedItem NOTIFY selectionChanged)

public:
    explicit SelectionManager(CatalogFilterModel *filterModel, QObject *parent = nullptr);
    ~SelectionManager() override = default;

    [[nodiscard]] int currentIndex() const;
    void setCurrentIndex(int index);

    [[nodiscard]] QString currentId() const;
    [[nodiscard]] const QSet<QString> &selectedIds() const;

    [[nodiscard]] bool hasSelection() const;
    [[nodiscard]] int selectedCount() const;
    [[nodiscard]] QVariantMap selectedItem() const;

    Q_INVOKABLE [[nodiscard]] bool isSelected(int index) const;
    Q_INVOKABLE [[nodiscard]] bool isSelectedId(const QString &id) const;

    Q_INVOKABLE void select(int index);
    Q_INVOKABLE void selectById(const QString &id);
    Q_INVOKABLE void toggleSelect(int index);
    Q_INVOKABLE void toggleSelectById(const QString &id);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void selectNext();
    Q_INVOKABLE void selectPrevious();
    Q_INVOKABLE void selectFirst();
    Q_INVOKABLE void selectLast();

signals:
    void selectionChanged();
    void currentItemActivated(int index);

private slots:
    void onModelResetOrChanged();

private:
    [[nodiscard]] int findRowForId(const QString &id) const;
    [[nodiscard]] QString idForRow(int row) const;
    void updateSelectedItem();

    CatalogFilterModel *m_filterModel = nullptr;
    QString m_currentId;
    QSet<QString> m_selectedIds;
    QVariantMap m_selectedItem;
};

} // namespace PhotoCarta
