// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

pragma Singleton

QtObject {
    id: theme

    // Direct proxy to C++ ThemeManager singleton/context property
    readonly property color background: themeManager.background
    readonly property color surface: themeManager.surface
    readonly property color surfaceElevated: themeManager.surfaceElevated
    readonly property color surfaceHover: themeManager.surfaceHover
    readonly property color surfaceSelected: themeManager.surfaceSelected
    readonly property color border: themeManager.border
    readonly property color borderFocus: themeManager.borderFocus
    readonly property color textPrimary: themeManager.textPrimary
    readonly property color textSecondary: themeManager.textSecondary
    readonly property color textMuted: themeManager.textMuted
    readonly property color accent: themeManager.accent
    readonly property color accentHover: themeManager.accentHover
    readonly property color accentText: themeManager.accentText
    readonly property color danger: themeManager.danger
    readonly property color warning: themeManager.warning
    readonly property color success: themeManager.success
    readonly property color badgeDemoBg: themeManager.badgeDemoBg
    readonly property color badgeDemoText: themeManager.badgeDemoText

    readonly property int spacingXs: themeManager.spacingXs
    readonly property int spacingSm: themeManager.spacingSm
    readonly property int spacingMd: themeManager.spacingMd
    readonly property int spacingLg: themeManager.spacingLg
    readonly property int spacingXl: themeManager.spacingXl

    readonly property int radiusSm: themeManager.radiusSm
    readonly property int radiusMd: themeManager.radiusMd
    readonly property int radiusLg: themeManager.radiusLg
    readonly property int radiusPill: themeManager.radiusPill

    readonly property int fontXs: themeManager.fontXs
    readonly property int fontSm: themeManager.fontSm
    readonly property int fontBase: themeManager.fontBase
    readonly property int fontLg: themeManager.fontLg
    readonly property int fontXl: themeManager.fontXl
}
