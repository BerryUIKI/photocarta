// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingsmanager.h"

namespace PhotoCarta
{

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
    , m_settings(std::make_unique<QSettings>(QStringLiteral("PhotoCarta"), QStringLiteral("PhotoCarta")))
{
    load();
}

SettingsManager::SettingsManager(const QString &customIniPath, QObject *parent)
    : QObject(parent)
    , m_settings(std::make_unique<QSettings>(customIniPath, QSettings::IniFormat))
{
    load();
}

void SettingsManager::load()
{
    if (!m_settings)
    {
        return;
    }

    m_theme = m_settings->value(QStringLiteral("appearance/theme"), QStringLiteral("dark")).toString();
    m_leftPaneVisible = m_settings->value(QStringLiteral("ui/leftPaneVisible"), true).toBool();
    m_rightPaneVisible = m_settings->value(QStringLiteral("ui/rightPaneVisible"), true).toBool();
    m_gridZoomFactor = m_settings->value(QStringLiteral("ui/gridZoomFactor"), 1.0).toDouble();
    m_sortCriterion = m_settings->value(QStringLiteral("catalog/sortCriterion"), QStringLiteral("date")).toString();
    m_sortAscending = m_settings->value(QStringLiteral("catalog/sortAscending"), false).toBool();
    m_lastDatabasePath = m_settings->value(QStringLiteral("session/lastDatabasePath"), QString()).toString();
    m_lastCollectionId = m_settings->value(QStringLiteral("session/lastCollectionId"), 0).toInt();
    m_windowWidth = m_settings->value(QStringLiteral("window/width"), 1200).toInt();
    m_windowHeight = m_settings->value(QStringLiteral("window/height"), 800).toInt();
    m_windowX = m_settings->value(QStringLiteral("window/x"), -1).toInt();
    m_windowY = m_settings->value(QStringLiteral("window/y"), -1).toInt();
    m_windowMaximized = m_settings->value(QStringLiteral("window/maximized"), false).toBool();
    m_filmstripVisible = m_settings->value(QStringLiteral("viewer/filmstripVisible"), true).toBool();
}

void SettingsManager::sync()
{
    if (m_settings)
    {
        m_settings->sync();
    }
}

QString SettingsManager::theme() const
{
    return m_theme;
}

void SettingsManager::setTheme(const QString &theme)
{
    if (m_theme != theme)
    {
        m_theme = theme;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("appearance/theme"), m_theme);
        }
        emit themeChanged(m_theme);
    }
}

bool SettingsManager::leftPaneVisible() const
{
    return m_leftPaneVisible;
}

void SettingsManager::setLeftPaneVisible(bool visible)
{
    if (m_leftPaneVisible != visible)
    {
        m_leftPaneVisible = visible;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("ui/leftPaneVisible"), m_leftPaneVisible);
        }
        emit leftPaneVisibleChanged(m_leftPaneVisible);
    }
}

bool SettingsManager::rightPaneVisible() const
{
    return m_rightPaneVisible;
}

void SettingsManager::setRightPaneVisible(bool visible)
{
    if (m_rightPaneVisible != visible)
    {
        m_rightPaneVisible = visible;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("ui/rightPaneVisible"), m_rightPaneVisible);
        }
        emit rightPaneVisibleChanged(m_rightPaneVisible);
    }
}

double SettingsManager::gridZoomFactor() const
{
    return m_gridZoomFactor;
}

void SettingsManager::setGridZoomFactor(double factor)
{
    const double clamped = qBound(0.5, factor, 2.5);
    if (!qFuzzyCompare(m_gridZoomFactor, clamped))
    {
        m_gridZoomFactor = clamped;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("ui/gridZoomFactor"), m_gridZoomFactor);
        }
        emit gridZoomFactorChanged(m_gridZoomFactor);
    }
}

QString SettingsManager::sortCriterion() const
{
    return m_sortCriterion;
}

void SettingsManager::setSortCriterion(const QString &criterion)
{
    if (m_sortCriterion != criterion)
    {
        m_sortCriterion = criterion;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("catalog/sortCriterion"), m_sortCriterion);
        }
        emit sortCriterionChanged(m_sortCriterion);
    }
}

bool SettingsManager::sortAscending() const
{
    return m_sortAscending;
}

