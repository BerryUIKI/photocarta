// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: root

    visible: sessionController.exportDialogOpen
    anchors.fill: parent
    color: Qt.rgba(0, 0, 0, 0.65)
    z: 200

    // Prevent clicks from penetrating to the views below
    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (!sessionController.isExporting) {
                sessionController.closeExportDialog()
            }
        }
    }

    // Modal Card
    Rectangle {
        id: dialogCard
        width: Math.min(540, parent.width - 32)
        implicitHeight: contentColumn.implicitHeight + 40
        anchors.centerIn: parent
        radius: themeManager.radiusLg
        color: themeManager.surfaceElevated
        border.color: themeManager.border
        border.width: 1

        // Consume mouse clicks inside card
        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }

        Column {
            id: contentColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 24
            anchors.top: parent.top
            anchors.topMargin: 20
            spacing: 16

            // Header Row
            Item {
                width: parent.width
                implicitHeight: 32

                Row {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Export Photos")
                        font.pixelSize: themeManager.fontLg
                        font.weight: Font.Bold
                        color: themeManager.textPrimary
                    }

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: countBadgeText.implicitWidth + 12
                        height: 20
                        radius: 10
                        color: themeManager.accent

                        Text {
                            id: countBadgeText
                            anchors.centerIn: parent
                            text: {
                                const count = selectionManager.selectedCount > 0 ? selectionManager.selectedCount : (selectionManager.hasSelection ? 1 : 0)
                                return qsTr("%1 selected").arg(count)
                            }
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: "#ffffff"
                        }
                    }
                }

                // Close Button
                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 28
                    height: 28
                    radius: 14
                    color: closeMouse.containsMouse ? themeManager.surfaceHover : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 12
                        color: themeManager.textSecondary
                    }

                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        enabled: !sessionController.isExporting
                        onClicked: sessionController.closeExportDialog()
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: themeManager.border
            }

            // Section 1: Output Destination
            Column {
                width: parent.width
                spacing: 6

                Text {
                    text: qsTr("Destination Folder")
                    font.pixelSize: themeManager.fontSm
                    font.weight: Font.DemiBold
                    color: themeManager.textSecondary
                }

                Row {
                    width: parent.width
                    spacing: 8

                    Rectangle {
                        width: parent.width - 92
                        height: 34
                        radius: themeManager.radiusMd
                        color: themeManager.surface
                        border.color: themeManager.border
                        border.width: 1

                        TextInput {
                            id: destPathInput
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            verticalAlignment: TextInput.AlignVCenter
                            font.pixelSize: themeManager.fontSm
                            color: themeManager.textPrimary
                            selectByMouse: true
                            text: sessionController.defaultExportDirectory() + "/PhotoCarta_Export"
                        }
                    }

                    Rectangle {
                        width: 84
                        height: 34
                        radius: themeManager.radiusMd
                        color: browseMouse.containsMouse ? themeManager.surfaceHover : themeManager.surface
                        border.color: themeManager.border
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Browse...")
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Medium
                            color: themeManager.accent
                        }

                        MouseArea {
                            id: browseMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            enabled: !sessionController.isExporting
                            onClicked: exportFolderDialog.open()
                        }
                    }
                }
            }

            // Section 2: Format & Quality
            Row {
                width: parent.width
                spacing: 16

                Column {
                    width: (parent.width - 16) / 2
                    spacing: 6

                    Text {
                        text: qsTr("Image Format")
                        font.pixelSize: themeManager.fontSm
                        font.weight: Font.DemiBold
                        color: themeManager.textSecondary
                    }

                    ComboBox {
                        id: formatCombo
                        width: parent.width
                        height: 34
                        model: [
                            { text: qsTr("Original Format"), value: "original" },
                            { text: qsTr("JPEG (.jpg)"), value: "jpg" },
                            { text: qsTr("PNG (.png)"), value: "png" },
                            { text: qsTr("WebP (.webp)"), value: "webp" }
                        ]
                        textRole: "text"
                        valueRole: "value"
                        font.pixelSize: themeManager.fontSm
                        enabled: !sessionController.isExporting

                        background: Rectangle {
                            radius: themeManager.radiusMd
                            color: themeManager.surface
                            border.color: themeManager.border
                            border.width: 1
                        }

                        contentItem: Text {
                            leftPadding: 10
                            text: formatCombo.displayText
                            font.pixelSize: themeManager.fontSm
                            color: themeManager.textPrimary
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }
                }

                Column {
                    width: (parent.width - 16) / 2
                    spacing: 6
                    opacity: (formatCombo.currentValue === "jpg" || formatCombo.currentValue === "webp") ? 1.0 : 0.4

                    Row {
                        width: parent.width
                        Text {
                            text: qsTr("Quality")
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.DemiBold
                            color: themeManager.textSecondary
                        }
                        Item { Layout.fillWidth: true; width: 10 }
                        Text {
                            anchors.right: parent.right
                            text: Math.round(qualitySlider.value) + "%"
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Bold
                            color: themeManager.accent
                        }
                    }

                    Slider {
                        id: qualitySlider
                        width: parent.width
                        height: 34
                        from: 20
                        to: 100
                        value: 90
                        stepSize: 1
                        enabled: !sessionController.isExporting && (formatCombo.currentValue === "jpg" || formatCombo.currentValue === "webp")
                    }
                }
            }

            // Section 3: Resolution & Scaling
            Column {
                width: parent.width
                spacing: 6

                Text {
                    text: qsTr("Resolution / Scaling")
                    font.pixelSize: themeManager.fontSm
                    font.weight: Font.DemiBold
                    color: themeManager.textSecondary
                }

                ComboBox {
                    id: scaleCombo
                    width: parent.width
                    height: 34
                    model: [
                        { text: qsTr("Original Size (No downscaling)"), value: 0 },
                        { text: qsTr("4K Ultra HD (3840 px max)"), value: 3840 },
                        { text: qsTr("Full HD (1920 px max)"), value: 1920 },
                        { text: qsTr("Web / Social (1200 px max)"), value: 1200 },
                        { text: qsTr("Thumbnail (600 px max)"), value: 600 }
                    ]
                    textRole: "text"
                    valueRole: "value"
                    font.pixelSize: themeManager.fontSm
                    enabled: !sessionController.isExporting

                    background: Rectangle {
                        radius: themeManager.radiusMd
                        color: themeManager.surface
                        border.color: themeManager.border
                        border.width: 1
                    }

                    contentItem: Text {
                        leftPadding: 10
                        text: scaleCombo.displayText
                        font.pixelSize: themeManager.fontSm
                        color: themeManager.textPrimary
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
            }

            // Section 4: Filename Prefix & Suffix
            Row {
                width: parent.width
                spacing: 16

                Column {
                    width: (parent.width - 16) / 2
                    spacing: 6

                    Text {
                        text: qsTr("Filename Prefix")
                        font.pixelSize: themeManager.fontSm
                        font.weight: Font.DemiBold
                        color: themeManager.textSecondary
                    }

                    Rectangle {
                        width: parent.width
                        height: 32
                        radius: themeManager.radiusMd
                        color: themeManager.surface
                        border.color: themeManager.border
                        border.width: 1

                        TextInput {
                            id: prefixInput
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            verticalAlignment: TextInput.AlignVCenter
                            font.pixelSize: themeManager.fontSm
                            color: themeManager.textPrimary
                            selectByMouse: true
                            enabled: !sessionController.isExporting
                        }
                    }
                }

                Column {
                    width: (parent.width - 16) / 2
                    spacing: 6

                    Text {
                        text: qsTr("Filename Suffix")
                        font.pixelSize: themeManager.fontSm
                        font.weight: Font.DemiBold
                        color: themeManager.textSecondary
                    }

                    Rectangle {
                        width: parent.width
                        height: 32
                        radius: themeManager.radiusMd
                        color: themeManager.surface
                        border.color: themeManager.border
                        border.width: 1

                        TextInput {
                            id: suffixInput
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            verticalAlignment: TextInput.AlignVCenter
                            font.pixelSize: themeManager.fontSm
                            color: themeManager.textPrimary
                            selectByMouse: true
                            enabled: !sessionController.isExporting
                        }
                    }
                }
            }

            // Section 5: Options Checkboxes
            Row {
                width: parent.width
                spacing: 24

                CheckBox {
                    id: overwriteCheck
                    text: qsTr("Overwrite existing files")
                    font.pixelSize: themeManager.fontSm
                    enabled: !sessionController.isExporting
                }

                CheckBox {
                    id: metadataCheck
                    text: qsTr("Preserve EXIF metadata")
                    font.pixelSize: themeManager.fontSm
                    checked: true
                    enabled: !sessionController.isExporting
                }
            }

            // Progress Bar (when active)
            Column {
                visible: sessionController.isExporting
                width: parent.width
                spacing: 6

                Row {
                    width: parent.width
                    Text {
                        text: qsTr("Exporting photos...")
                        font.pixelSize: themeManager.fontSm
                        font.weight: Font.DemiBold
                        color: themeManager.textPrimary
                    }
                    Item { Layout.fillWidth: true; width: 10 }
                    Text {
                        anchors.right: parent.right
                        text: qsTr("%1 of %2").arg(sessionController.exportProgress).arg(sessionController.exportTotal)
                        font.pixelSize: themeManager.fontSm
                        font.weight: Font.DemiBold
                        color: themeManager.accent
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 8
                    radius: 4
                    color: themeManager.surface
                    border.color: themeManager.border
                    border.width: 1

                    Rectangle {
                        height: parent.height
                        radius: 4
                        color: themeManager.accent
                        width: sessionController.exportTotal > 0
                               ? Math.max(8, parent.width * (sessionController.exportProgress / sessionController.exportTotal))
                               : 0
                    }
                }
            }

            // Footer Buttons
            Item {
                width: parent.width
                implicitHeight: 38

                Row {
                    anchors.right: parent.right
                    spacing: 12

                    // Cancel / Stop Button
                    Rectangle {
                        width: cancelBtnText.implicitWidth + 24
                        height: 36
                        radius: themeManager.radiusMd
                        color: cancelMouse.containsMouse ? themeManager.surfaceHover : themeManager.surface
                        border.color: themeManager.border
                        border.width: 1

                        Text {
                            id: cancelBtnText
                            anchors.centerIn: parent
                            text: sessionController.isExporting ? qsTr("Cancel Export") : qsTr("Cancel")
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Medium
                            color: themeManager.textPrimary
                        }

                        MouseArea {
                            id: cancelMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (sessionController.isExporting) {
                                    sessionController.cancelExport()
                                } else {
                                    sessionController.closeExportDialog()
                                }
                            }
                        }
                    }

                    // Export Action Button
                    Rectangle {
                        width: exportBtnText.implicitWidth + 28
                        height: 36
                        radius: themeManager.radiusMd
                        color: sessionController.isExporting
                               ? themeManager.surfaceHover
                               : (exportMouse.containsMouse ? Qt.darker(themeManager.accent, 1.1) : themeManager.accent)
                        opacity: (selectionManager.selectedCount > 0 || selectionManager.hasSelection) && !sessionController.isExporting ? 1.0 : 0.6

                        Text {
                            id: exportBtnText
                            anchors.centerIn: parent
                            text: qsTr("Export Now")
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Bold
                            color: "#ffffff"
                        }

                        MouseArea {
                            id: exportMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            enabled: (selectionManager.selectedCount > 0 || selectionManager.hasSelection) && !sessionController.isExporting
                            onClicked: {
                                sessionController.startExportSelected({
                                    outputDirectory: destPathInput.text.trim(),
                                    format: formatCombo.currentValue,
                                    quality: Math.round(qualitySlider.value),
                                    maxDimension: scaleCombo.currentValue,
                                    preserveMetadata: metadataCheck.checked,
                                    filenamePrefix: prefixInput.text,
                                    filenameSuffix: suffixInput.text,
                                    overwriteExisting: overwriteCheck.checked
                                })
                            }
                        }
                    }
                }
            }
        }
    }

    // Native Folder Picker
    FolderDialog {
        id: exportFolderDialog
        title: qsTr("Select Export Destination Directory")
        currentFolder: sessionController.defaultExportDirectory()
        onAccepted: {
            destPathInput.text = selectedFolder.toString()
        }
    }
}
