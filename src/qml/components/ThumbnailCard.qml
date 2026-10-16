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

        onClicked: (mouse) => {
            if (mouse.modifiers & Qt.ControlModifier) {
                selectionManager.toggleSelect(root.itemIndex)
            } else {
                selectionManager.select(root.itemIndex)
            }
        }

        onDoubleClicked: {
            selectionManager.select(root.itemIndex)
            sessionController.setPreviewMode(true)
            root.activated()
        }
    }
}
