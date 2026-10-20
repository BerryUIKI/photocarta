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

                        MouseArea {
                            id: scanPromptMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.requestScanFolder()
                        }
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
                                anchors.verticalCenter: parent.verticalCenter
                                width: 20
                                height: 20
                                radius: 4
                                visible: rowMouse.containsMouse || collMenu.visible
                                color: menuBtnMouse.containsMouse ? themeManager.surfaceElevated : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: "⋮"
                                    font.pixelSize: 13
                                    color: themeManager.textSecondary
                                }

                                MouseArea {
                                    id: menuBtnMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: collMenu.open()
                                }

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
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: themeManager.border
            }

            // SECTION 3: MINIMUM RATING
            Text {
                text: qsTr("MINIMUM RATING")
                font.pixelSize: themeManager.fontXs
                font.weight: Font.Bold
                color: themeManager.textMuted
                font.letterSpacing: 1.2
            }

            Row {
                spacing: 8

                RatingStars {
                    anchors.verticalCenter: parent.verticalCenter
                    rating: filterModel.minRating
                    interactive: true
                    onRatingSelected: (newRating) => {
                        filterModel.minRating = newRating
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: filterModel.minRating > 0
                    text: qsTr("%1+").arg(filterModel.minRating)
                    font.pixelSize: themeManager.fontXs
                    font.weight: Font.DemiBold
                    color: themeManager.accent
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: themeManager.border
            }

            // SECTION 4: TAGS
            Text {
                text: qsTr("TAGS")
                font.pixelSize: themeManager.fontXs
                font.weight: Font.Bold
                color: themeManager.textMuted
                font.letterSpacing: 1.2
            }

            Text {
                width: parent.width
                visible: catalogModel.availableTags.length === 0
                text: qsTr("No tags in this catalog yet")
                font.pixelSize: 11
                color: themeManager.textMuted
                wrapMode: Text.Wrap
            }

            Flow {
                width: parent.width
                spacing: 6

                Repeater {
                    model: catalogModel.availableTags
                    TagChip {
                        text: modelData
                        active: filterModel.tagFilter === modelData
                        onClicked: {
                            filterModel.tagFilter = (filterModel.tagFilter === modelData) ? "" : modelData
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: themeManager.border
            }

            // SECTION 5: SESSION CONTROLS
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

                    MouseArea {
                        id: loadDemoMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.loadDemoSamples()
                    }
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

                    MouseArea {
                        id: clearMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.clearSession()
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: themeManager.border
            }

            // SECTION 6: SESSION STATUS NOTE
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
