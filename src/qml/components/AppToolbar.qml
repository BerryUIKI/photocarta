// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    signal requestOpenFiles()

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

                Text {
                    id: prototypeText
                    anchors.centerIn: parent
                    text: qsTr("GUI SLICE")
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

            ToolTip.visible: toggleLeftMouse.containsMouse
            ToolTip.text: qsTr("Toggle Navigation Pane (Ctrl+1)")

            MouseArea {
                id: toggleLeftMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: sessionController.toggleLeftPane()
            }
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
                }
            }

            ToolTip.visible: openMouse.containsMouse
            ToolTip.text: qsTr("Add local photos to ephemeral session (Ctrl+O)")

            MouseArea {
                id: openMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.requestOpenFiles()
            }
        }

        // View Mode Switcher: Grid vs Loupe
        Rectangle {
            width: 68
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
                    color: !sessionController.previewMode ? themeManager.accent : "transparent"

                    Image {
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        source: "qrc:/assets/icons/grid_view.svg"
                        fillMode: Image.PreserveAspectFit
                    }

                    ToolTip.visible: gridBtnMouse.containsMouse
                    ToolTip.text: qsTr("Grid View")

                    MouseArea {
                        id: gridBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.setPreviewMode(false)
                    }
                }

                // Loupe Button
                Rectangle {
                    width: 34
                    height: 32
                    radius: themeManager.radiusMd
                    color: sessionController.previewMode ? themeManager.accent : "transparent"

                    Image {
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        source: "qrc:/assets/icons/loupe_view.svg"
                        fillMode: Image.PreserveAspectFit
                    }

                    ToolTip.visible: loupeBtnMouse.containsMouse
                    ToolTip.text: qsTr("Preview View (Enter)")

                    MouseArea {
                        id: loupeBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.setPreviewMode(true)
                    }
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
            width: 220
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

                    Text {
                        anchors.fill: parent
                        visible: !searchInput.text && !searchInput.activeFocus
                        text: qsTr("Search photos, tags, cameras...")
                        font.pixelSize: themeManager.fontSm
                        color: themeManager.textMuted
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

        // Sort Criterion Selector
        ComboBox {
            id: sortCombo
            width: 110
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

            ToolTip.visible: sortOrderMouse.containsMouse
            ToolTip.text: filterModel.sortAscending ? qsTr("Sort Ascending") : qsTr("Sort Descending")

            MouseArea {
                id: sortOrderMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: filterModel.sortAscending = !filterModel.sortAscending
            }
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

            ToolTip.visible: toggleRightMouse.containsMouse
            ToolTip.text: qsTr("Toggle Details Pane (Ctrl+2)")

            MouseArea {
                id: toggleRightMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: sessionController.toggleRightPane()
            }
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

            ToolTip.visible: themeBtnMouse.containsMouse
            ToolTip.text: themeManager.isDark ? qsTr("Switch to Light Theme") : qsTr("Switch to Dark Theme")

            MouseArea {
                id: themeBtnMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: themeManager.toggleTheme()
            }
        }
    }
}
