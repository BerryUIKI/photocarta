// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    visible: sessionController.trashDialogOpen
    anchors.fill: parent
    color: Qt.rgba(0, 0, 0, 0.65)
    z: 220

    // Dismiss on background click
    MouseArea {
        anchors.fill: parent
        onClicked: sessionController.closeTrashDialog()
    }

    // Modal Dialog Card
    Rectangle {
        id: dialogCard
        width: Math.min(480, parent.width - 32)
        height: contentCol.implicitHeight + 40
        anchors.centerIn: parent
        radius: themeManager.radiusLg
        color: themeManager.surfaceElevated
        border.color: themeManager.border
        border.width: 1

        // Consume click inside card
        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }

        Column {
            id: contentCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 20
            spacing: 14

            // Header
            Text {
                width: parent.width
                text: qsTr("Move to Trash")
                font.pixelSize: themeManager.fontLg
                font.weight: Font.Bold
                color: themeManager.textPrimary
            }

            // Main message
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("Move %1 photo(s) to the trash? The catalog entries will be removed. You can restore the files from the system trash.")
                      .arg(sessionController.trashCandidateCount)
                font.pixelSize: themeManager.fontSm
                color: themeManager.textPrimary
            }

            // Skipped items note (offline / demo / missing files are never removed)
            Rectangle {
                width: parent.width
                height: skippedText.implicitHeight + 16
                radius: themeManager.radiusMd
                color: themeManager.isDark ? "#1e293b" : "#f1f5f9"
                border.color: themeManager.border
                border.width: 1
                visible: sessionController.trashSkippedCount > 0

                Text {
                    id: skippedText
                    anchors.fill: parent
                    anchors.margins: 8
                    wrapMode: Text.WordWrap
                    text: qsTr("%1 item(s) will be skipped: demo photos, offline collections, and missing files are never removed.")
                          .arg(sessionController.trashSkippedCount)
                    font.pixelSize: 11
                    color: themeManager.textMuted
                }
            }

            // Warning: trash may be unavailable on network/removable locations
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("Files on network or removable media may not have a trash; those files will be left in place and reported instead of being permanently deleted.")
                font.pixelSize: 11
                color: themeManager.textMuted
            }

            // Bottom Actions Bar
            Item {
                width: parent.width
                implicitHeight: 36

                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 10

                    // Cancel Button
                    Rectangle {
                        width: 90
                        height: 32
                        radius: themeManager.radiusMd
                        color: cancelBtnMouse.containsMouse ? themeManager.surfaceHover : themeManager.surfaceElevated
                        border.color: themeManager.border
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Cancel")
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Medium
                            color: themeManager.textPrimary
                        }

                        MouseArea {
                            id: cancelBtnMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sessionController.closeTrashDialog()
                        }
                    }

                    // Confirm Move to Trash Button (destructive styling)
                    Rectangle {
                        width: confirmBtnText.implicitWidth + 24
                        height: 32
                        radius: themeManager.radiusMd
                        color: sessionController.trashCandidateCount > 0
                               ? (confirmBtnMouse.containsMouse ? "#b91c1c" : "#dc2626")
                               : themeManager.surfaceElevated
                        opacity: sessionController.trashCandidateCount > 0 ? 1.0 : 0.5
                        border.color: "#dc2626"
                        border.width: 1

                        Text {
                            id: confirmBtnText
                            anchors.centerIn: parent
                            text: qsTr("Move %1 to Trash").arg(sessionController.trashCandidateCount)
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Bold
                            color: sessionController.trashCandidateCount > 0 ? "#ffffff" : themeManager.textMuted
                        }

                        MouseArea {
                            id: confirmBtnMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            enabled: sessionController.trashCandidateCount > 0
                            onClicked: sessionController.confirmTrashSelected()
                        }
                    }
                }
            }
        }
    }
}
