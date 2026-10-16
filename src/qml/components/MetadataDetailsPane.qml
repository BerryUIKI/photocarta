// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property var item: selectionManager.selectedItem

    implicitWidth: 280
    color: themeManager.surface
    border.color: themeManager.border
    border.width: 1

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        Item {
            width: root.width
            implicitHeight: contentCol.implicitHeight + 36

            Column {
                id: contentCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                topPadding: 16
                bottomPadding: 20
                spacing: 16

            // Empty selection placeholder
            Column {
                width: parent.width
                spacing: 12
                visible: !selectionManager.hasSelection || !root.item.id

                Item { width: 1; height: 40 }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "📋"
                    font.pixelSize: 32
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("No Photo Selected")
                    font.pixelSize: themeManager.fontBase
                    font.weight: Font.DemiBold
                    color: themeManager.textPrimary
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width - 20
                    wrapMode: Text.Wrap
                    text: qsTr("Select a photo in the grid to inspect technical EXIF parameters, file details, and tags.")
                    font.pixelSize: themeManager.fontSm
                    color: themeManager.textMuted
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // Populated Metadata Pane
            Column {
                width: parent.width
                spacing: 14
                visible: selectionManager.hasSelection && !!root.item.id

                // Thumbnail Card Preview Header
                Rectangle {
                    width: parent.width
                    height: 140
                    radius: themeManager.radiusMd
                    color: themeManager.surfaceElevated
                    border.color: themeManager.border
                    border.width: 1
                    clip: true

                    Image {
                        anchors.fill: parent
                        source: root.item.thumbnailUrl ? root.item.thumbnailUrl : ""
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }

                    Rectangle {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: 6
                        visible: root.item.isDemoFixture === true
                        width: cardDemoPill.implicitWidth + 8
                        height: 18
                        radius: 4
                        color: themeManager.badgeDemoBg
                        border.color: themeManager.badgeDemoText
                        border.width: 1

                        Text {
                            id: cardDemoPill
                            anchors.centerIn: parent
                            text: qsTr("DEMO")
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            color: themeManager.badgeDemoText
                        }
                    }
                }

                // Title & Filename
                Text {
                    width: parent.width
                    text: root.item.title ? root.item.title : (root.item.fileName ? root.item.fileName : "")
                    font.pixelSize: themeManager.fontBase
                    font.weight: Font.Bold
                    color: themeManager.textPrimary
                    wrapMode: Text.Wrap
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: themeManager.border
                }

                // FILE DETAILS SECTION
                Text {
                    text: qsTr("FILE DETAILS")
                    font.pixelSize: themeManager.fontXs
                    font.weight: Font.Bold
                    color: themeManager.textMuted
                    font.letterSpacing: 1.2
                }

                Column {
                    width: parent.width
                    spacing: 8

                    Row {
                        width: parent.width
                        Text { width: 90; text: qsTr("Filename:"); font.pixelSize: 11; color: themeManager.textSecondary }
                        Text { width: parent.width - 90; text: root.item.fileName ? root.item.fileName : "-"; font.pixelSize: 11; color: themeManager.textPrimary; elide: Text.ElideMiddle }
                    }

                    Row {
                        width: parent.width
                        Text { width: 90; text: qsTr("Dimensions:"); font.pixelSize: 11; color: themeManager.textSecondary }
                        Text { width: parent.width - 90; text: root.item.formattedDimensions ? root.item.formattedDimensions : "-"; font.pixelSize: 11; color: themeManager.textPrimary }
                    }

                    Row {
                        width: parent.width
                        Text { width: 90; text: qsTr("File Size:"); font.pixelSize: 11; color: themeManager.textSecondary }
                        Text { width: parent.width - 90; text: root.item.formattedFileSize ? root.item.formattedFileSize : "-"; font.pixelSize: 11; color: themeManager.textPrimary }
                    }

                    Row {
                        width: parent.width
                        Text { width: 90; text: qsTr("MIME Type:"); font.pixelSize: 11; color: themeManager.textSecondary }
                        Text { width: parent.width - 90; text: root.item.mimeType ? root.item.mimeType : "-"; font.pixelSize: 11; color: themeManager.textPrimary }
                    }

                    Row {
                        width: parent.width
                        Text { width: 90; text: qsTr("Date Taken:"); font.pixelSize: 11; color: themeManager.textSecondary }
                        Text { width: parent.width - 90; text: root.item.formattedDate ? root.item.formattedDate : "-"; font.pixelSize: 11; color: themeManager.textPrimary }
                    }

                    Row {
                        width: parent.width
                        Text { width: 90; text: qsTr("Modified:"); font.pixelSize: 11; color: themeManager.textSecondary }
                        Text { width: parent.width - 90; text: root.item.formattedLastModified ? root.item.formattedLastModified : "-"; font.pixelSize: 11; color: themeManager.textPrimary }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: themeManager.border
                }

                // CAMERA & EXIF SECTION
                Text {
                    text: qsTr("CAMERA & EXIF")
                    font.pixelSize: themeManager.fontXs
                    font.weight: Font.Bold
                    color: themeManager.textMuted
                    font.letterSpacing: 1.2
                }

                Column {
                    width: parent.width
                    spacing: 8

                    Row {
                        width: parent.width
                        Text { width: 90; text: qsTr("Camera:"); font.pixelSize: 11; color: themeManager.textSecondary }
                        Text { width: parent.width - 90; text: root.item.cameraModel ? root.item.cameraModel : "-"; font.pixelSize: 11; color: themeManager.textPrimary; wrapMode: Text.Wrap }
                    }

                    Row {
                        width: parent.width
                        Text { width: 90; text: qsTr("Lens:"); font.pixelSize: 11; color: themeManager.textSecondary }
                        Text { width: parent.width - 90; text: root.item.lensModel ? root.item.lensModel : "-"; font.pixelSize: 11; color: themeManager.textPrimary; wrapMode: Text.Wrap }
                    }

                    Row {
                        width: parent.width
                        Text { width: 90; text: qsTr("Focal Length:"); font.pixelSize: 11; color: themeManager.textSecondary }
                        Text { width: parent.width - 90; text: root.item.focalLength ? root.item.focalLength : "-"; font.pixelSize: 11; color: themeManager.textPrimary }
                    }

                    Row {
                        width: parent.width
                        Text { width: 90; text: qsTr("Aperture:"); font.pixelSize: 11; color: themeManager.textSecondary }
                        Text { width: parent.width - 90; text: root.item.aperture ? root.item.aperture : "-"; font.pixelSize: 11; color: themeManager.textPrimary }
                    }

                    Row {
                        width: parent.width
                        Text { width: 90; text: qsTr("Shutter:"); font.pixelSize: 11; color: themeManager.textSecondary }
                        Text { width: parent.width - 90; text: root.item.shutterSpeed ? root.item.shutterSpeed : "-"; font.pixelSize: 11; color: themeManager.textPrimary }
                    }

                    Row {
                        width: parent.width
                        Text { width: 90; text: qsTr("ISO:"); font.pixelSize: 11; color: themeManager.textSecondary }
                        Text { width: parent.width - 90; text: root.item.iso ? root.item.iso.toString() : "-"; font.pixelSize: 11; color: themeManager.textPrimary }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: themeManager.border
                }

                // RATING & TAGS SECTION
                Text {
                    text: qsTr("RATING & TAGS (DISPLAY FIXTURES)")
                    font.pixelSize: themeManager.fontXs
                    font.weight: Font.Bold
                    color: themeManager.textMuted
                    font.letterSpacing: 1.2
                }

                Column {
                    width: parent.width
                    spacing: 10

                    Row {
                        spacing: 10
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Rating:")
                            font.pixelSize: 11
                            color: themeManager.textSecondary
                        }
                        RatingStars {
                            anchors.verticalCenter: parent.verticalCenter
                            rating: root.item.rating ? root.item.rating : 0
                            interactive: true
                        }
                    }

                    Flow {
                        width: parent.width
                        spacing: 6

                        Repeater {
                            model: root.item.tags ? root.item.tags : []
                            TagChip {
                                text: modelData
                                interactive: false
                            }
                        }
                    }
                }

                // Non-persistent fixture notice
                Rectangle {
                    width: parent.width
                    implicitHeight: fixNoticeText.implicitHeight + 14
                    radius: themeManager.radiusSm
                    color: themeManager.surfaceElevated
                    border.color: themeManager.border
                    border.width: 1

                    Text {
                        id: fixNoticeText
                        anchors.fill: parent
                        anchors.margins: 8
                        wrapMode: Text.Wrap
                        text: qsTr("Ratings and tags in this temporary session are read-only. Database persistence will connect in upcoming releases.")
                        font.pixelSize: 10
                        lineHeight: 1.2
                        color: themeManager.textMuted
                    }
                }
            }
        }
    }
    }
}
