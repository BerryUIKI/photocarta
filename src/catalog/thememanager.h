// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QObject>

namespace PhotoCarta
{

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isDark READ isDark WRITE setIsDark NOTIFY themeChanged)

    // Colors
    Q_PROPERTY(QColor background READ background NOTIFY themeChanged)
    Q_PROPERTY(QColor surface READ surface NOTIFY themeChanged)
    Q_PROPERTY(QColor surfaceElevated READ surfaceElevated NOTIFY themeChanged)
    Q_PROPERTY(QColor surfaceHover READ surfaceHover NOTIFY themeChanged)
    Q_PROPERTY(QColor surfaceSelected READ surfaceSelected NOTIFY themeChanged)
    Q_PROPERTY(QColor border READ border NOTIFY themeChanged)
    Q_PROPERTY(QColor borderFocus READ borderFocus NOTIFY themeChanged)
    Q_PROPERTY(QColor textPrimary READ textPrimary NOTIFY themeChanged)
    Q_PROPERTY(QColor textSecondary READ textSecondary NOTIFY themeChanged)
    Q_PROPERTY(QColor textMuted READ textMuted NOTIFY themeChanged)
    Q_PROPERTY(QColor accent READ accent NOTIFY themeChanged)
    Q_PROPERTY(QColor accentHover READ accentHover NOTIFY themeChanged)
    Q_PROPERTY(QColor accentText READ accentText NOTIFY themeChanged)
    Q_PROPERTY(QColor danger READ danger NOTIFY themeChanged)
    Q_PROPERTY(QColor warning READ warning NOTIFY themeChanged)
    Q_PROPERTY(QColor success READ success NOTIFY themeChanged)
    Q_PROPERTY(QColor badgeDemoBg READ badgeDemoBg NOTIFY themeChanged)
    Q_PROPERTY(QColor badgeDemoText READ badgeDemoText NOTIFY themeChanged)

    // Spacing design tokens
    Q_PROPERTY(int spacingXs READ spacingXs CONSTANT)
    Q_PROPERTY(int spacingSm READ spacingSm CONSTANT)
    Q_PROPERTY(int spacingMd READ spacingMd CONSTANT)
    Q_PROPERTY(int spacingLg READ spacingLg CONSTANT)
    Q_PROPERTY(int spacingXl READ spacingXl CONSTANT)

    // Corner radius design tokens
    Q_PROPERTY(int radiusSm READ radiusSm CONSTANT)
    Q_PROPERTY(int radiusMd READ radiusMd CONSTANT)
    Q_PROPERTY(int radiusLg READ radiusLg CONSTANT)
    Q_PROPERTY(int radiusPill READ radiusPill CONSTANT)

    // Font size design tokens
    Q_PROPERTY(int fontXs READ fontXs CONSTANT)
    Q_PROPERTY(int fontSm READ fontSm CONSTANT)
    Q_PROPERTY(int fontBase READ fontBase CONSTANT)
    Q_PROPERTY(int fontLg READ fontLg CONSTANT)
    Q_PROPERTY(int fontXl READ fontXl CONSTANT)

public:
    explicit ThemeManager(bool dark = true, QObject *parent = nullptr);
    ~ThemeManager() override = default;

    [[nodiscard]] bool isDark() const;
    void setIsDark(bool dark);
    Q_INVOKABLE void toggleTheme();

    [[nodiscard]] QColor background() const;
    [[nodiscard]] QColor surface() const;
    [[nodiscard]] QColor surfaceElevated() const;
    [[nodiscard]] QColor surfaceHover() const;
    [[nodiscard]] QColor surfaceSelected() const;
    [[nodiscard]] QColor border() const;
    [[nodiscard]] QColor borderFocus() const;
    [[nodiscard]] QColor textPrimary() const;
    [[nodiscard]] QColor textSecondary() const;
    [[nodiscard]] QColor textMuted() const;
    [[nodiscard]] QColor accent() const;
    [[nodiscard]] QColor accentHover() const;
    [[nodiscard]] QColor accentText() const;
    [[nodiscard]] QColor danger() const;
    [[nodiscard]] QColor warning() const;
    [[nodiscard]] QColor success() const;
    [[nodiscard]] QColor badgeDemoBg() const;
    [[nodiscard]] QColor badgeDemoText() const;

    [[nodiscard]] static constexpr int spacingXs() { return 4; }
    [[nodiscard]] static constexpr int spacingSm() { return 8; }
    [[nodiscard]] static constexpr int spacingMd() { return 12; }
    [[nodiscard]] static constexpr int spacingLg() { return 16; }
    [[nodiscard]] static constexpr int spacingXl() { return 24; }

    [[nodiscard]] static constexpr int radiusSm() { return 4; }
    [[nodiscard]] static constexpr int radiusMd() { return 6; }
    [[nodiscard]] static constexpr int radiusLg() { return 8; }
    [[nodiscard]] static constexpr int radiusPill() { return 999; }

    [[nodiscard]] static constexpr int fontXs() { return 11; }
    [[nodiscard]] static constexpr int fontSm() { return 12; }
    [[nodiscard]] static constexpr int fontBase() { return 13; }
    [[nodiscard]] static constexpr int fontLg() { return 15; }
    [[nodiscard]] static constexpr int fontXl() { return 18; }

signals:
    void themeChanged();

private:
    bool m_isDark = true;
};

} // namespace PhotoCarta
