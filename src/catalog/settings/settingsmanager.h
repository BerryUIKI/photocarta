// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <memory>

namespace PhotoCarta
{

/**
 * @brief Manages application-wide user preferences and session persistence.
 *
 * Persists UI preferences across application restarts using QSettings:
 * - Theme selection (Dark / Light)
 * - Navigation and details sidebar visibility
 * - Grid thumbnail zoom factor
 * - Catalog sort criteria and sort direction
 * - Last opened catalog database path and collection ID
 * - Main window geometry and maximized state
 * - Image viewer filmstrip visibility
 */
class SettingsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool leftPaneVisible READ leftPaneVisible WRITE setLeftPaneVisible NOTIFY leftPaneVisibleChanged)
    Q_PROPERTY(bool rightPaneVisible READ rightPaneVisible WRITE setRightPaneVisible NOTIFY rightPaneVisibleChanged)
    Q_PROPERTY(double gridZoomFactor READ gridZoomFactor WRITE setGridZoomFactor NOTIFY gridZoomFactorChanged)
    Q_PROPERTY(QString sortCriterion READ sortCriterion WRITE setSortCriterion NOTIFY sortCriterionChanged)
    Q_PROPERTY(bool sortAscending READ sortAscending WRITE setSortAscending NOTIFY sortAscendingChanged)
    Q_PROPERTY(QString lastDatabasePath READ lastDatabasePath WRITE setLastDatabasePath NOTIFY lastDatabasePathChanged)
    Q_PROPERTY(int lastCollectionId READ lastCollectionId WRITE setLastCollectionId NOTIFY lastCollectionIdChanged)
    Q_PROPERTY(int windowWidth READ windowWidth WRITE setWindowWidth NOTIFY windowGeometryChanged)
    Q_PROPERTY(int windowHeight READ windowHeight WRITE setWindowHeight NOTIFY windowGeometryChanged)
    Q_PROPERTY(int windowX READ windowX WRITE setWindowX NOTIFY windowGeometryChanged)
    Q_PROPERTY(int windowY READ windowY WRITE setWindowY NOTIFY windowGeometryChanged)
    Q_PROPERTY(bool windowMaximized READ windowMaximized WRITE setWindowMaximized NOTIFY windowGeometryChanged)
    Q_PROPERTY(bool filmstripVisible READ filmstripVisible WRITE setFilmstripVisible NOTIFY filmstripVisibleChanged)

public:
    explicit SettingsManager(QObject *parent = nullptr);
    explicit SettingsManager(const QString &customIniPath, QObject *parent = nullptr);
    ~SettingsManager() override = default;

    /// Loads settings from storage.
    void load();

    /// Synchronizes changes immediately to storage.
    void sync();

    // Theme
    [[nodiscard]] QString theme() const;
    void setTheme(const QString &theme);

    // Panes
    [[nodiscard]] bool leftPaneVisible() const;
    void setLeftPaneVisible(bool visible);

    [[nodiscard]] bool rightPaneVisible() const;
    void setRightPaneVisible(bool visible);

    // Grid Zoom
    [[nodiscard]] double gridZoomFactor() const;
    void setGridZoomFactor(double factor);

    // Sorting
    [[nodiscard]] QString sortCriterion() const;
    void setSortCriterion(const QString &criterion);

    [[nodiscard]] bool sortAscending() const;
    void setSortAscending(bool ascending);

    // Catalog & Collection
    [[nodiscard]] QString lastDatabasePath() const;
    void setLastDatabasePath(const QString &path);

    [[nodiscard]] int lastCollectionId() const;
    void setLastCollectionId(int id);

    // Window Geometry
    [[nodiscard]] int windowWidth() const;
    void setWindowWidth(int width);

    [[nodiscard]] int windowHeight() const;
    void setWindowHeight(int height);

    [[nodiscard]] int windowX() const;
    void setWindowX(int x);

    [[nodiscard]] int windowY() const;
    void setWindowY(int y);

    [[nodiscard]] bool windowMaximized() const;
    void setWindowMaximized(bool maximized);

    // Viewer
    [[nodiscard]] bool filmstripVisible() const;
    void setFilmstripVisible(bool visible);

    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);
    Q_INVOKABLE void resetToDefaults();

signals:
    void themeChanged(const QString &theme);
    void leftPaneVisibleChanged(bool visible);
    void rightPaneVisibleChanged(bool visible);
    void gridZoomFactorChanged(double factor);
    void sortCriterionChanged(const QString &criterion);
    void sortAscendingChanged(bool ascending);
    void lastDatabasePathChanged(const QString &path);
    void lastCollectionIdChanged(int id);
    void windowGeometryChanged();
    void filmstripVisibleChanged(bool visible);

private:
    std::unique_ptr<QSettings> m_settings;

    QString m_theme = QStringLiteral("dark");
    bool m_leftPaneVisible = true;
    bool m_rightPaneVisible = true;
    double m_gridZoomFactor = 1.0;
    QString m_sortCriterion = QStringLiteral("date");
    bool m_sortAscending = false;
    QString m_lastDatabasePath;
    int m_lastCollectionId = 0;
    int m_windowWidth = 1200;
    int m_windowHeight = 800;
    int m_windowX = -1;
    int m_windowY = -1;
    bool m_windowMaximized = false;
    bool m_filmstripVisible = true;
};

} // namespace PhotoCarta
