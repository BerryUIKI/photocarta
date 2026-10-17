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
                visible: !selectionManager.hasSelection || selectionManager.selectedCount === 0

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

            // Batch Selection Metadata Pane
            Column {
                width: parent.width
                spacing: 16
                visible: selectionManager.selectedCount > 1

                // Batch Header Card
                Rectangle {
                    width: parent.width
                    height: 100
                    radius: themeManager.radiusMd
                    color: themeManager.surfaceElevated
                    border.color: themeManager.border
                    border.width: 1
                    clip: true

                    // Mini thumbnails preview stack / grid (up to 4 thumbnails)
                    Row {
                        anchors.centerIn: parent
                        spacing: 6

                        Repeater {
                            model: selectionManager.selectedItemsSummary(4)
                            Rectangle {
                                width: 50
                                height: 50
                                radius: 6
                                color: "#11141c"
                                border.color: themeManager.border
                                border.width: 1
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    source: modelData.thumbnailUrl
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                }
                            }
                        }
                    }

                    // Count Badge
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        anchors.margins: 8
                        width: batchBadgeText.implicitWidth + 12
                        height: 20
                        radius: 10
                        color: themeManager.accent

                        Text {
                            id: batchBadgeText
                            anchors.centerIn: parent
                            text: qsTr("%1 photos").arg(selectionManager.selectedCount)
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: themeManager.accentText
                        }
                    }
                }

                // Title & Deselect Action
                Row {
                    width: parent.width

                    Column {
                        width: parent.width - clearSelBtn.width
                        spacing: 2

                        Text {
                            text: qsTr("Batch Editing")
                            font.pixelSize: themeManager.fontBase
                            font.weight: Font.Bold
                            color: themeManager.textPrimary
                        }

                        Text {
                            text: qsTr("%1 items selected").arg(selectionManager.selectedCount)
                            font.pixelSize: 11
                            color: themeManager.textSecondary
                        }
                    }

                    Rectangle {
                        id: clearSelBtn
                        width: clearSelText.implicitWidth + 14
                        height: 26
                        radius: themeManager.radiusSm
                        color: clearSelMouse.containsMouse ? themeManager.surfaceHover : "transparent"
                        border.color: themeManager.border
                        border.width: 1

                        Text {
                            id: clearSelText
                            anchors.centerIn: parent
                            text: qsTr("Deselect")
                            font.pixelSize: 11
                            color: themeManager.textSecondary
                        }

                        MouseArea {
                            id: clearSelMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: selectionManager.clearSelection()
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: themeManager.border
                }

                // BATCH STAR RATING
                Text {
                    text: qsTr("BATCH RATING")
                    font.pixelSize: themeManager.fontXs
                    font.weight: Font.Bold
                    color: themeManager.textMuted
                    font.letterSpacing: 1.2
                }

                Column {
                    width: parent.width
                    spacing: 8

                    Row {
                        spacing: 10
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Set Rating:")
                            font.pixelSize: 11
                            color: themeManager.textSecondary
                        }
                        RatingStars {
                            anchors.verticalCenter: parent.verticalCenter
                            rating: 0
                            interactive: true
                            onRatingSelected: (newRating) => {
                                sessionController.setRatingForSelected(newRating)
                            }
                        }
                    }

                    Text {
                        text: qsTr("Click a star to apply rating to all %1 selected photos.").arg(selectionManager.selectedCount)
                        font.pixelSize: 10
                        color: themeManager.textMuted
                        wrapMode: Text.Wrap
                        width: parent.width
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: themeManager.border
                }

                // BATCH FAVORITES / FLAGGING
                Text {
                    text: qsTr("BATCH FLAGGING")
                    font.pixelSize: themeManager.fontXs
                    font.weight: Font.Bold
                    color: themeManager.textMuted
                    font.letterSpacing: 1.2
                }

                Row {
                    width: parent.width
                    spacing: 8

                    Rectangle {
                        width: (parent.width - 8) / 2
                        height: 30
                        radius: themeManager.radiusSm
                        color: flagAllMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
                        border.color: themeManager.border
                        border.width: 1

                        Row {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: "⭐"; font.pixelSize: 11 }
                            Text { text: qsTr("Flag All"); font.pixelSize: 11; font.weight: Font.Medium; color: themeManager.textPrimary }
                        }

                        MouseArea {
                            id: flagAllMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sessionController.setFlaggedForSelected(true)
                        }
                    }

                    Rectangle {
                        width: (parent.width - 8) / 2
                        height: 30
                        radius: themeManager.radiusSm
                        color: unflagAllMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
                        border.color: themeManager.border
                        border.width: 1

                        Row {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: "✕"; font.pixelSize: 10; color: themeManager.textSecondary }
                            Text { text: qsTr("Unflag All"); font.pixelSize: 11; font.weight: Font.Medium; color: themeManager.textPrimary }
                        }

                        MouseArea {
                            id: unflagAllMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sessionController.setFlaggedForSelected(false)
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: themeManager.border
                }

                // BATCH TAGS
                Text {
                    text: qsTr("SHARED TAGS")
                    font.pixelSize: themeManager.fontXs
                    font.weight: Font.Bold
                    color: themeManager.textMuted
                    font.letterSpacing: 1.2
                }

                Flow {
                    width: parent.width
                    spacing: 6

                    Repeater {
                        model: selectionManager.commonTags
                        TagChip {
                            text: modelData
                            interactive: true
                            removable: true
                            active: filterModel.tagFilter.toLowerCase() === modelData.toLowerCase()
                            onClicked: {
                                if (filterModel.tagFilter.toLowerCase() === modelData.toLowerCase()) {
                                    sessionController.clearTagFilter()
                                } else {
                                    sessionController.setTagFilter(modelData)
                                }
                            }
                            onRemoveRequested: {
                                sessionController.removeTagFromSelected(modelData)
                            }
                        }
                    }

                    Text {
                        visible: selectionManager.commonTags.length === 0
                        text: qsTr("No shared tags common to all selected photos")
                        font.pixelSize: 10
                        color: themeManager.textMuted
                        wrapMode: Text.Wrap
                        width: parent.width
                    }
                }

                // Add Tag to All Row
                Row {
                    width: parent.width
                    spacing: 6

                    Rectangle {
                        id: batchTagInputBox
                        width: parent.width - addBatchTagBtn.width - 6
                        height: 28
                        radius: themeManager.radiusSm
                        color: themeManager.surfaceElevated
                        border.color: batchTagInput.activeFocus ? themeManager.accent : themeManager.border
                        border.width: 1

                        TextInput {
                            id: batchTagInput
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            verticalAlignment: TextInput.AlignVCenter
                            font.pixelSize: 11
                            color: themeManager.textPrimary
                            selectByMouse: true

                            Text {
                                anchors.fill: parent
                                verticalAlignment: Text.AlignVCenter
                                text: qsTr("Add tag to all selected...")
                                font.pixelSize: 11
                                color: themeManager.textMuted
                                visible: !batchTagInput.text && !batchTagInput.activeFocus
                            }

                            onAccepted: {
                                const trimmed = batchTagInput.text.trim()
                                if (trimmed.length > 0) {
                                    sessionController.addTagToSelected(trimmed)
                                    batchTagInput.text = ""
                                }
                            }
                        }
                    }

                    Rectangle {
                        id: addBatchTagBtn
                        width: 52
                        height: 28
                        radius: themeManager.radiusSm
                        color: addBatchTagMouse.containsMouse ? themeManager.accentHover : themeManager.accent

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("+ Add")
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            color: themeManager.accentText
                        }

                        MouseArea {
                            id: addBatchTagMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                const trimmed = batchTagInput.text.trim()
                                if (trimmed.length > 0) {
                                    sessionController.addTagToSelected(trimmed)
                                    batchTagInput.text = ""
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: themeManager.border
                }

                // BATCH ACTIONS
                Text {
                    text: qsTr("QUICK ACTIONS")
                    font.pixelSize: themeManager.fontXs
                    font.weight: Font.Bold
                    color: themeManager.textMuted
                    font.letterSpacing: 1.2
                }

                Rectangle {
                    width: parent.width
                    height: 32
                    radius: themeManager.radiusSm
                    color: copyPathsMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
                    border.color: themeManager.border
                    border.width: 1

                    Row {
                        anchors.centerIn: parent
                        spacing: 6
                        Text { text: "📋"; font.pixelSize: 12 }
                        Text { text: qsTr("Copy File Paths"); font.pixelSize: 11; font.weight: Font.Medium; color: themeManager.textPrimary }
                    }

                    MouseArea {
                        id: copyPathsMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.copySelectedPathsToClipboard()
                    }
                }
            }

            // Populated Single Photo Metadata Pane
            Column {
                width: parent.width
                spacing: 14
                visible: selectionManager.hasSelection && selectionManager.selectedCount === 1 && !!root.item.id

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
                    text: qsTr("RATING & TAGS")
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
                            onRatingSelected: (newRating) => {
                                sessionController.setItemRating(selectionManager.currentIndex, newRating)
                            }
                        }
                    }

                    Flow {
                        width: parent.width
                        spacing: 6

                        Repeater {
                            model: root.item.tags ? root.item.tags : []
                            TagChip {
                                text: modelData
                                interactive: true
                                removable: true
                                active: filterModel.tagFilter.toLowerCase() === modelData.toLowerCase()
                                onClicked: {
                                    if (filterModel.tagFilter.toLowerCase() === modelData.toLowerCase()) {
                                        sessionController.clearTagFilter()
                                    } else {
                                        sessionController.setTagFilter(modelData)
                                    }
                                }
                                onRemoveRequested: {
                                    sessionController.removeTagFromItem(selectionManager.currentIndex, modelData)
                                }
                            }
                        }

                        Text {
                            visible: !root.item.tags || root.item.tags.length === 0
                            text: qsTr("No tags assigned")
                            font.pixelSize: 11
                            color: themeManager.textMuted
                        }
                    }

                    // Add Tag Input Row
                    Row {
                        width: parent.width
                        spacing: 6

                        Rectangle {
                            id: tagInputBox
                            width: parent.width - addTagBtn.width - 6
                            height: 28
                            radius: themeManager.radiusSm
                            color: themeManager.surfaceElevated
                            border.color: tagInput.activeFocus ? themeManager.accent : themeManager.border
                            border.width: 1

                            TextInput {
                                id: tagInput
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                verticalAlignment: TextInput.AlignVCenter
                                font.pixelSize: 11
                                color: themeManager.textPrimary
                                selectByMouse: true

                                Text {
                                    anchors.fill: parent
                                    verticalAlignment: Text.AlignVCenter
                                    text: qsTr("Add a tag...")
                                    font.pixelSize: 11
                                    color: themeManager.textMuted
                                    visible: !tagInput.text && !tagInput.activeFocus
                                }

                                onAccepted: {
                                    const trimmed = tagInput.text.trim()
                                    if (trimmed.length > 0) {
                                        sessionController.addTagToItem(selectionManager.currentIndex, trimmed)
                                        tagInput.text = ""
                                    }
                                }
                            }
                        }

                        Rectangle {
                            id: addTagBtn
                            width: 52
                            height: 28
                            radius: themeManager.radiusSm
                            color: addTagMouseArea.containsMouse ? themeManager.accentHover : themeManager.accent

                            Text {
                                anchors.centerIn: parent
                                text: qsTr("+ Add")
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                color: themeManager.accentText
                            }

                            MouseArea {
                                id: addTagMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    const trimmed = tagInput.text.trim()
                                    if (trimmed.length > 0) {
                                        sessionController.addTagToItem(selectionManager.currentIndex, trimmed)
                                        tagInput.text = ""
                                    }
                                }
                            }
                        }
                    }
                }

                // Persistence status notice
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
                        text: sessionController.hasDatabase
                              ? qsTr("Metadata, star ratings, and tags are persisted in your local SQLite catalog.")
                              : qsTr("Ratings and tags are stored in temporary session memory.")
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
