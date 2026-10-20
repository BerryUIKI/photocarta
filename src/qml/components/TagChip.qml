// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string text: ""
    property bool active: false
    property bool interactive: true
    property bool removable: false
    signal clicked()
    signal removeClicked()

    implicitWidth: contentRow.implicitWidth + 18
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

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 4

        Text {
            id: chipLabel
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            font.pixelSize: themeManager.fontXs
            font.weight: root.active ? Font.DemiBold : Font.Normal
            color: root.active ? themeManager.accentText : themeManager.textPrimary
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.removable
            text: "×"
            font.pixelSize: themeManager.fontSm
            color: removeMouseArea.containsMouse ? themeManager.textPrimary : themeManager.textMuted

            MouseArea {
                id: removeMouseArea
                anchors.fill: parent
                anchors.margins: -4
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.removeClicked()
            }
        }
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
