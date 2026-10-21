// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Item {
    id: root

    property bool isSearchEmpty: catalogModel.count > 0 && filterModel.count === 0
    signal requestOpenFiles()

    anchors.fill: parent

    Column {
        anchors.centerIn: parent
        spacing: 16
        width: Math.min(parent.width - 40, 420)

        // Illustration Circle
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 72
            height: 72
            radius: 36
            color: themeManager.surfaceElevated
            border.color: themeManager.border
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: root.isSearchEmpty ? "🔍" : "📸"
                font.pixelSize: 32
            }
        }

        // Title
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.isSearchEmpty
                  ? qsTr("No photos match your filter")
                  : qsTr("No photos in session")
            font.pixelSize: themeManager.fontLg
            font.weight: Font.Bold
            color: themeManager.textPrimary
            horizontalAlignment: Text.AlignHCenter
        }

        // Description
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            wrapMode: Text.Wrap
            text: root.isSearchEmpty
                  ? qsTr("Try clearing your search query or relaxing active category and rating filters.")
                  : qsTr("PhotoCarta is in prototype mode. Load procedural demo fixtures or select standard local images (JPG, PNG, WebP) to browse non-destructively.")
            font.pixelSize: themeManager.fontSm
            lineHeight: 1.4
            color: themeManager.textMuted
            horizontalAlignment: Text.AlignHCenter
        }

        Item { width: 1; height: 8 }

        // Action Buttons Row
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 12

            // If search empty, offer reset filters button
            Rectangle {
                activeFocusOnTab: root.isSearchEmpty
                visible: root.isSearchEmpty
                width: resetFiltersText.implicitWidth + 24
                height: 36
                radius: themeManager.radiusMd
                color: themeManager.accent

                Text {
                    id: resetFiltersText
                    anchors.centerIn: parent
                    text: qsTr("Clear All Filters")
                    font.pixelSize: themeManager.fontSm
                    font.weight: Font.Medium
                    color: themeManager.accentText
                }

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Clear all filters")
                Accessible.onPressAction: {
                    filterModel.searchQuery = ""
                    filterModel.categoryFilter = "all"
                    filterModel.tagFilter = ""
                    filterModel.minRating = 0
                }
                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        filterModel.searchQuery = ""
                        filterModel.categoryFilter = "all"
                        filterModel.tagFilter = ""
                        filterModel.minRating = 0
                        event.accepted = true
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        filterModel.searchQuery = ""
                        filterModel.categoryFilter = "all"
                        filterModel.tagFilter = ""
                        filterModel.minRating = 0
                    }
                }

                FocusRing { targetRadius: themeManager.radiusMd }
            }

            // If library empty: Load Demo Samples
            Rectangle {
                visible: !root.isSearchEmpty
                activeFocusOnTab: !root.isSearchEmpty
                width: loadDemoText.implicitWidth + 24
                height: 36
                radius: themeManager.radiusMd
                color: themeManager.accent

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Load demo samples")
                Accessible.onPressAction: sessionController.loadDemoSamples()
                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        sessionController.loadDemoSamples()
                        event.accepted = true
                    }
                }

                Text {
                    id: loadDemoText
                    anchors.centerIn: parent
                    text: qsTr("Load Demo Samples")
                    font.pixelSize: themeManager.fontSm
                    font.weight: Font.Medium
                    color: themeManager.accentText
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.loadDemoSamples()
                }

                FocusRing { targetRadius: themeManager.radiusMd }
            }

            // If library empty: Open Photos
            Rectangle {
                visible: !root.isSearchEmpty
                activeFocusOnTab: !root.isSearchEmpty
                width: openPhotosText.implicitWidth + 24
                height: 36
                radius: themeManager.radiusMd
                color: themeManager.surfaceElevated
                border.color: themeManager.border
                border.width: 1

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Open local photos")
                Accessible.onPressAction: root.requestOpenFiles()
                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        root.requestOpenFiles()
                        event.accepted = true
                    }
                }

                Text {
                    id: openPhotosText
                    anchors.centerIn: parent
                    text: qsTr("Open Local Photos...")
                    font.pixelSize: themeManager.fontSm
                    font.weight: Font.Medium
                    color: themeManager.textPrimary
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestOpenFiles()
                }

                FocusRing { targetRadius: themeManager.radiusMd }
            }
        }
    }
}
