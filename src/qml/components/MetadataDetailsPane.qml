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

                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Deselect all photos")
                        Accessible.onPressAction: selectionManager.clearSelection()
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                selectionManager.clearSelection()
                                event.accepted = true
                            }
                        }

                        MouseArea {
                            id: clearSelMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: selectionManager.clearSelection()
                        }

                        FocusRing { targetRadius: themeManager.radiusSm }
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

                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Flag all selected photos as favorites")
                        Accessible.onPressAction: sessionController.setFlaggedForSelected(true)
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                sessionController.setFlaggedForSelected(true)
                                event.accepted = true
                            }
                        }

                        MouseArea {
                            id: flagAllMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sessionController.setFlaggedForSelected(true)
                        }

                        FocusRing { targetRadius: themeManager.radiusSm }
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

                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Unflag all selected photos")
                        Accessible.onPressAction: sessionController.setFlaggedForSelected(false)
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                sessionController.setFlaggedForSelected(false)
                                event.accepted = true
                            }
                        }

                        MouseArea {
                            id: unflagAllMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sessionController.setFlaggedForSelected(false)
                        }

                        FocusRing { targetRadius: themeManager.radiusSm }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: themeManager.border
                }

                // BATCH COLOR LABEL
                Text {
                    text: qsTr("BATCH COLOR LABEL")
                    font.pixelSize: themeManager.fontXs
                    font.weight: Font.Bold
                    color: themeManager.textMuted
                    font.letterSpacing: 1.2
                }

                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Repeater {
                        model: [
                            { id: 0, label: qsTr("Clear Color"), color: "transparent", border: themeManager.textMuted },
                            { id: 1, label: qsTr("Red (6)"), color: "#ef4444", border: "#dc2626" },
                            { id: 2, label: qsTr("Yellow (7)"), color: "#eab308", border: "#ca8a04" },
                            { id: 3, label: qsTr("Green (8)"), color: "#22c55e", border: "#16a34a" },
                            { id: 4, label: qsTr("Blue (9)"), color: "#3b82f6", border: "#2563eb" },
                            { id: 5, label: qsTr("Purple"), color: "#a855f7", border: "#9333ea" }
                        ]

                        Rectangle {
                            width: 22
                            height: 22
                            radius: 11
                            color: modelData.color
                            border.color: modelData.border
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                visible: modelData.id === 0
                                text: "✕"
                                font.pixelSize: 9
                                color: themeManager.textMuted
                            }

                            activeFocusOnTab: true
                            Accessible.role: Accessible.RadioButton
                            Accessible.name: modelData.label
                            Accessible.checkable: true
                            Accessible.checked: false
                            Accessible.onPressAction: sessionController.setColorLabelForSelected(modelData.id)
                            Keys.onPressed: (event) => {
                                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                    sessionController.setColorLabelForSelected(modelData.id)
                                    event.accepted = true
                                }
                            }

                            ToolTip.visible: batchColMouse.containsMouse
                            ToolTip.text: modelData.label

                            MouseArea {
                                id: batchColMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: sessionController.setColorLabelForSelected(modelData.id)
                            }

                            FocusRing { targetRadius: themeManager.radiusPill }
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
                            activeFocusOnTab: true
                            Accessible.role: Accessible.EditableText
                            Accessible.name: qsTr("New tag for all selected photos")

                            function applyTag() {
                                const trimmed = text.trim()
                                if (trimmed.length > 0) {
                                    sessionController.addTagToSelected(trimmed)
                                    text = ""
                                }
                            }

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

                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Add tag to all selected photos")
                        Accessible.onPressAction: batchTagInput.applyTag()
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                batchTagInput.applyTag()
                                event.accepted = true
                            }
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

                        FocusRing { targetRadius: themeManager.radiusSm }
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

                    activeFocusOnTab: true
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Copy file paths")
                    Accessible.onPressAction: sessionController.copySelectedPathsToClipboard()
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            sessionController.copySelectedPathsToClipboard()
                            event.accepted = true
                        }
                    }

                    MouseArea {
                        id: copyPathsMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.copySelectedPathsToClipboard()
                    }

                    FocusRing { targetRadius: themeManager.radiusSm }
                }

                Rectangle {
                    width: parent.width
                    height: 32
                    radius: themeManager.radiusSm
                    color: exportBatchMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
                    border.color: themeManager.border
                    border.width: 1

                    Row {
                        anchors.centerIn: parent
                        spacing: 6
                        Text { text: "📤"; font.pixelSize: 12 }
                        Text { text: qsTr("Export Selected... (Ctrl+E)"); font.pixelSize: 11; font.weight: Font.Medium; color: themeManager.textPrimary }
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Export selected photos")
                    Accessible.onPressAction: sessionController.openExportDialog()
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            sessionController.openExportDialog()
                            event.accepted = true
                        }
                    }

                    MouseArea {
                        id: exportBatchMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.openExportDialog()
                    }

                    FocusRing { targetRadius: themeManager.radiusSm }
                }

                Row {
                    width: parent.width
                    spacing: 8

                    Rectangle {
                        width: (parent.width - 8) / 2
                        height: 32
                        radius: themeManager.radiusSm
                        color: batchSaveXmpMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
                        border.color: themeManager.border
                        border.width: 1

                        Row {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: "💾"; font.pixelSize: 11 }
                            Text { text: qsTr("Save XMP"); font.pixelSize: 11; font.weight: Font.Medium; color: themeManager.textPrimary }
                        }

                        ToolTip.visible: batchSaveXmpMouse.containsMouse
                        ToolTip.text: qsTr("Write metadata to companion .xmp sidecar files")

                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Save metadata to XMP sidecars")
                        Accessible.onPressAction: sessionController.exportXmpSidecarsForSelected()
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                sessionController.exportXmpSidecarsForSelected()
                                event.accepted = true
                            }
                        }

                        MouseArea {
                            id: batchSaveXmpMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sessionController.exportXmpSidecarsForSelected()
                        }

                        FocusRing { targetRadius: themeManager.radiusSm }
                    }

                    Rectangle {
                        width: (parent.width - 8) / 2
                        height: 32
                        radius: themeManager.radiusSm
                        color: batchReloadXmpMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
                        border.color: themeManager.border
                        border.width: 1

                        Row {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: "🔄"; font.pixelSize: 11 }
                            Text { text: qsTr("Reload XMP"); font.pixelSize: 11; font.weight: Font.Medium; color: themeManager.textPrimary }
                        }

                        ToolTip.visible: batchReloadXmpMouse.containsMouse
                        ToolTip.text: qsTr("Reload metadata from companion .xmp sidecars")

                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Reload metadata from XMP sidecars")
                        Accessible.onPressAction: sessionController.importXmpSidecarsForSelected()
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                sessionController.importXmpSidecarsForSelected()
                                event.accepted = true
                            }
                        }

                        MouseArea {
                            id: batchReloadXmpMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sessionController.importXmpSidecarsForSelected()
                        }

                        FocusRing { targetRadius: themeManager.radiusSm }
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
                Column {
                    width: parent.width
                    spacing: 4

                    Rectangle {
                        width: parent.width
                        height: 32
                        radius: themeManager.radiusSm
                        color: titleInput.activeFocus ? themeManager.surfaceSelected : themeManager.surfaceElevated
                        border.color: titleInput.activeFocus ? themeManager.borderFocus : (titleInputMouse.containsMouse ? themeManager.textMuted : themeManager.border)
                        border.width: 1

                        TextInput {
                            id: titleInput
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Bold
                            color: themeManager.textPrimary
                            clip: true
                            activeFocusOnTab: true
                            Accessible.role: Accessible.EditableText
                            Accessible.name: qsTr("Photo title")
                            selectByMouse: true
                            text: root.item.title || ""

                            Text {
                                anchors.fill: parent
                                verticalAlignment: Text.AlignVCenter
                                visible: !titleInput.text && !titleInput.activeFocus
                                text: root.item.fileName ? qsTr("Untitled (%1)").arg(root.item.fileName) : qsTr("Add title...")
                                font.pixelSize: themeManager.fontSm
                                color: themeManager.textMuted
                                elide: Text.ElideRight
                            }

                            onEditingFinished: {
                                if (text !== (root.item.title || "")) {
                                    sessionController.setItemTitle(selectionManager.currentIndex, text)
                                }
                            }
                        }

                        MouseArea {
                            id: titleInputMouse
                            anchors.fill: parent
                            cursorShape: Qt.IBeamCursor
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    Text {
                        width: parent.width
                        text: root.item.fileName ? root.item.fileName : ""
                        font.pixelSize: 10
                        color: themeManager.textMuted
                        elide: Text.ElideMiddle
                    }
                }

                // Caption / Description Section
                Column {
                    width: parent.width
                    spacing: 4

                    Text {
                        text: qsTr("CAPTION / NOTES")
                        font.pixelSize: themeManager.fontXs
                        font.weight: Font.Bold
                        color: themeManager.textMuted
                        font.letterSpacing: 1.2
                    }

                    Rectangle {
                        width: parent.width
                        height: 64
                        radius: themeManager.radiusSm
                        color: captionInput.activeFocus ? themeManager.surfaceSelected : themeManager.surfaceElevated
                        border.color: captionInput.activeFocus ? themeManager.borderFocus : (captionInputMouse.containsMouse ? themeManager.textMuted : themeManager.border)
                        border.width: 1

                        Flickable {
                            id: captionFlick
                            anchors.fill: parent
                            anchors.margins: 6
                            contentWidth: width
                            contentHeight: captionInput.paintedHeight
                            clip: true

                            TextEdit {
                                id: captionInput
                                width: captionFlick.width
                                wrapMode: TextEdit.Wrap
                                font.pixelSize: 11
                                color: themeManager.textPrimary
                                activeFocusOnTab: true
                                Accessible.role: Accessible.EditableText
                                Accessible.name: qsTr("Caption or notes")
                                selectByMouse: true
                                text: root.item.caption || ""

                                Text {
                                    anchors.fill: parent
                                    visible: !captionInput.text && !captionInput.activeFocus
                                    text: qsTr("Add description or notes...")
                                    font.pixelSize: 11
                                    color: themeManager.textMuted
                                    wrapMode: Text.Wrap
                                }

                                onEditingFinished: {
                                    if (text !== (root.item.caption || "")) {
                                        sessionController.setItemCaption(selectionManager.currentIndex, text)
                                    }
                                }
                            }
                        }

                        MouseArea {
                            id: captionInputMouse
                            anchors.fill: parent
                            cursorShape: Qt.IBeamCursor
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: themeManager.border
                }

                // HISTOGRAM SECTION
                Item {
                    width: parent.width
                    height: 16

                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("HISTOGRAM")
                        font.pixelSize: themeManager.fontXs
                        font.weight: Font.Bold
                        color: themeManager.textMuted
                        font.letterSpacing: 1.2
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: sessionController.histogramVisible ? qsTr("Hide") : qsTr("Show")
                        font.pixelSize: 10
                        font.weight: Font.Medium
                        color: themeManager.accent

                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: sessionController.histogramVisible ? qsTr("Hide histogram") : qsTr("Show histogram")
                        Accessible.onPressAction: sessionController.toggleHistogramVisible()
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                sessionController.toggleHistogramVisible()
                                event.accepted = true
                            }
                        }

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: -2
                            radius: 3
                            color: "transparent"
                            border.color: themeManager.borderFocus
                            border.width: 1
                            visible: parent.activeFocus
                            Accessible.ignored: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sessionController.toggleHistogramVisible()
                        }
                    }
                }

                HistogramView {
                    width: parent.width
                    visible: sessionController.histogramVisible
                    compact: false
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

                // XMP SIDECAR SECTION
                Text {
                    text: qsTr("XMP SIDECAR")
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
                        spacing: 8

                        Rectangle {
                            width: (parent.width - 8) / 2
                            height: 28
                            radius: themeManager.radiusSm
                            color: exportXmpMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
                            border.color: themeManager.border
                            border.width: 1

                            Row {
                                anchors.centerIn: parent
                                spacing: 4
                                Text { text: "💾"; font.pixelSize: 10 }
                                Text { text: qsTr("Save XMP"); font.pixelSize: 11; font.weight: Font.Medium; color: themeManager.textPrimary }
                            }

                            activeFocusOnTab: true
                            Accessible.role: Accessible.Button
                            Accessible.name: qsTr("Save metadata to an XMP sidecar")
                            Accessible.onPressAction: sessionController.exportXmpSidecarsForSelected()
                            Keys.onPressed: (event) => {
                                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                    sessionController.exportXmpSidecarsForSelected()
                                    event.accepted = true
                                }
                            }

                            ToolTip.visible: exportXmpMouse.containsMouse
                            ToolTip.text: qsTr("Write metadata to companion .xmp sidecar file")

                            MouseArea {
                                id: exportXmpMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: sessionController.exportXmpSidecarsForSelected()
                            }

                            FocusRing { targetRadius: themeManager.radiusSm }
                        }

                        Rectangle {
                            width: (parent.width - 8) / 2
                            height: 28
                            radius: themeManager.radiusSm
                            color: importXmpMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
                            border.color: themeManager.border
                            border.width: 1

                            Row {
                                anchors.centerIn: parent
                                spacing: 4
                                Text { text: "🔄"; font.pixelSize: 10 }
                                Text { text: qsTr("Reload XMP"); font.pixelSize: 11; font.weight: Font.Medium; color: themeManager.textPrimary }
                            }

                            activeFocusOnTab: true
                            Accessible.role: Accessible.Button
                            Accessible.name: qsTr("Reload metadata from the XMP sidecar")
                            Accessible.onPressAction: sessionController.importXmpSidecarsForSelected()
                            Keys.onPressed: (event) => {
                                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                    sessionController.importXmpSidecarsForSelected()
                                    event.accepted = true
                                }
                            }

                            ToolTip.visible: importXmpMouse.containsMouse
                            ToolTip.text: qsTr("Reload metadata from companion .xmp sidecar file")

                            MouseArea {
                                id: importXmpMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: sessionController.importXmpSidecarsForSelected()
                            }

                            FocusRing { targetRadius: themeManager.radiusSm }
                        }
                    }

                    // Auto-sync toggle
                    Row {
                        spacing: 8
                        Rectangle {
                            width: 14
                            height: 14
                            radius: 3
                            anchors.verticalCenter: parent.verticalCenter
                            color: sessionController.xmpSyncEnabled ? themeManager.accent : themeManager.surfaceElevated
                            border.color: sessionController.xmpSyncEnabled ? themeManager.accent : themeManager.border
                            border.width: 1

                            activeFocusOnTab: true
                            Accessible.role: Accessible.CheckBox
                            Accessible.name: qsTr("Automatically sync edits with XMP sidecars")
                            Accessible.checkable: true
                            Accessible.checked: sessionController.xmpSyncEnabled
                            Accessible.onPressAction: sessionController.setXmpSyncEnabled(!sessionController.xmpSyncEnabled)
                            Keys.onPressed: (event) => {
                                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                    sessionController.setXmpSyncEnabled(!sessionController.xmpSyncEnabled)
                                    event.accepted = true
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: -3
                                radius: 4
                                color: "transparent"
                                border.color: themeManager.borderFocus
                                border.width: 1
                                visible: parent.activeFocus
                                Accessible.ignored: true
                            }

                            Text {
                                anchors.centerIn: parent
                                visible: sessionController.xmpSyncEnabled
                                text: "✓"
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                color: "#ffffff"
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: sessionController.setXmpSyncEnabled(!sessionController.xmpSyncEnabled)
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Auto-sync edits with .xmp sidecar")
                            font.pixelSize: 11
                            color: themeManager.textSecondary

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: sessionController.setXmpSyncEnabled(!sessionController.xmpSyncEnabled)
                            }
                        }
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

                    // COLOR LABEL PICKER
                    Row {
                        spacing: 8
                        anchors.left: parent.left

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Color:")
                            font.pixelSize: 11
                            color: themeManager.textSecondary
                        }

                        Row {
                            spacing: 6
                            anchors.verticalCenter: parent.verticalCenter

                            Repeater {
                                model: [
                                    { id: 0, label: qsTr("Clear Color"), color: "transparent", border: themeManager.textMuted },
                                    { id: 1, label: qsTr("Red (6)"), color: "#ef4444", border: "#dc2626" },
                                    { id: 2, label: qsTr("Yellow (7)"), color: "#eab308", border: "#ca8a04" },
                                    { id: 3, label: qsTr("Green (8)"), color: "#22c55e", border: "#16a34a" },
                                    { id: 4, label: qsTr("Blue (9)"), color: "#3b82f6", border: "#2563eb" },
                                    { id: 5, label: qsTr("Purple"), color: "#a855f7", border: "#9333ea" }
                                ]

                                Rectangle {
                                    width: 18
                                    height: 18
                                    radius: 9
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: modelData.color
                                    border.color: (root.item.colorLabel || 0) === modelData.id ? "#ffffff" : modelData.border
                                    border.width: (root.item.colorLabel || 0) === modelData.id ? 2 : 1

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 24
                                        height: 24
                                        radius: 12
                                        color: "transparent"
                                        border.color: themeManager.accent
                                        border.width: 1.5
                                        visible: (root.item.colorLabel || 0) === modelData.id
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        visible: modelData.id === 0
                                        text: "✕"
                                        font.pixelSize: 8
                                        color: themeManager.textMuted
                                    }

                                    activeFocusOnTab: true
                                    Accessible.role: Accessible.RadioButton
                                    Accessible.name: modelData.label
                                    Accessible.checkable: true
                                    Accessible.checked: (root.item.colorLabel || 0) === modelData.id
                                    Accessible.onPressAction: sessionController.setItemColorLabel(selectionManager.currentIndex, modelData.id)
                                    Keys.onPressed: (event) => {
                                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                            sessionController.setItemColorLabel(selectionManager.currentIndex, modelData.id)
                                            event.accepted = true
                                        }
                                    }

                                    ToolTip.visible: colMouse.containsMouse
                                    ToolTip.text: modelData.label

                                    MouseArea {
                                        id: colMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: sessionController.setItemColorLabel(selectionManager.currentIndex, modelData.id)
                                    }

                                    FocusRing { targetRadius: themeManager.radiusPill }
                                }
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
                                activeFocusOnTab: true
                                Accessible.role: Accessible.EditableText
                                Accessible.name: qsTr("New tag for this photo")
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

                                function addTag() {
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

                            activeFocusOnTab: true
                            Accessible.role: Accessible.Button
                            Accessible.name: qsTr("Add tag to this photo")
                            Accessible.onPressAction: tagInput.addTag()
                            Keys.onPressed: (event) => {
                                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                    tagInput.addTag()
                                    event.accepted = true
                                }
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

                            FocusRing { targetRadius: themeManager.radiusSm }
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

                // Quick Export Button
                Rectangle {
                    width: parent.width
                    height: 32
                    radius: themeManager.radiusSm
                    color: exportSingleMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
                    border.color: themeManager.border
                    border.width: 1

                    Row {
                        anchors.centerIn: parent
                        spacing: 6
                        Text { text: "📤"; font.pixelSize: 12 }
                        Text { text: qsTr("Export Photo... (Ctrl+E)"); font.pixelSize: 11; font.weight: Font.Medium; color: themeManager.textPrimary }
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Export this photo")
                    Accessible.onPressAction: sessionController.openExportDialog()
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            sessionController.openExportDialog()
                            event.accepted = true
                        }
                    }

                    MouseArea {
                        id: exportSingleMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.openExportDialog()
                    }

                    FocusRing { targetRadius: themeManager.radiusSm }
                }
            }
        }
    }
    }
}
