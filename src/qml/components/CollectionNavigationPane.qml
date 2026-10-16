// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

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

            // SECTION 2: POPULAR TAGS
            Text {
                text: qsTr("CATEGORIES & TAGS")
                font.pixelSize: themeManager.fontXs
                font.weight: Font.Bold
                color: themeManager.textMuted
                font.letterSpacing: 1.2
            }

            Flow {
                width: parent.width
                spacing: 6

                TagChip {
                    text: qsTr("Landscape")
                    active: filterModel.tagFilter === "Landscape"
                    onClicked: {
                        filterModel.tagFilter = (filterModel.tagFilter === "Landscape") ? "" : "Landscape"
                    }
                }

                TagChip {
                    text: qsTr("Portrait")
                    active: filterModel.tagFilter === "Portrait"
                    onClicked: {
                        filterModel.tagFilter = (filterModel.tagFilter === "Portrait") ? "" : "Portrait"
                    }
                }

                TagChip {
                    text: qsTr("Architecture")
                    active: filterModel.tagFilter === "Architecture"
                    onClicked: {
                        filterModel.tagFilter = (filterModel.tagFilter === "Architecture") ? "" : "Architecture"
                    }
                }

                TagChip {
                    text: qsTr("Street")
                    active: filterModel.tagFilter === "Street"
                    onClicked: {
                        filterModel.tagFilter = (filterModel.tagFilter === "Street") ? "" : "Street"
                    }
                }

                TagChip {
                    text: qsTr("Macro")
                    active: filterModel.tagFilter === "Macro"
                    onClicked: {
                        filterModel.tagFilter = (filterModel.tagFilter === "Macro") ? "" : "Macro"
                    }
                }

                TagChip {
                    text: qsTr("Astro")
                    active: filterModel.tagFilter === "Astro"
                    onClicked: {
                        filterModel.tagFilter = (filterModel.tagFilter === "Astro") ? "" : "Astro"
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
