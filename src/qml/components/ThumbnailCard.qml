// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property int itemIndex: index
    property bool isSelected: selectionManager.currentIndex === index || selectionManager.isSelected(index)

    signal activated()

    width: GridView.view.cellWidth - 12
    height: GridView.view.cellHeight - 12
    x: 6
    y: 6
    radius: themeManager.radiusMd
    color: isSelected
           ? themeManager.surfaceSelected
           : (cardMouseArea.containsMouse ? themeManager.surfaceHover : themeManager.surface)
    border.color: isSelected
                  ? themeManager.borderFocus
                  : (cardMouseArea.containsMouse ? themeManager.textMuted : themeManager.border)
    border.width: isSelected ? 2 : 1
    clip: true

    Behavior on color { ColorAnimation { duration: 120 } }
    Behavior on border.color { ColorAnimation { duration: 120 } }

    // Image Preview Area
    Item {
        anchors.fill: parent
        anchors.margins: 4

        // Placeholder background while loading
        Rectangle {
            anchors.fill: parent
            radius: themeManager.radiusMd - 2
            color: themeManager.surfaceElevated
        }

        Image {
            id: thumbImage
            anchors.fill: parent
            source: model.thumbnailUrl
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
            // Bounded image buffer to avoid huge memory usage on raw/high-res images
            sourceSize.width: 480
            sourceSize.height: 360

            Rectangle {
                anchors.fill: parent
                color: themeManager.isDark ? "#000000" : "#ffffff"
                opacity: thumbImage.status === Image.Loading ? 0.4 : 0.0
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }
        }

        // Selection Checkmark Badge
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 8
            width: 22
            height: 22
            radius: 11
            color: root.isSelected ? themeManager.accent : "#80000000"
            border.color: "#ffffff"
            border.width: 1.5
            visible: root.isSelected || cardMouseArea.containsMouse

            Text {
                anchors.centerIn: parent
                text: root.isSelected ? "✓" : ""
                font.pixelSize: 11
                font.weight: Font.Bold
                color: "#ffffff"
            }
        }

        // Demo Badge Overlay
        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 8
            width: demoText.implicitWidth + 8
            height: 18
            radius: 4
            color: themeManager.badgeDemoBg
            border.color: themeManager.badgeDemoText
            border.width: 1
            visible: model.isDemoFixture

            Text {
                id: demoText
                anchors.centerIn: parent
                text: qsTr("DEMO")
                font.pixelSize: 9
                font.weight: Font.Bold
                color: themeManager.badgeDemoText
            }
        }

        // Offline / Missing Collection Badge Overlay
        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 8
            width: offlineText.implicitWidth + 8
            height: 18
            radius: 4
            color: "#b91c1c"
            border.color: "#fca5a5"
            border.width: 1
            visible: model.isMissing === true && !model.isDemoFixture

            Text {
                id: offlineText
                anchors.centerIn: parent
                text: qsTr("OFFLINE")
                font.pixelSize: 9
                font.weight: Font.Bold
                color: "#ffffff"
            }
        }

        // Bottom Info Gradient Overlay
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 48
            radius: themeManager.radiusMd - 2

            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: "#d0090d16" }
            }

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 8
                spacing: 2

                Text {
                    width: parent.width
                    text: model.title ? model.title : model.fileName
                    font.pixelSize: themeManager.fontSm
                    font.weight: Font.Medium
                    color: "#ffffff"
                    elide: Text.ElideRight
                }

                Row {
                    width: parent.width
                    spacing: 6

                    Text {
                        text: model.formattedFileSize
                        font.pixelSize: 10
                        color: "#cbd5e1"
                    }

                    Text {
                        text: "•"
                        font.pixelSize: 10
                        color: "#94a3b8"
                    }

                    RatingStars {
                        rating: model.rating
                        starSize: 10
                        interactive: false
                    }
                }
            }
        }
    }

    MouseArea {
        id: cardMouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: (mouse) => {
            if (mouse.button === Qt.RightButton) {
                if (!root.isSelected) {
                    selectionManager.select(root.itemIndex)
                }
                cardContextMenu.popup()
                return
            }

            if (mouse.modifiers & Qt.ShiftModifier) {
                selectionManager.selectRange(root.itemIndex)
            } else if (mouse.modifiers & Qt.ControlModifier) {
                selectionManager.toggleSelect(root.itemIndex)
            } else {
                selectionManager.select(root.itemIndex)
            }
        }

        onDoubleClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton) {
                selectionManager.select(root.itemIndex)
                sessionController.setPreviewMode(true)
                root.activated()
            }
        }
    }

    Menu {
        id: cardContextMenu

        MenuItem {
            text: qsTr("Preview Photo (Enter)")
            onTriggered: {
                selectionManager.select(root.itemIndex)
                sessionController.setPreviewMode(true)
            }
        }

        MenuSeparator {}

        Menu {
            title: qsTr("Set Rating")

            MenuItem {
                text: "★ 5"
                onTriggered: {
                    if (selectionManager.selectedCount > 1 && root.isSelected) {
                        sessionController.setRatingForSelected(5)
                    } else {
                        sessionController.setItemRating(root.itemIndex, 5)
                    }
                }
            }
            MenuItem {
                text: "★ 4"
                onTriggered: {
                    if (selectionManager.selectedCount > 1 && root.isSelected) {
                        sessionController.setRatingForSelected(4)
                    } else {
                        sessionController.setItemRating(root.itemIndex, 4)
                    }
                }
            }
            MenuItem {
                text: "★ 3"
                onTriggered: {
                    if (selectionManager.selectedCount > 1 && root.isSelected) {
                        sessionController.setRatingForSelected(3)
                    } else {
                        sessionController.setItemRating(root.itemIndex, 3)
                    }
                }
            }
            MenuItem {
                text: "★ 2"
                onTriggered: {
                    if (selectionManager.selectedCount > 1 && root.isSelected) {
                        sessionController.setRatingForSelected(2)
                    } else {
                        sessionController.setItemRating(root.itemIndex, 2)
                    }
                }
            }
            MenuItem {
                text: "★ 1"
                onTriggered: {
                    if (selectionManager.selectedCount > 1 && root.isSelected) {
                        sessionController.setRatingForSelected(1)
                    } else {
                        sessionController.setItemRating(root.itemIndex, 1)
                    }
                }
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Clear Rating (0)")
                onTriggered: {
                    if (selectionManager.selectedCount > 1 && root.isSelected) {
                        sessionController.setRatingForSelected(0)
                    } else {
                        sessionController.setItemRating(root.itemIndex, 0)
                    }
                }
            }
        }

        MenuItem {
            text: (selectionManager.selectedCount > 1 && root.isSelected)
                  ? qsTr("Toggle Flag for Selected (F)")
                  : (model.isFlagged ? qsTr("Unflag Photo (F)") : qsTr("Flag as Favorite (F)"))
            onTriggered: {
                if (selectionManager.selectedCount > 1 && root.isSelected) {
                    sessionController.toggleFlaggedForSelected()
                } else {
                    sessionController.toggleItemFlagged(root.itemIndex)
                }
            }
        }

        MenuSeparator {}

        MenuItem {
            text: qsTr("Copy File Path (Ctrl+C)")
            onTriggered: {
                if (selectionManager.selectedCount > 1 && root.isSelected) {
                    sessionController.copySelectedPathsToClipboard()
                } else {
                    selectionManager.select(root.itemIndex)
                    sessionController.copySelectedPathsToClipboard()
                }
            }
        }

        MenuItem {
            text: qsTr("Reveal in File Manager")
            enabled: !model.isDemoFixture
            onTriggered: sessionController.revealInFileManager(root.itemIndex)
        }

        MenuSeparator {}

        MenuItem {
            text: (selectionManager.selectedCount > 1 && root.isSelected)
                  ? qsTr("Export Selected Photos... (Ctrl+E)")
                  : qsTr("Export Photo... (Ctrl+E)")
            onTriggered: sessionController.openExportDialog()
        }

        MenuSeparator {}

        MenuItem {
            text: qsTr("Select All (Ctrl+A)")
            onTriggered: selectionManager.selectAll()
        }

        MenuItem {
            text: qsTr("Deselect All (Esc)")
            enabled: selectionManager.hasSelection
            onTriggered: selectionManager.clearSelection()
        }
    }
}
