// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    signal requestScanFolder()

    implicitWidth: 230
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
            implicitHeight: navContentCol.implicitHeight + 32

            Column {
                id: navContentCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                topPadding: 16
                bottomPadding: 16
                spacing: 16

            // SECTION 1: LIBRARY COLLECTIONS
            Text {
                text: qsTr("LIBRARY")
                font.pixelSize: themeManager.fontXs
                font.weight: Font.Bold
                color: themeManager.textMuted
                font.letterSpacing: 1.2
            }

            Column {
                width: parent.width
                spacing: 4

                // All Items Nav Row
                Rectangle {
                    width: parent.width
                    height: 34
                    radius: themeManager.radiusMd
                    color: (filterModel.categoryFilter === "all" && !filterModel.tagFilter)
                           ? themeManager.surfaceSelected
                           : (allNavMouse.containsMouse ? themeManager.surfaceHover : "transparent")

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "🖼️"
                            font.pixelSize: 13
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("All Photos")
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Medium
                            color: themeManager.textPrimary
                        }
                    }

                    Rectangle {
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: allCountText.implicitWidth + 12
                        height: 18
                        radius: 9
                        color: themeManager.surfaceElevated

                        Text {
                            id: allCountText
                            anchors.centerIn: parent
                            text: catalogModel.count
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            color: themeManager.textSecondary
                        }
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.ListItem
                    Accessible.name: qsTr("All Photos")
                    Accessible.selected: filterModel.categoryFilter === "all" && !filterModel.tagFilter
                    Accessible.onPressAction: {
                        filterModel.categoryFilter = "all"
                        filterModel.tagFilter = ""
                        filterModel.minRating = 0
                        sessionController.selectCollection(0)
                    }
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            filterModel.categoryFilter = "all"
                            filterModel.tagFilter = ""
                            filterModel.minRating = 0
                            sessionController.selectCollection(0)
                            event.accepted = true
                        }
                    }

                    MouseArea {
                        id: allNavMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            filterModel.categoryFilter = "all"
                            filterModel.tagFilter = ""
                            filterModel.minRating = 0
                            sessionController.selectCollection(0)
                        }
                    }

                    FocusRing { targetRadius: themeManager.radiusMd }
                }

                // Demo Fixtures Nav Row
                Rectangle {
                    width: parent.width
                    height: 34
                    radius: themeManager.radiusMd
                    color: (filterModel.categoryFilter === "demo")
                           ? themeManager.surfaceSelected
                           : (demoNavMouse.containsMouse ? themeManager.surfaceHover : "transparent")

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "✨"
                            font.pixelSize: 13
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Demo Fixtures")
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Medium
                            color: themeManager.textPrimary
                        }
                    }

                    Rectangle {
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: demoTagText.implicitWidth + 8
                        height: 16
                        radius: 4
                        color: themeManager.badgeDemoBg

                        Text {
                            id: demoTagText
                            anchors.centerIn: parent
                            text: qsTr("DEMO")
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            color: themeManager.badgeDemoText
                        }
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.ListItem
                    Accessible.name: qsTr("Demo Fixtures")
                    Accessible.selected: filterModel.categoryFilter === "demo"
                    Accessible.onPressAction: {
                        filterModel.categoryFilter = "demo"
                        filterModel.tagFilter = ""
                        filterModel.minRating = 0
                        sessionController.selectCollection(0)
                    }
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            filterModel.categoryFilter = "demo"
                            filterModel.tagFilter = ""
                            filterModel.minRating = 0
                            sessionController.selectCollection(0)
                            event.accepted = true
                        }
                    }

                    MouseArea {
                        id: demoNavMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            filterModel.categoryFilter = "demo"
                            filterModel.tagFilter = ""
                            filterModel.minRating = 0
                            sessionController.selectCollection(0)
                        }
                    }

                    FocusRing { targetRadius: themeManager.radiusMd }
                }

                // Imported Photos Nav Row
                Rectangle {
                    width: parent.width
                    height: 34
                    radius: themeManager.radiusMd
                    color: (filterModel.categoryFilter === "imported")
                           ? themeManager.surfaceSelected
                           : (importedNavMouse.containsMouse ? themeManager.surfaceHover : "transparent")

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "📁"
                            font.pixelSize: 13
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Imported Photos")
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Medium
                            color: themeManager.textPrimary
                        }
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.ListItem
                    Accessible.name: qsTr("Imported Photos")
                    Accessible.selected: filterModel.categoryFilter === "imported"
                    Accessible.onPressAction: {
                        filterModel.categoryFilter = "imported"
                        filterModel.tagFilter = ""
                        filterModel.minRating = 0
                    }
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            filterModel.categoryFilter = "imported"
                            filterModel.tagFilter = ""
                            filterModel.minRating = 0
                            event.accepted = true
                        }
                    }

                    MouseArea {
                        id: importedNavMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            filterModel.categoryFilter = "imported"
                            filterModel.tagFilter = ""
                            filterModel.minRating = 0
                        }
                    }

                    FocusRing { targetRadius: themeManager.radiusMd }
                }

                // Favorites Nav Row
                Rectangle {
                    width: parent.width
                    height: 34
                    radius: themeManager.radiusMd
                    color: (filterModel.categoryFilter === "favorites")
                           ? themeManager.surfaceSelected
                           : (favNavMouse.containsMouse ? themeManager.surfaceHover : "transparent")

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "⭐"
                            font.pixelSize: 13
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Favorites (4★+)")
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Medium
                            color: themeManager.textPrimary
                        }
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.ListItem
                    Accessible.name: qsTr("Favorites, four stars and above")
                    Accessible.selected: filterModel.categoryFilter === "favorites"
                    Accessible.onPressAction: {
                        filterModel.categoryFilter = "favorites"
                        filterModel.tagFilter = ""
                    }
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            filterModel.categoryFilter = "favorites"
                            filterModel.tagFilter = ""
                            event.accepted = true
                        }
                    }

                    MouseArea {
                        id: favNavMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            filterModel.categoryFilter = "favorites"
                            filterModel.tagFilter = ""
                        }
                    }

                    FocusRing { targetRadius: themeManager.radiusMd }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: themeManager.border
            }

            // SECTION 2: COLLECTIONS & ALBUMS
            Item {
                width: parent.width
                implicitHeight: 22

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("COLLECTIONS & ALBUMS")
                    font.pixelSize: themeManager.fontXs
                    font.weight: Font.Bold
                    color: themeManager.textMuted
                    font.letterSpacing: 1.2
                }

                // Add / Scan Folder action button
                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 22
                    height: 22
                    radius: 4
                    color: addAlbumMouse.containsMouse ? themeManager.surfaceHover : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "＋"
                        font.pixelSize: 13
                        font.weight: Font.Bold
                        color: addAlbumMouse.containsMouse ? themeManager.accent : themeManager.textSecondary
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Add or scan a photo folder")
                    Accessible.onPressAction: root.requestScanFolder()
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            root.requestScanFolder()
                            event.accepted = true
                        }
                    }

                    ToolTip.visible: addAlbumMouse.containsMouse
                    ToolTip.text: qsTr("Add / Scan Photo Folder...")
                    ToolTip.delay: 400

                    MouseArea {
                        id: addAlbumMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.requestScanFolder()
                    }

                    FocusRing { targetRadius: themeManager.radiusSm }
                }
            }

            // Empty state for collections
            Rectangle {
                visible: collectionModel.count === 0
                width: parent.width
                implicitHeight: emptyCol.implicitHeight + 16
                radius: themeManager.radiusMd
                color: themeManager.surfaceElevated
                border.color: themeManager.border
                border.width: 1

                Column {
                    id: emptyCol
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    Text {
                        text: qsTr("No Albums Registered")
                        font.pixelSize: themeManager.fontSm
                        font.weight: Font.DemiBold
                        color: themeManager.textPrimary
                    }

                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: qsTr("Scan a photo directory to organize your local or removable media collections.")
                        font.pixelSize: 10
                        lineHeight: 1.2
                        color: themeManager.textMuted
                    }

                    Rectangle {
                        width: parent.width
                        height: 26
                        radius: 4
                        color: scanPromptMouse.containsMouse ? themeManager.surfaceHover : themeManager.surface
                        border.color: themeManager.border
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Scan Directory...")
                            font.pixelSize: 11
                            font.weight: Font.Medium
                            color: themeManager.accent
                        }

                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Scan directory")
                        Accessible.onPressAction: root.requestScanFolder()
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                root.requestScanFolder()
                                event.accepted = true
                            }
                        }

                        MouseArea {
                            id: scanPromptMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.requestScanFolder()
                        }

                        FocusRing { targetRadius: themeManager.radiusSm }
                    }
                }
            }

            // Collection list
            Column {
                visible: collectionModel.count > 0
                width: parent.width
                spacing: 4

                // "All Albums" row to clear collection filter
                Rectangle {
                    width: parent.width
                    height: 32
                    radius: themeManager.radiusMd
                    color: (sessionController.currentCollectionId === 0 && filterModel.categoryFilter === "all")
                           ? themeManager.surfaceSelected
                           : (allCollMouse.containsMouse ? themeManager.surfaceHover : "transparent")

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "📚"
                            font.pixelSize: 12
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("All Albums")
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Medium
                            color: themeManager.textPrimary
                        }
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.ListItem
                    Accessible.name: qsTr("All Albums")
                    Accessible.selected: sessionController.currentCollectionId === 0 && filterModel.categoryFilter === "all"
                    Accessible.onPressAction: {
                        filterModel.categoryFilter = "all"
                        sessionController.selectCollection(0)
                    }
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            filterModel.categoryFilter = "all"
                            sessionController.selectCollection(0)
                            event.accepted = true
                        }
                    }

                    MouseArea {
                        id: allCollMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            filterModel.categoryFilter = "all"
                            sessionController.selectCollection(0)
                        }
                    }

                    FocusRing { targetRadius: themeManager.radiusMd }
                }

                // Individual Collection Rows
                Repeater {
                    model: collectionModel

                    delegate: Rectangle {
                        id: collRow
                        required property int id
                        required property string name
                        required property string rootPath
                        required property string collectionType
                        required property bool isAvailable
                        required property int itemCount
                        required property string lastScannedAt
                        required property bool isActive

                        width: parent.width
                        height: 34
                        radius: themeManager.radiusMd
                        color: isActive
                               ? themeManager.surfaceSelected
                               : (rowMouse.containsMouse ? themeManager.surfaceHover : "transparent")

                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            anchors.right: actionRow.left
                            anchors.rightMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 6

                            // Online/Offline Status Indicator Dot
                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 7
                                height: 7
                                radius: 3.5
                                color: collRow.isAvailable ? "#22c55e" : "#f59e0b"
                                ToolTip.visible: statusDotMouse.containsMouse
                                ToolTip.text: collRow.isAvailable ? qsTr("Online & Available") : qsTr("Offline (Preserved)")

                                MouseArea {
                                    id: statusDotMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                }
                            }

                            // Collection Icon: local vs removable vs network
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: collRow.collectionType === "removable" ? "💾" : (collRow.collectionType === "network" ? "🌐" : "📁")
                                font.pixelSize: 12
                                opacity: collRow.isAvailable ? 1.0 : 0.6
                            }

                            // Collection Name
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: collRow.name
                                font.pixelSize: themeManager.fontSm
                                font.weight: collRow.isActive ? Font.DemiBold : Font.Normal
                                color: collRow.isAvailable ? themeManager.textPrimary : themeManager.textMuted
                                elide: Text.ElideRight
                                width: Math.max(20, collRow.width - 110)
                            }
                        }

                        // Right side: Item count badge & hover action menu
                        Row {
                            id: actionRow
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 4

                            // Item count badge
                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: countText.implicitWidth + 10
                                height: 18
                                radius: 9
                                color: themeManager.surfaceElevated

                                Text {
                                    id: countText
                                    anchors.centerIn: parent
                                    text: collRow.itemCount
                                    font.pixelSize: 10
                                    font.weight: Font.DemiBold
                                    color: themeManager.textSecondary
                                }
                            }

                            // Actions Button (···)
                            Rectangle {
                                id: menuBtn
                                anchors.verticalCenter: parent.verticalCenter
                                width: 20
                                height: 20
                                radius: 4
                                visible: rowMouse.containsMouse || collMenu.visible || menuBtn.activeFocus
                                color: menuBtnMouse.containsMouse ? themeManager.surfaceElevated : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: "⋮"
                                    font.pixelSize: 13
                                    color: themeManager.textSecondary
                                }

                                activeFocusOnTab: true
                                Accessible.role: Accessible.Button
                                Accessible.name: qsTr("Album actions for %1").arg(collRow.name)
                                Accessible.onPressAction: collMenu.open()
                                Keys.onPressed: (event) => {
                                    if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                        collMenu.open()
                                        event.accepted = true
                                    }
                                }

                                MouseArea {
                                    id: menuBtnMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: collMenu.open()
                                }

                                FocusRing { targetRadius: themeManager.radiusSm }

                                Menu {
                                    id: collMenu
                                    y: parent.height

                                    MenuItem {
                                        text: qsTr("Rescan Album")
                                        onTriggered: sessionController.rescanCollection(collRow.id)
                                    }

                                    MenuItem {
                                        text: collRow.isAvailable ? qsTr("Mark as Offline") : qsTr("Mark as Online")
                                        onTriggered: sessionController.toggleCollectionAvailable(collRow.id)
                                    }

                                    MenuItem {
                                        text: qsTr("Reveal in Explorer")
                                        onTriggered: collectionModel.openInFileManager(collRow.id)
                                    }

                                    MenuSeparator {}

                                    MenuItem {
                                        text: qsTr("Remove from Catalog")
                                        onTriggered: sessionController.removeCollection(collRow.id)
                                    }
                                }
                            }
                        }

                        activeFocusOnTab: true
                        Accessible.role: Accessible.ListItem
                        Accessible.name: collRow.name
                        Accessible.description: collRow.rootPath + ", "
                                                + qsTr("%1 items").arg(collRow.itemCount) + ", "
                                                + (collRow.isAvailable ? qsTr("online") : qsTr("offline, preserved"))
                        Accessible.selected: collRow.isActive
                        Accessible.onPressAction: {
                            filterModel.categoryFilter = "all"
                            sessionController.selectCollection(collRow.id)
                        }
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                filterModel.categoryFilter = "all"
                                sessionController.selectCollection(collRow.id)
                                event.accepted = true
                            }
                        }

                        ToolTip.visible: rowMouse.containsMouse && !menuBtnMouse.containsMouse && !statusDotMouse.containsMouse
                        ToolTip.text: collRow.name + "\n" + collRow.rootPath + "\n" + qsTr("Items: ") + collRow.itemCount + (collRow.lastScannedAt ? ("\n" + qsTr("Last scan: ") + collRow.lastScannedAt) : "")
                        ToolTip.delay: 800

                        MouseArea {
                            id: rowMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: (mouse) => {
                                if (mouse.button === Qt.RightButton) {
                                    collMenu.open()
                                } else {
                                    filterModel.categoryFilter = "all"
                                    sessionController.selectCollection(collRow.id)
                                }
                            }
                        }

                        FocusRing { targetRadius: themeManager.radiusMd }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: themeManager.border
            }

            // SECTION 3: POPULAR TAGS & RATINGS
            Text {
                text: qsTr("RATING & TAG FILTERS")
                font.pixelSize: themeManager.fontXs
                font.weight: Font.Bold
                color: themeManager.textMuted
                font.letterSpacing: 1.2
            }

            // Rating Filter Row
            Row {
                width: parent.width
                spacing: 4

                Repeater {
                    model: [
                        { text: qsTr("All"), val: 0 },
                        { text: "5★", val: 5 },
                        { text: "4★+", val: 4 },
                        { text: "3★+", val: 3 },
                        { text: "2★+", val: 2 }
                    ]

                    delegate: Rectangle {
                        id: rChip
                        required property var modelData

                        width: Math.floor((parent.width - 16) / 5)
                        height: 24
                        radius: themeManager.radiusSm
                        color: filterModel.minRating === modelData.val
                               ? themeManager.surfaceSelected
                               : (rChipMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated)
                        border.color: filterModel.minRating === modelData.val
                                      ? themeManager.accent
                                      : themeManager.border
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: rChip.modelData.text
                            font.pixelSize: 10
                            font.weight: filterModel.minRating === rChip.modelData.val ? Font.Bold : Font.Normal
                            color: filterModel.minRating === rChip.modelData.val
                                   ? themeManager.accent
                                   : themeManager.textSecondary
                        }

                        activeFocusOnTab: true
                        Accessible.role: Accessible.RadioButton
                        Accessible.name: rChip.modelData.val === 0
                                        ? qsTr("All ratings")
                                        : qsTr("%1 stars and above").arg(rChip.modelData.val)
                        Accessible.checkable: true
                        Accessible.checked: filterModel.minRating === rChip.modelData.val
                        Accessible.onPressAction: {
                            if (filterModel.minRating === rChip.modelData.val && rChip.modelData.val > 0) {
                                sessionController.clearRatingFilter()
                            } else {
                                sessionController.setRatingFilter(rChip.modelData.val)
                            }
                        }
                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                if (filterModel.minRating === rChip.modelData.val && rChip.modelData.val > 0) {
                                    sessionController.clearRatingFilter()
                                } else {
                                    sessionController.setRatingFilter(rChip.modelData.val)
                                }
                                event.accepted = true
                            }
                        }

                        MouseArea {
                            id: rChipMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (filterModel.minRating === rChip.modelData.val && rChip.modelData.val > 0) {
                                    sessionController.clearRatingFilter()
                                } else {
                                    sessionController.setRatingFilter(rChip.modelData.val)
                                }
                            }
                        }

                        FocusRing { targetRadius: themeManager.radiusSm }
                    }
                }
            }

            Flow {
                width: parent.width
                spacing: 6

                Repeater {
                    model: catalogModel.availableTags.length > 0
                           ? catalogModel.availableTags
                           : ["Landscape", "Portrait", "Architecture", "Street", "Macro", "Astro"]

                    delegate: TagChip {
                        required property var modelData
                        text: modelData
                        active: filterModel.tagFilter.toLowerCase() === modelData.toLowerCase()
                        onClicked: {
                            if (filterModel.tagFilter.toLowerCase() === modelData.toLowerCase()) {
                                sessionController.clearTagFilter()
                            } else {
                                sessionController.setTagFilter(modelData)
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

            // SECTION 3: SESSION CONTROLS
            Text {
                text: qsTr("SESSION ACTIONS")
                font.pixelSize: themeManager.fontXs
                font.weight: Font.Bold
                color: themeManager.textMuted
                font.letterSpacing: 1.2
            }

            Column {
                width: parent.width
                spacing: 8

                // Load Demo Samples Button
                Rectangle {
                    width: parent.width
                    height: 32
                    radius: themeManager.radiusMd
                    color: loadDemoMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
                    border.color: themeManager.border
                    border.width: 1

                    Row {
                        anchors.centerIn: parent
                        spacing: 6

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "↺"
                            font.pixelSize: 13
                            color: themeManager.textPrimary
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Reset Demo Fixtures")
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Medium
                            color: themeManager.textPrimary
                        }
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Reset demo fixtures")
                    Accessible.onPressAction: sessionController.loadDemoSamples()
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            sessionController.loadDemoSamples()
                            event.accepted = true
                        }
                    }

                    MouseArea {
                        id: loadDemoMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.loadDemoSamples()
                    }

                    FocusRing { targetRadius: themeManager.radiusMd }
                }

                // Clear Session Button
                Rectangle {
                    width: parent.width
                    height: 32
                    radius: themeManager.radiusMd
                    color: clearMouse.containsMouse ? (themeManager.isDark ? "#3f1a1a" : "#fee2e2") : themeManager.surfaceElevated
                    border.color: clearMouse.containsMouse ? themeManager.danger : themeManager.border
                    border.width: 1

                    Row {
                        anchors.centerIn: parent
                        spacing: 6

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "✕"
                            font.pixelSize: 11
                            color: clearMouse.containsMouse ? themeManager.danger : themeManager.textSecondary
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Clear Session")
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Medium
                            color: clearMouse.containsMouse ? themeManager.danger : themeManager.textSecondary
                        }
                    }

                    activeFocusOnTab: true
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Clear session")
                    Accessible.description: qsTr("Remove photos from this temporary browsing session")
                    Accessible.onPressAction: sessionController.clearSession()
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            sessionController.clearSession()
                            event.accepted = true
                        }
                    }

                    MouseArea {
                        id: clearMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.clearSession()
                    }

                    FocusRing { targetRadius: themeManager.radiusMd }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: themeManager.border
            }

            // SECTION 4: SESSION STATUS NOTE
            Rectangle {
                width: parent.width
                implicitHeight: noticeCol.implicitHeight + 14
                radius: themeManager.radiusSm
                color: themeManager.surfaceElevated
                border.color: themeManager.border
                border.width: 1

                Column {
                    id: noticeCol
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    Text {
                        text: qsTr("Temporary Session")
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        color: themeManager.textSecondary
                    }

                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: qsTr("Safe, read-only inspection. Originals and production catalogs remain untouched.")
                        font.pixelSize: 9
                        lineHeight: 1.2
                        color: themeManager.textMuted
                    }
                }
            }
        }
    }
    }
}
