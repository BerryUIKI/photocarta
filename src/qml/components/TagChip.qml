// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string text: ""
    property bool active: false
    property bool interactive: true
    signal clicked()

    implicitWidth: chipLabel.implicitWidth + 18
    implicitHeight: 24
    radius: 12

    color: active
           ? themeManager.accent
           : (chipMouseArea.containsMouse && interactive)
             ? themeManager.surfaceHover
             : themeManager.surfaceElevated

    border.color: active
                  ? themeManager.accent
                  : themeManager.border
    border.width: 1

    Behavior on color { ColorAnimation { duration: 120 } }
    Behavior on border.color { ColorAnimation { duration: 120 } }

    Text {
        id: chipLabel
        anchors.centerIn: parent
        text: root.text
        font.pixelSize: themeManager.fontXs
        font.weight: root.active ? Font.DemiBold : Font.Normal
        color: root.active ? themeManager.accentText : themeManager.textPrimary
    }

    MouseArea {
        id: chipMouseArea
        anchors.fill: parent
        enabled: root.interactive
        hoverEnabled: root.interactive
        cursorShape: root.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: root.clicked()
    }
}
