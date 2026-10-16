// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Item {
    id: root

    property var currentItem: selectionManager.selectedItem

    anchors.fill: parent
    focus: sessionController.previewMode

    // Dark neutral canvas for viewing photos
    Rectangle {
        anchors.fill: parent
        color: "#0a0b0e"
    }

    // TOP PREVIEW BAR
    Rectangle {
        id: previewHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 48
        color: "#16171d"
        border.color: "#2a2c37"
        border.width: 1
        z: 10

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            // Back to Grid Button
            Rectangle {
                width: backRow.implicitWidth + 16
                height: 32
                radius: themeManager.radiusMd
                color: backMouse.containsMouse ? "#2f313d" : "#22242c"
                border.color: "#3a3d4c"
                border.width: 1

                Row {
                    id: backRow
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "←"
                        font.pixelSize: 14
                        color: "#f1f5f9"
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Grid View (Esc)")
                        font.pixelSize: themeManager.fontSm
                        font.weight: Font.Medium
                        color: "#f1f5f9"
                    }
                }

                MouseArea {
                    id: backMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.setPreviewMode(false)
                }
            }

            Rectangle {
                width: 1
                height: 20
                anchors.verticalCenter: parent.verticalCenter
                color: "#2a2c37"
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.currentItem.title ? root.currentItem.title : (root.currentItem.fileName ? root.currentItem.fileName : "")
                font.pixelSize: themeManager.fontSm
                font.weight: Font.DemiBold
                color: "#f1f5f9"
                elide: Text.ElideRight
            }

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.currentItem.isDemoFixture === true
                width: previewDemoText.implicitWidth + 8
                height: 18
                radius: 4
                color: themeManager.badgeDemoBg
                border.color: themeManager.badgeDemoText
                border.width: 1

                Text {
                    id: previewDemoText
                    anchors.centerIn: parent
                    text: qsTr("DEMO FIXTURE")
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: themeManager.badgeDemoText
                }
            }
        }

        // Photo Index Counter
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("%1 of %2").arg(selectionManager.currentIndex + 1).arg(filterModel.count)
            font.pixelSize: themeManager.fontSm
            color: "#94a3b8"
        }
    }

    // MAIN IMAGE DISPLAY AREA
    Item {
        id: imageArea
        anchors.top: previewHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: filmstripBar.top
        clip: true

        Image {
            id: mainPreviewImage
            anchors.fill: parent
            anchors.margins: 20
            source: root.currentItem.previewUrl ? root.currentItem.previewUrl : ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: true
            // Bound high-resolution textures to 2560x1600 to prevent unbounded GPU allocation
            sourceSize.width: 2560
            sourceSize.height: 1600

            Rectangle {
                anchors.fill: parent
                color: "#0a0b0e"
                opacity: mainPreviewImage.status === Image.Loading ? 0.6 : 0.0
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }
        }

        // Floating Technical EXIF HUD Badge
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 16
            anchors.horizontalCenter: parent.horizontalCenter
            width: exifRow.implicitWidth + 24
            height: 32
            radius: 16
            color: "#cc0f131a"
            border.color: "#334155"
            border.width: 1
            visible: !!root.currentItem.cameraModel

            Row {
                id: exifRow
                anchors.centerIn: parent
                spacing: 10

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.currentItem.cameraModel
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    color: "#f8fafc"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "•"
                    font.pixelSize: 11
                    color: "#64748b"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.currentItem.lensModel
                    font.pixelSize: 11
                    color: "#94a3b8"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "•"
                    font.pixelSize: 11
                    color: "#64748b"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("%1  %2  %3  ISO %4")
                          .arg(root.currentItem.focalLength)
                          .arg(root.currentItem.aperture)
                          .arg(root.currentItem.shutterSpeed)
                          .arg(root.currentItem.iso)
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    color: "#38bdf8"
                }
            }
        }

        // Left Navigation Arrow Overlay
        Rectangle {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            width: 44
            height: 44
            radius: 22
            color: leftNavMouse.containsMouse ? "#cc1e293b" : "#660f172a"
            border.color: "#475569"
            border.width: 1
            visible: selectionManager.currentIndex > 0

            Text {
                anchors.centerIn: parent
                text: "‹"
                font.pixelSize: 26
                font.weight: Font.Light
                color: "#f8fafc"
            }

            MouseArea {
                id: leftNavMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: selectionManager.selectPrevious()
            }
        }

        // Right Navigation Arrow Overlay
        Rectangle {
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            width: 44
            height: 44
            radius: 22
            color: rightNavMouse.containsMouse ? "#cc1e293b" : "#660f172a"
            border.color: "#475569"
            border.width: 1
            visible: selectionManager.currentIndex + 1 < filterModel.count

            Text {
                anchors.centerIn: parent
                text: "›"
                font.pixelSize: 26
                font.weight: Font.Light
                color: "#f8fafc"
            }

            MouseArea {
                id: rightNavMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: selectionManager.selectNext()
            }
        }
    }

    // BOTTOM FILMSTRIP MINI-BAR
    Rectangle {
        id: filmstripBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 80
        color: "#121319"
        border.color: "#242630"
        border.width: 1

        ListView {
            id: filmstripList
            anchors.fill: parent
            anchors.margins: 8
            orientation: ListView.Horizontal
            spacing: 8
            clip: true
            model: filterModel

            delegate: Rectangle {
                id: stripThumb
                width: 76
                height: 64
                radius: 4
                color: "#1e2029"
                border.color: (selectionManager.currentIndex === index) ? themeManager.accent : "#343846"
                border.width: (selectionManager.currentIndex === index) ? 2 : 1
                clip: true

                Image {
                    anchors.fill: parent
                    anchors.margins: 2
                    source: model.thumbnailUrl
                    fillMode: Image.PreserveAspectCrop
                    sourceSize.width: 160
                    sourceSize.height: 120
                    asynchronous: true
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: selectionManager.select(index)
                }
            }

            ScrollBar.horizontal: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
        }
    }

    // Keyboard Shortcuts
    Keys.onEscapePressed: sessionController.setPreviewMode(false)
    Keys.onLeftPressed: selectionManager.selectPrevious()
    Keys.onRightPressed: selectionManager.selectNext()
}
