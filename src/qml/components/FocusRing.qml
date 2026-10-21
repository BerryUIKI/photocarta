// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

// Visible keyboard focus indicator for custom-drawn controls.
// Place as the last child of the interactive item: it draws only while that
// item holds active keyboard focus, so mouse users never see it.
Rectangle {
    id: root

    property int targetRadius: themeManager.radiusMd

    anchors.fill: parent
    radius: root.targetRadius
    color: "transparent"
    border.color: themeManager.borderFocus
    border.width: 2
    visible: parent ? parent.activeFocus : false
    z: 100

    Accessible.ignored: true
}
