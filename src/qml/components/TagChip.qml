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
    signal removeRequested()

    implicitWidth: contentRow.implicitWidth + (removable ? 20 : 16)
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
        spacing: 6

        Text {
            id: chipLabel
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            font.pixelSize: themeManager.fontXs
            font.weight: root.active ? Font.DemiBold : Font.Normal
            color: root.active ? themeManager.accentText : themeManager.textPrimary
        }

        // Removable 'x' button
        Rectangle {
            id: removeBtn
            visible: root.removable
            width: 14
            height: 14
            radius: 7
            anchors.verticalCenter: parent.verticalCenter
            color: removeMouseArea.containsMouse
                   ? (root.active ? "#33ffffff" : themeManager.danger)
                   : "transparent"

            Text {
                anchors.centerIn: parent
                text: "✕"
                font.pixelSize: 8
                font.weight: Font.Bold
                color: root.active
                       ? themeManager.accentText
                       : (removeMouseArea.containsMouse ? "#ffffff" : themeManager.textSecondary)
            }

            MouseArea {
                id: removeMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: (mouse) => {
                    mouse.accepted = true
                    root.removeRequested()
                }
            }
        }
    }

    MouseArea {
        id: chipMouseArea
        anchors.fill: parent
        enabled: root.interactive
        hoverEnabled: root.interactive
        cursorShape: root.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: (mouse) => {
            if (root.removable && removeMouseArea.containsMouse) {
                return
            }
            root.clicked()
        }
    }
}
