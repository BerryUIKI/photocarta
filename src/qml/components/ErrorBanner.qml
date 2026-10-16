// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string message: sessionController.errorMessage
    property bool visibleState: sessionController.hasError

    visible: visibleState
    height: visible ? 42 : 0
    clip: true

    color: themeManager.isDark ? "#450a0a" : "#fef2f2"
    border.color: themeManager.isDark ? "#991b1b" : "#fca5a5"
    border.width: 1

    Behavior on height { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10

        Image {
            width: 18
            height: 18
            anchors.verticalCenter: parent.verticalCenter
            source: "qrc:/assets/icons/info.svg"
            fillMode: Image.PreserveAspectFit
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.message
            font.pixelSize: themeManager.fontSm
            font.weight: Font.Medium
            color: themeManager.isDark ? "#fca5a5" : "#991b1b"
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        width: 26
        height: 26
        radius: 13
        color: dismissMouse.containsMouse
               ? (themeManager.isDark ? "#7f1d1d" : "#fee2e2")
               : "transparent"

        Image {
            anchors.centerIn: parent
            width: 14
            height: 14
            source: "qrc:/assets/icons/clear.svg"
            fillMode: Image.PreserveAspectFit
        }

        MouseArea {
            id: dismissMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: sessionController.dismissError()
        }
    }
}