void SettingsManager::setSortAscending(bool ascending)
{
    if (m_sortAscending != ascending)
    {
        m_sortAscending = ascending;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("catalog/sortAscending"), m_sortAscending);
        }
        emit sortAscendingChanged(m_sortAscending);
    }
}

QString SettingsManager::lastDatabasePath() const
{
    return m_lastDatabasePath;
}

void SettingsManager::setLastDatabasePath(const QString &path)
{
    if (m_lastDatabasePath != path)
    {
        m_lastDatabasePath = path;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("session/lastDatabasePath"), m_lastDatabasePath);
        }
        emit lastDatabasePathChanged(m_lastDatabasePath);
    }
}

int SettingsManager::lastCollectionId() const
{
    return m_lastCollectionId;
}

void SettingsManager::setLastCollectionId(int id)
{
    if (m_lastCollectionId != id)
    {
        m_lastCollectionId = id;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("session/lastCollectionId"), m_lastCollectionId);
        }
        emit lastCollectionIdChanged(m_lastCollectionId);
    }
}

int SettingsManager::windowWidth() const
{
    return m_windowWidth;
}

void SettingsManager::setWindowWidth(int width)
{
    if (m_windowWidth != width && width > 400)
    {
        m_windowWidth = width;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("window/width"), m_windowWidth);
        }
        emit windowGeometryChanged();
    }
}

int SettingsManager::windowHeight() const
{
    return m_windowHeight;
}

void SettingsManager::setWindowHeight(int height)
{
    if (m_windowHeight != height && height > 300)
    {
        m_windowHeight = height;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("window/height"), m_windowHeight);
        }
        emit windowGeometryChanged();
    }
}

int SettingsManager::windowX() const
{
    return m_windowX;
}

void SettingsManager::setWindowX(int x)
{
    if (m_windowX != x)
    {
        m_windowX = x;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("window/x"), m_windowX);
        }
        emit windowGeometryChanged();
    }
}

int SettingsManager::windowY() const
{
    return m_windowY;
}

void SettingsManager::setWindowY(int y)
{
    if (m_windowY != y)
    {
        m_windowY = y;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("window/y"), m_windowY);
        }
        emit windowGeometryChanged();
    }
}

bool SettingsManager::windowMaximized() const
{
    return m_windowMaximized;
}

void SettingsManager::setWindowMaximized(bool maximized)
{
    if (m_windowMaximized != maximized)
    {
        m_windowMaximized = maximized;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("window/maximized"), m_windowMaximized);
        }
        emit windowGeometryChanged();
    }
}

bool SettingsManager::filmstripVisible() const
{
    return m_filmstripVisible;
}

void SettingsManager::setFilmstripVisible(bool visible)
{
    if (m_filmstripVisible != visible)
    {
        m_filmstripVisible = visible;
        if (m_settings)
        {
            m_settings->setValue(QStringLiteral("viewer/filmstripVisible"), m_filmstripVisible);
        }
        emit filmstripVisibleChanged(m_filmstripVisible);
    }
}

void SettingsManager::saveWindowGeometry(int x, int y, int width, int height, bool maximized)
{
    m_windowX = x;
    m_windowY = y;
    if (width > 400)
    {
        m_windowWidth = width;
    }
    if (height > 300)
    {
        m_windowHeight = height;
    }
    m_windowMaximized = maximized;

    if (m_settings)
    {
        m_settings->setValue(QStringLiteral("window/x"), m_windowX);
        m_settings->setValue(QStringLiteral("window/y"), m_windowY);
        m_settings->setValue(QStringLiteral("window/width"), m_windowWidth);
        m_settings->setValue(QStringLiteral("window/height"), m_windowHeight);
        m_settings->setValue(QStringLiteral("window/maximized"), m_windowMaximized);
    }
    emit windowGeometryChanged();
}

void SettingsManager::resetToDefaults()
{
    if (m_settings)
    {
        m_settings->clear();
    }

    setTheme(QStringLiteral("dark"));
    setLeftPaneVisible(true);
    setRightPaneVisible(true);
    setGridZoomFactor(1.0);
    setSortCriterion(QStringLiteral("date"));
    setSortAscending(false);
    setLastDatabasePath(QString());
    setLastCollectionId(0);
    setFilmstripVisible(true);
    saveWindowGeometry(-1, -1, 1200, 800, false);
}

} // namespace PhotoCarta
