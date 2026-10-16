// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    implicitHeight: 28
    color: themeManager.surface
    border.color: themeManager.border
    border.width: 1

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        // Status text / notification
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: sessionController.statusMessage
            font.pixelSize: 11
            color: themeManager.textSecondary
            elide: Text.ElideRight
            maximumLineCount: 1
        }
    }

    // Keyboard Shortcuts Hint (Center)
    Text {
        anchors.centerIn: parent
        text: qsTr("Enter: Preview  •  Esc: Grid  •  Space: Select  •  Arrows: Navigate  •  Ctrl+O: Open")
        font.pixelSize: 10
        color: themeManager.textMuted
        visible: parent.width > 780
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        // Zoom Slider for Thumbnail Grid
        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6
            visible: !sessionController.previewMode

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "−"
                font.pixelSize: 11
                color: themeManager.textSecondary
            }

            Slider {
                id: zoomSlider
                anchors.verticalCenter: parent.verticalCenter
                width: 70
                from: 0.7
                to: 1.8
                value: sessionController.zoomFactor
                onMoved: sessionController.zoomFactor = value
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "+"
                font.pixelSize: 11
                color: themeManager.textSecondary
            }
        }

        Rectangle {
            width: 1
            height: 14
            anchors.verticalCenter: parent.verticalCenter
            color: themeManager.border
        }

        // Selected Counter
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: selectionManager.hasSelection
                  ? qsTr("%1 of %2 shown (%3 selected)")
                    .arg(filterModel.count)
                    .arg(catalogModel.count)
                    .arg(selectionManager.selectedCount)
                  : qsTr("%1 of %2 shown")
                    .arg(filterModel.count)
                    .arg(catalogModel.count)
            font.pixelSize: 11
            color: themeManager.textSecondary
        }

        // Session mode badge
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: sessionModeText.implicitWidth + 8
            height: 16
            radius: 3
            color: themeManager.surfaceElevated

            Text {
                id: sessionModeText
                anchors.centerIn: parent
                text: qsTr("EPHEMERAL")
                font.pixelSize: 9
                font.weight: Font.Bold
                color: themeManager.textMuted
            }
        }
    }
}
