// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thememanager.h"

namespace PhotoCarta
{

ThemeManager::ThemeManager(bool dark, QObject *parent)
    : QObject(parent)
    , m_isDark(dark)
{
}

bool ThemeManager::isDark() const
{
    return m_isDark;
}

void ThemeManager::setIsDark(bool dark)
{
    if (m_isDark == dark)
    {
        return;
    }
    m_isDark = dark;
    emit themeChanged();
}

void ThemeManager::toggleTheme()
{
    setIsDark(!m_isDark);
}

QColor ThemeManager::background() const
{
    return m_isDark ? QColor(QStringLiteral("#121316")) : QColor(QStringLiteral("#f1f5f9"));
}

QColor ThemeManager::surface() const
{
    return m_isDark ? QColor(QStringLiteral("#1a1b1f")) : QColor(QStringLiteral("#ffffff"));
}

QColor ThemeManager::surfaceElevated() const
{
    return m_isDark ? QColor(QStringLiteral("#22242a")) : QColor(QStringLiteral("#f8fafc"));
}

QColor ThemeManager::surfaceHover() const
{
    return m_isDark ? QColor(QStringLiteral("#2c2e36")) : QColor(QStringLiteral("#e2e8f0"));
}

QColor ThemeManager::surfaceSelected() const
{
    return m_isDark ? QColor(QStringLiteral("#1e3a5f")) : QColor(QStringLiteral("#dbeafe"));
}

QColor ThemeManager::border() const
{
    return m_isDark ? QColor(QStringLiteral("#2d3039")) : QColor(QStringLiteral("#cbd5e1"));
}

QColor ThemeManager::borderFocus() const
{
    return m_isDark ? QColor(QStringLiteral("#3b82f6")) : QColor(QStringLiteral("#2563eb"));
}

QColor ThemeManager::textPrimary() const
{
    return m_isDark ? QColor(QStringLiteral("#f1f5f9")) : QColor(QStringLiteral("#0f172a"));
}

QColor ThemeManager::textSecondary() const
{
    return m_isDark ? QColor(QStringLiteral("#94a3b8")) : QColor(QStringLiteral("#475569"));
}

QColor ThemeManager::textMuted() const
{
    return m_isDark ? QColor(QStringLiteral("#64748b")) : QColor(QStringLiteral("#94a3b8"));
}

QColor ThemeManager::accent() const
{
    return m_isDark ? QColor(QStringLiteral("#3b82f6")) : QColor(QStringLiteral("#2563eb"));
}

QColor ThemeManager::accentHover() const
{
    return m_isDark ? QColor(QStringLiteral("#60a5fa")) : QColor(QStringLiteral("#1d4ed8"));
}

QColor ThemeManager::accentText() const
{
    return QColor(QStringLiteral("#ffffff"));
}

QColor ThemeManager::danger() const
{
    return m_isDark ? QColor(QStringLiteral("#ef4444")) : QColor(QStringLiteral("#dc2626"));
}

QColor ThemeManager::warning() const
{
    return m_isDark ? QColor(QStringLiteral("#f59e0b")) : QColor(QStringLiteral("#d97706"));
}

QColor ThemeManager::success() const
{
    return m_isDark ? QColor(QStringLiteral("#10b981")) : QColor(QStringLiteral("#059669"));
}

QColor ThemeManager::badgeDemoBg() const
{
    return m_isDark ? QColor(QStringLiteral("#3b1d54")) : QColor(QStringLiteral("#ede9fe"));
}

QColor ThemeManager::badgeDemoText() const
{
    return m_isDark ? QColor(QStringLiteral("#d8b4fe")) : QColor(QStringLiteral("#6d28d9"));
}

} // namespace PhotoCarta
