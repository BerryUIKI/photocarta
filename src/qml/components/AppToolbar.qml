// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    signal requestOpenFiles()
    signal requestScanFolder()

    // Keyboard entry point for the search field (Ctrl+F)
    function focusSearch() {
        searchInput.forceActiveFocus()
    }

    implicitHeight: 52
    color: themeManager.surface
    border.color: themeManager.border
    border.width: 1

    Row {
        id: leftGroup
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        // App Branding
        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Image {
                width: 28
                height: 28
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/assets/icons/app_icon.svg"
                fillMode: Image.PreserveAspectFit
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "PhotoCarta"
                font.pixelSize: themeManager.fontLg
                font.weight: Font.Bold
                color: themeManager.textPrimary
            }

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: prototypeText.implicitWidth + 10
                height: 18
                radius: 4
                color: themeManager.badgeDemoBg
                visible: root.width > 920

                Text {
                    id: prototypeText
                    anchors.centerIn: parent
                    text: qsTr("PROTOTYPE")
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: themeManager.badgeDemoText
                }
            }
        }

        Rectangle {
            width: 1
            height: 24
            anchors.verticalCenter: parent.verticalCenter
            color: themeManager.border
        }

        // Left Panel Toggle
        Rectangle {
            width: 32
            height: 32
            radius: themeManager.radiusMd
            anchors.verticalCenter: parent.verticalCenter
            color: sessionController.leftPaneVisible
                   ? (toggleLeftMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated)
                   : (toggleLeftMouse.containsMouse ? themeManager.surfaceHover : "transparent")
            border.color: sessionController.leftPaneVisible ? themeManager.accent : themeManager.border
            border.width: 1

            Image {
                anchors.centerIn: parent
                width: 16
                height: 16
                source: "qrc:/assets/icons/sidebar_toggle.svg"
                fillMode: Image.PreserveAspectFit
            }

            activeFocusOnTab: true
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Toggle navigation pane")
            Accessible.description: qsTr("Show or hide the collections sidebar (Ctrl+1)")
            Accessible.checkable: true
            Accessible.checked: sessionController.leftPaneVisible
            Accessible.onPressAction: sessionController.toggleLeftPane()
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    sessionController.toggleLeftPane()
                    event.accepted = true
                }
            }

            ToolTip.visible: toggleLeftMouse.containsMouse
            ToolTip.text: qsTr("Toggle Navigation Pane (Ctrl+1)")

            MouseArea {
                id: toggleLeftMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: sessionController.toggleLeftPane()
            }

            FocusRing { targetRadius: themeManager.radiusMd }
        }

        // Open Photos Button
        Rectangle {
            width: openRow.implicitWidth + 16
            height: 32
            radius: themeManager.radiusMd
            anchors.verticalCenter: parent.verticalCenter
            color: openMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
            border.color: themeManager.border
            border.width: 1

            Row {
                id: openRow
                anchors.centerIn: parent
                spacing: 6

                Image {
                    width: 16
                    height: 16
                    anchors.verticalCenter: parent.verticalCenter
                    source: "qrc:/assets/icons/folder_open.svg"
                    fillMode: Image.PreserveAspectFit
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Open Photos...")
                    font.pixelSize: themeManager.fontSm
                    font.weight: Font.Medium
                    color: themeManager.textPrimary
                    visible: root.width > 860
                }
            }

            activeFocusOnTab: true
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Open photos")
            Accessible.description: qsTr("Add local photos to a temporary session (Ctrl+O)")
            Accessible.onPressAction: root.requestOpenFiles()
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    root.requestOpenFiles()
                    event.accepted = true
                }
            }

            ToolTip.visible: openMouse.containsMouse
            ToolTip.text: qsTr("Add local photos to session (Ctrl+O)")

            MouseArea {
                id: openMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.requestOpenFiles()
            }

            FocusRing { targetRadius: themeManager.radiusMd }
        }

        // Scan Directory Button
        Rectangle {
            width: scanRow.implicitWidth + 16
            height: 32
            radius: themeManager.radiusMd
            anchors.verticalCenter: parent.verticalCenter
            color: sessionController.isScanning
                   ? themeManager.surfaceSelected
                   : (scanMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated)
            border.color: sessionController.isScanning ? themeManager.accent : themeManager.border
            border.width: 1

            Row {
                id: scanRow
                anchors.centerIn: parent
                spacing: 6

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: sessionController.isScanning ? "⏳" : "🔍"
                    font.pixelSize: 13
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: sessionController.isScanning ? qsTr("Scanning...") : qsTr("Scan Folder...")
                    font.pixelSize: themeManager.fontSm
                    font.weight: Font.Medium
                    color: themeManager.textPrimary
                    visible: root.width > 960
                }
            }

            activeFocusOnTab: true
            Accessible.role: Accessible.Button
            Accessible.name: sessionController.isScanning ? qsTr("Cancel scanning") : qsTr("Scan folder")
            Accessible.description: sessionController.isScanning
                                    ? qsTr("Stop the running directory scan")
                                    : qsTr("Scan a directory recursively into the catalog (Ctrl+Shift+O)")
            Accessible.onPressAction: {
                if (sessionController.isScanning) {
                    sessionController.cancelScan()
                } else {
                    root.requestScanFolder()
                }
            }
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    if (sessionController.isScanning) {
                        sessionController.cancelScan()
                    } else {
                        root.requestScanFolder()
                    }
                    event.accepted = true
                }
            }

            ToolTip.visible: scanMouse.containsMouse
            ToolTip.text: sessionController.isScanning
                          ? qsTr("Scanning in progress... Click to cancel")
                          : qsTr("Scan directory recursively and import into SQLite catalog (Ctrl+Shift+O)")

            MouseArea {
                id: scanMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (sessionController.isScanning) {
                        sessionController.cancelScan();
                    } else {
                        root.requestScanFolder();
                    }
                }
            }

            FocusRing { targetRadius: themeManager.radiusMd }
        }

        // Export Photos Button
        Rectangle {
            width: exportBtnRow.implicitWidth + 16
            height: 32
            radius: themeManager.radiusMd
            anchors.verticalCenter: parent.verticalCenter
            color: sessionController.isExporting
                   ? themeManager.surfaceSelected
                   : (exportToolbarMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated)
            border.color: sessionController.isExporting ? themeManager.accent : themeManager.border
            border.width: 1
            opacity: (selectionManager.hasSelection || selectionManager.selectedCount > 0) ? 1.0 : 0.5

            Row {
                id: exportBtnRow
                anchors.centerIn: parent
                spacing: 6

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "📤"
                    font.pixelSize: 13
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Export...")
                    font.pixelSize: themeManager.fontSm
                    font.weight: Font.Medium
                    color: themeManager.textPrimary
                    visible: root.width > 1060
                }
            }

            activeFocusOnTab: enabled
            enabled: (selectionManager.hasSelection || selectionManager.selectedCount > 0) && !sessionController.isExporting
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Export photos")
            Accessible.description: qsTr("Export the selected photos with format conversion and resizing (Ctrl+E)")
            Accessible.onPressAction: sessionController.openExportDialog()
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    sessionController.openExportDialog()
                    event.accepted = true
                }
            }

            ToolTip.visible: exportToolbarMouse.containsMouse
            ToolTip.text: qsTr("Export selected photos with format conversion and resizing (Ctrl+E)")

            MouseArea {
                id: exportToolbarMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                enabled: (selectionManager.hasSelection || selectionManager.selectedCount > 0) && !sessionController.isExporting
                onClicked: sessionController.openExportDialog()
            }

            FocusRing { targetRadius: themeManager.radiusMd }
        }

        // Batch Rename Photos Button
        Rectangle {
            width: renameBtnRow.implicitWidth + 16
            height: 32
            radius: themeManager.radiusMd
            anchors.verticalCenter: parent.verticalCenter
            color: renameToolbarMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
            border.color: themeManager.border
            border.width: 1
            opacity: (selectionManager.hasSelection || selectionManager.selectedCount > 0 || catalogModel.count > 0) ? 1.0 : 0.5

            Row {
                id: renameBtnRow
                anchors.centerIn: parent
                spacing: 6

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "🏷️"
                    font.pixelSize: 13
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Rename...")
                    font.pixelSize: themeManager.fontSm
                    font.weight: Font.Medium
                    color: themeManager.textPrimary
                    visible: root.width > 1160
                }
            }

            activeFocusOnTab: enabled
            enabled: (selectionManager.hasSelection || selectionManager.selectedCount > 0 || catalogModel.count > 0)
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Batch rename photos")
            Accessible.description: qsTr("Rename photos using custom templates (F2)")
            Accessible.onPressAction: sessionController.openBatchRenameDialog()
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    sessionController.openBatchRenameDialog()
                    event.accepted = true
                }
            }

            ToolTip.visible: renameToolbarMouse.containsMouse
            ToolTip.text: qsTr("Batch rename photos using custom templates (F2)")

            MouseArea {
                id: renameToolbarMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                enabled: (selectionManager.hasSelection || selectionManager.selectedCount > 0 || catalogModel.count > 0)
                onClicked: sessionController.openBatchRenameDialog()
            }

            FocusRing { targetRadius: themeManager.radiusMd }
        }

        // View Mode Switcher: Grid vs Loupe vs Compare
        Rectangle {
            width: 102
            height: 32
            radius: themeManager.radiusMd
            anchors.verticalCenter: parent.verticalCenter
            color: themeManager.surfaceElevated
            border.color: themeManager.border
            border.width: 1

            Row {
                anchors.fill: parent

                // Grid Button
                Rectangle {
                    width: 34
                    height: 32
                    radius: themeManager.radiusMd
                    color: (!sessionController.previewMode && !sessionController.compareMode) ? themeManager.accent : "transparent"

                    Image {
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        source: "qrc:/assets/icons/grid_view.svg"
                        fillMode: Image.PreserveAspectFit
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: qsTr("Grid view")
                    Accessible.checkable: true
                    Accessible.checked: !sessionController.previewMode && !sessionController.compareMode
                    Accessible.onPressAction: {
                        if (sessionController.compareMode) sessionController.setCompareMode(false)
                        if (sessionController.previewMode) sessionController.setPreviewMode(false)
                    }
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            if (sessionController.compareMode) sessionController.setCompareMode(false)
                            if (sessionController.previewMode) sessionController.setPreviewMode(false)
                            event.accepted = true
                        }
                    }

                    ToolTip.visible: gridBtnMouse.containsMouse
                    ToolTip.text: qsTr("Grid View (Esc)")

                    MouseArea {
                        id: gridBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (sessionController.compareMode) sessionController.setCompareMode(false)
                            if (sessionController.previewMode) sessionController.setPreviewMode(false)
                        }
                    }

                    FocusRing { targetRadius: themeManager.radiusMd }
                }

                // Loupe / Preview Button
                Rectangle {
                    width: 34
                    height: 32
                    radius: themeManager.radiusMd
                    color: (sessionController.previewMode && !sessionController.compareMode) ? themeManager.accent : "transparent"

                    Image {
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        source: "qrc:/assets/icons/loupe_view.svg"
                        fillMode: Image.PreserveAspectFit
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: qsTr("Preview view")
                    Accessible.checkable: true
                    Accessible.checked: sessionController.previewMode && !sessionController.compareMode
                    Accessible.onPressAction: {
                        if (sessionController.compareMode) sessionController.setCompareMode(false)
                        sessionController.setPreviewMode(true)
                    }
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            if (sessionController.compareMode) sessionController.setCompareMode(false)
                            sessionController.setPreviewMode(true)
                            event.accepted = true
                        }
                    }

                    ToolTip.visible: loupeBtnMouse.containsMouse
                    ToolTip.text: qsTr("Preview / Loupe View (Enter)")

                    MouseArea {
                        id: loupeBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (sessionController.compareMode) sessionController.setCompareMode(false)
                            sessionController.setPreviewMode(true)
                        }
                    }

                    FocusRing { targetRadius: themeManager.radiusMd }
                }

                // Side-by-Side Compare Button (Light Table)
                Rectangle {
                    width: 34
                    height: 32
                    radius: themeManager.radiusMd
                    color: sessionController.compareMode ? themeManager.accent : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "◫"
                        font.pixelSize: 15
                        font.weight: Font.Bold
                        color: sessionController.compareMode ? "#ffffff" : themeManager.textSecondary
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: qsTr("Compare view")
                    Accessible.checkable: true
                    Accessible.checked: sessionController.compareMode
                    Accessible.onPressAction: sessionController.toggleCompareMode()
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            sessionController.toggleCompareMode()
                            event.accepted = true
                        }
                    }

                    ToolTip.visible: compareBtnMouse.containsMouse
                    ToolTip.text: qsTr("Compare Selected Photos / Light Table (C)")

                    MouseArea {
                        id: compareBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.toggleCompareMode()
                    }

                    FocusRing { targetRadius: themeManager.radiusMd }
                }
            }
        }
    }

    // Center / Right Controls
    Row {
        id: rightGroup
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10

        // Search Field
        Rectangle {
            width: root.width > 1080 ? 220 : (root.width > 920 ? 160 : 130)
            height: 32
            radius: themeManager.radiusMd
            anchors.verticalCenter: parent.verticalCenter
            color: themeManager.surfaceElevated
            border.color: searchInput.activeFocus ? themeManager.borderFocus : themeManager.border
            border.width: 1

            Row {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 6
                spacing: 6

                Image {
                    width: 14
                    height: 14
                    anchors.verticalCenter: parent.verticalCenter
                    source: "qrc:/assets/icons/search.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.6
                }

                TextInput {
                    id: searchInput
                    width: parent.width - 44
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: themeManager.fontSm
                    color: themeManager.textPrimary
                    clip: true
                    selectByMouse: true
                    activeFocusOnTab: true
                    Accessible.role: Accessible.EditableText
                    Accessible.name: qsTr("Search photos and tags")
                    Accessible.description: qsTr("Filters the photo grid as you type")

                    Text {
                        anchors.fill: parent
                        visible: !searchInput.text && !searchInput.activeFocus
                        text: root.width > 1000 ? qsTr("Search photos, tags...") : qsTr("Search...")
                        font.pixelSize: themeManager.fontSm
                        color: themeManager.textMuted
                        elide: Text.ElideRight
                    }

                    onTextChanged: filterModel.searchQuery = text
                }

                Image {
                    width: 14
                    height: 14
                    anchors.verticalCenter: parent.verticalCenter
                    visible: searchInput.text.length > 0
                    source: "qrc:/assets/icons/clear.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: clearSearchMouse.containsMouse ? 1.0 : 0.6

                    MouseArea {
                        id: clearSearchMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            searchInput.text = ""
                            filterModel.searchQuery = ""
                        }
                    }
                }
            }
        }

        // Active Tag Filter Chip
        Rectangle {
            visible: filterModel.tagFilter.length > 0
            height: 28
            radius: 14
            anchors.verticalCenter: parent.verticalCenter
            color: themeManager.surfaceSelected
            border.color: themeManager.accent
            border.width: 1
            implicitWidth: tagFilterRow.implicitWidth + 16

            Row {
                id: tagFilterRow
                anchors.centerIn: parent
                spacing: 6

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "#" + filterModel.tagFilter
                    font.pixelSize: themeManager.fontXs
                    font.weight: Font.DemiBold
                    color: themeManager.accent
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Rectangle {
                    width: 14
                    height: 14
                    radius: 7
                    anchors.verticalCenter: parent.verticalCenter
                    color: activeTagMouse.containsMouse ? themeManager.danger : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 8
                        font.weight: Font.Bold
                        color: activeTagMouse.containsMouse ? "#ffffff" : themeManager.accent
                    }
                }
            }

            activeFocusOnTab: true
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Clear tag filter %1").arg(filterModel.tagFilter)
            Accessible.onPressAction: sessionController.clearTagFilter()
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    sessionController.clearTagFilter()
                    event.accepted = true
                }
            }

            ToolTip.visible: activeTagMouse.containsMouse
            ToolTip.text: qsTr("Clear tag filter '%1'").arg(filterModel.tagFilter)

            MouseArea {
                id: activeTagMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: sessionController.clearTagFilter()
            }

            FocusRing { targetRadius: themeManager.radiusPill }
        }

        // Rating Filter Selector
        ComboBox {
            id: ratingCombo
            width: root.width > 1020 ? 94 : 80
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            model: [
                { text: qsTr("Rating"), value: 0 },
                { text: "★ 5", value: 5 },
                { text: "★ 4+", value: 4 },
                { text: "★ 3+", value: 3 },
                { text: "★ 2+", value: 2 },
                { text: "★ 1+", value: 1 }
            ]
            textRole: "text"
            valueRole: "value"
            currentIndex: {
                for (var i = 0; i < model.length; ++i) {
                    if (model[i].value === filterModel.minRating) return i;
                }
                return 0;
            }
            font.pixelSize: themeManager.fontSm

            background: Rectangle {
                radius: themeManager.radiusMd
                color: filterModel.minRating > 0 ? themeManager.surfaceSelected : themeManager.surfaceElevated
                border.color: filterModel.minRating > 0 ? themeManager.accent : themeManager.border
                border.width: 1
            }

            contentItem: Text {
                leftPadding: 8
                text: ratingCombo.displayText
                font.pixelSize: themeManager.fontSm
                color: filterModel.minRating > 0 ? themeManager.accent : themeManager.textPrimary
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            onActivated: {
                sessionController.setRatingFilter(currentValue)
            }
        }

        // Color Label Filter Selector
        ComboBox {
            id: colorLabelCombo
            width: root.width > 1020 ? 94 : 80
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            model: [
                { text: qsTr("Color"), value: 0 },
                { text: "🔴 Red", value: 1 },
                { text: "🟡 Yellow", value: 2 },
                { text: "🟢 Green", value: 3 },
                { text: "🔵 Blue", value: 4 },
                { text: "🟣 Purple", value: 5 }
            ]
            textRole: "text"
            valueRole: "value"
            currentIndex: {
                for (var i = 0; i < model.length; ++i) {
                    if (model[i].value === filterModel.colorLabelFilter) return i;
                }
                return 0;
            }
            font.pixelSize: themeManager.fontSm

            background: Rectangle {
                radius: themeManager.radiusMd
                color: filterModel.colorLabelFilter > 0 ? themeManager.surfaceSelected : themeManager.surfaceElevated
                border.color: filterModel.colorLabelFilter > 0 ? themeManager.accent : themeManager.border
                border.width: 1
            }

            contentItem: Text {
                leftPadding: 8
                text: colorLabelCombo.displayText
                font.pixelSize: themeManager.fontSm
                color: filterModel.colorLabelFilter > 0 ? themeManager.accent : themeManager.textPrimary
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            onActivated: {
                sessionController.setColorLabelFilter(currentValue)
            }
        }

        // Reset All Filters Button
        Rectangle {
            visible: filterModel.tagFilter.length > 0 || filterModel.minRating > 0 || filterModel.colorLabelFilter > 0 || filterModel.categoryFilter !== "all" || searchInput.text.length > 0
            width: resetFiltersRow.implicitWidth + 14
            height: 28
            radius: themeManager.radiusSm
            anchors.verticalCenter: parent.verticalCenter
            color: resetFiltersMouse.containsMouse ? themeManager.surfaceHover : "transparent"
            border.color: themeManager.border
            border.width: 1

            Row {
                id: resetFiltersRow
                anchors.centerIn: parent
                spacing: 4

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "✕"
                    font.pixelSize: 10
                    color: themeManager.textSecondary
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Reset")
                    font.pixelSize: 11
                    color: themeManager.textSecondary
                }
            }

            activeFocusOnTab: true
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Reset all filters")
            Accessible.description: qsTr("Clear the search query, tag, rating, color label, and category filters")
            Accessible.onPressAction: {
                searchInput.text = ""
                sessionController.clearAllFilters()
            }
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    searchInput.text = ""
                    sessionController.clearAllFilters()
                    event.accepted = true
                }
            }

            ToolTip.visible: resetFiltersMouse.containsMouse
            ToolTip.text: qsTr("Clear all filters and search query")

            MouseArea {
                id: resetFiltersMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    searchInput.text = ""
                    sessionController.clearAllFilters()
                }
            }

            FocusRing { targetRadius: themeManager.radiusSm }
        }

        // Sort Criterion Selector
        ComboBox {
            id: sortCombo
            width: root.width > 920 ? 100 : 80
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            model: [
                { text: qsTr("Date"), value: "date" },
                { text: qsTr("Name"), value: "name" },
                { text: qsTr("Size"), value: "size" },
                { text: qsTr("Rating"), value: "rating" }
            ]
            textRole: "text"
            valueRole: "value"
            currentIndex: {
                for (var i = 0; i < model.length; ++i) {
                    if (model[i].value === filterModel.sortCriterion) return i;
                }
                return 0;
            }
            font.pixelSize: themeManager.fontSm

            background: Rectangle {
                radius: themeManager.radiusMd
                color: themeManager.surfaceElevated
                border.color: themeManager.border
                border.width: 1
            }

            contentItem: Text {
                leftPadding: 8
                text: sortCombo.displayText
                font.pixelSize: themeManager.fontSm
                color: themeManager.textPrimary
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            onActivated: {
                filterModel.sortCriterion = currentValue
            }
        }

        // Sort Order Toggle Button
        Rectangle {
            width: 32
            height: 32
            radius: themeManager.radiusMd
            anchors.verticalCenter: parent.verticalCenter
            color: sortOrderMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
            border.color: themeManager.border
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: filterModel.sortAscending ? "▲" : "▼"
                font.pixelSize: 11
                color: themeManager.textPrimary
            }

            activeFocusOnTab: true
            Accessible.role: Accessible.Button
            Accessible.name: filterModel.sortAscending ? qsTr("Sort ascending") : qsTr("Sort descending")
            Accessible.description: qsTr("Toggle the sort direction of the photo grid")
            Accessible.onPressAction: filterModel.sortAscending = !filterModel.sortAscending
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    filterModel.sortAscending = !filterModel.sortAscending
                    event.accepted = true
                }
            }

            ToolTip.visible: sortOrderMouse.containsMouse
            ToolTip.text: filterModel.sortAscending ? qsTr("Sort Ascending") : qsTr("Sort Descending")

            MouseArea {
                id: sortOrderMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: filterModel.sortAscending = !filterModel.sortAscending
            }

            FocusRing { targetRadius: themeManager.radiusMd }
        }

        // Right Panel Toggle
        Rectangle {
            width: 32
            height: 32
            radius: themeManager.radiusMd
            anchors.verticalCenter: parent.verticalCenter
            color: sessionController.rightPaneVisible
                   ? (toggleRightMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated)
                   : (toggleRightMouse.containsMouse ? themeManager.surfaceHover : "transparent")
            border.color: sessionController.rightPaneVisible ? themeManager.accent : themeManager.border
            border.width: 1

            Image {
                anchors.centerIn: parent
                width: 16
                height: 16
                source: "qrc:/assets/icons/details_toggle.svg"
                fillMode: Image.PreserveAspectFit
            }

            activeFocusOnTab: true
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Toggle details pane")
            Accessible.description: qsTr("Show or hide the metadata panel (Ctrl+2)")
            Accessible.checkable: true
            Accessible.checked: sessionController.rightPaneVisible
            Accessible.onPressAction: sessionController.toggleRightPane()
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    sessionController.toggleRightPane()
                    event.accepted = true
                }
            }

            ToolTip.visible: toggleRightMouse.containsMouse
            ToolTip.text: qsTr("Toggle Details Pane (Ctrl+2)")

            MouseArea {
                id: toggleRightMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: sessionController.toggleRightPane()
            }

            FocusRing { targetRadius: themeManager.radiusMd }
        }

        // Theme Toggle Button
        Rectangle {
            width: 32
            height: 32
            radius: themeManager.radiusMd
            anchors.verticalCenter: parent.verticalCenter
            color: themeBtnMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
            border.color: themeManager.border
            border.width: 1

            Image {
                anchors.centerIn: parent
                width: 16
                height: 16
                source: "qrc:/assets/icons/theme_toggle.svg"
                fillMode: Image.PreserveAspectFit
            }

            activeFocusOnTab: true
            Accessible.role: Accessible.Button
            Accessible.name: themeManager.isDark ? qsTr("Switch to light theme") : qsTr("Switch to dark theme")
            Accessible.description: qsTr("Toggle the application color theme (Ctrl+L)")
            Accessible.onPressAction: themeManager.toggleTheme()
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    themeManager.toggleTheme()
                    event.accepted = true
                }
            }

            ToolTip.visible: themeBtnMouse.containsMouse
            ToolTip.text: themeManager.isDark ? qsTr("Switch to Light Theme") : qsTr("Switch to Dark Theme")

            MouseArea {
                id: themeBtnMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: themeManager.toggleTheme()
            }

            FocusRing { targetRadius: themeManager.radiusMd }
        }
    }
}
