// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    visible: sessionController.batchRenameDialogOpen
    anchors.fill: parent
    color: Qt.rgba(0, 0, 0, 0.65)
    z: 210

    // Internal state
    property var previewCandidates: []
    property int okCount: 0
    property int conflictCount: 0
    property bool isExecuting: false

    function updatePreview() {
        if (!visible) return;
        var rule = {
            pattern: patternInput.text,
            startSequence: parseInt(startSeqInput.text) || 1,
            sequenceStep: parseInt(stepInput.text) || 1,
            findText: findInput.text,
            replaceText: replaceInput.text,
            matchCase: matchCaseCheck.checked,
            caseConversion: caseCombo.currentIndex
        };
        var list = sessionController.previewBatchRename(rule);
        previewCandidates = list;

        var ok = 0;
        var conflicts = 0;
        for (var i = 0; i < list.length; ++i) {
            if (list[i].canRename) {
                ok++;
            } else {
                conflicts++;
            }
        }
        okCount = ok;
        conflictCount = conflicts;
    }

    onVisibleChanged: {
        if (visible) {
            updatePreview();
        }
    }

    // Dismiss on background click
    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (!root.isExecuting) {
                sessionController.closeBatchRenameDialog();
            }
        }
    }

    // Modal Dialog Card
    Rectangle {
        id: dialogCard
        width: Math.min(720, parent.width - 32)
        height: Math.min(620, parent.height - 40)
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
            id: mainCol
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

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
                        text: qsTr("Batch Rename Photos")
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
                                var count = selectionManager.selectedCount > 0 ? selectionManager.selectedCount : (selectionManager.hasSelection ? 1 : catalogModel.count);
                                return qsTr("%1 photos").arg(count);
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
                        onClicked: {
                            if (!root.isExecuting) {
                                sessionController.closeBatchRenameDialog();
                            }
                        }
                    }
                }
            }

            // Separator
            Rectangle {
                width: parent.width
                height: 1
                color: themeManager.border
            }

            // Pattern Template Row
            Column {
                width: parent.width
                spacing: 6

                Text {
                    text: qsTr("FILENAME PATTERN TEMPLATE")
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: themeManager.textMuted
                    font.letterSpacing: 1.0
                }

                Rectangle {
                    width: parent.width
                    height: 36
                    radius: themeManager.radiusMd
                    color: themeManager.surface
                    border.color: patternInput.activeFocus ? themeManager.borderFocus : themeManager.border
                    border.width: 1

                    TextInput {
                        id: patternInput
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        verticalAlignment: TextInput.AlignVCenter
                        text: "{date}_{original}_{seq:3}"
                        font.pixelSize: themeManager.fontSm
                        font.family: "Monospace"
                        color: themeManager.textPrimary
                        selectByMouse: true
                        onTextChanged: root.updatePreview()
                    }
                }

                // Insertable Token Pills
                Flow {
                    width: parent.width
                    spacing: 6

                    Repeater {
                        model: [
                            { token: "{original}", label: qsTr("Original Name") },
                            { token: "{date}", label: "YYYYMMDD" },
                            { token: "{time}", label: "HHMMSS" },
                            { token: "{seq:3}", label: "Seq (001)" },
                            { token: "{seq}", label: "Seq (1)" },
                            { token: "{camera}", label: qsTr("Camera") },
                            { token: "{rating}", label: qsTr("Rating") },
                            { token: "{label}", label: qsTr("Color Label") }
                        ]

                        delegate: Rectangle {
                            id: tokenPill
                            required property var modelData
                            height: 22
                            radius: 11
                            color: pillMouse.containsMouse ? themeManager.surfaceSelected : themeManager.surfaceElevated
                            border.color: pillMouse.containsMouse ? themeManager.accent : themeManager.border
                            border.width: 1
                            implicitWidth: pillText.implicitWidth + 14

                            Text {
                                id: pillText
                                anchors.centerIn: parent
                                text: "+" + tokenPill.modelData.token
                                font.pixelSize: 10
                                font.weight: Font.Medium
                                color: pillMouse.containsMouse ? themeManager.accent : themeManager.textSecondary
                            }

                            ToolTip.visible: pillMouse.containsMouse
                            ToolTip.text: tokenPill.modelData.label
                            ToolTip.delay: 300

                            MouseArea {
                                id: pillMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    patternInput.text += tokenPill.modelData.token;
                                }
                            }
                        }
                    }
                }
            }

            // Options Row: Sequence Settings & Case Conversion
            Row {
                width: parent.width
                spacing: 12

                // Sequence Start
                Column {
                    width: 110
                    spacing: 4

                    Text {
                        text: qsTr("START NUMBER")
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        color: themeManager.textMuted
                    }

                    Rectangle {
                        width: parent.width
                        height: 30
                        radius: themeManager.radiusSm
                        color: themeManager.surface
                        border.color: themeManager.border
                        border.width: 1

                        TextInput {
                            id: startSeqInput
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            verticalAlignment: TextInput.AlignVCenter
                            text: "1"
                            font.pixelSize: themeManager.fontSm
                            color: themeManager.textPrimary
                            validator: IntValidator { bottom: 1; top: 999999 }
                            onTextChanged: root.updatePreview()
                        }
                    }
                }

                // Sequence Step
                Column {
                    width: 90
                    spacing: 4

                    Text {
                        text: qsTr("STEP")
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        color: themeManager.textMuted
                    }

                    Rectangle {
                        width: parent.width
                        height: 30
                        radius: themeManager.radiusSm
                        color: themeManager.surface
                        border.color: themeManager.border
                        border.width: 1

                        TextInput {
                            id: stepInput
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            verticalAlignment: TextInput.AlignVCenter
                            text: "1"
                            font.pixelSize: themeManager.fontSm
                            color: themeManager.textPrimary
                            validator: IntValidator { bottom: 1; top: 100 }
                            onTextChanged: root.updatePreview()
                        }
                    }
                }

                // Case Conversion
                Column {
                    width: 140
                    spacing: 4

                    Text {
                        text: qsTr("CASE CONVERSION")
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        color: themeManager.textMuted
                    }

                    ComboBox {
                        id: caseCombo
                        width: parent.width
                        height: 30
                        model: [qsTr("Unchanged"), qsTr("lowercase"), qsTr("UPPERCASE"), qsTr("Title Case")]
                        font.pixelSize: themeManager.fontSm
                        onActivated: root.updatePreview()
                    }
                }

                // Find & Replace Inputs
                Column {
                    width: parent.width - 364
                    spacing: 4

                    Text {
                        text: qsTr("FIND & REPLACE (OPTIONAL)")
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        color: themeManager.textMuted
                    }

                    Row {
                        width: parent.width
                        spacing: 6

                        Rectangle {
                            width: (parent.width - 70) / 2
                            height: 30
                            radius: themeManager.radiusSm
                            color: themeManager.surface
                            border.color: themeManager.border
                            border.width: 1

                            TextInput {
                                id: findInput
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                verticalAlignment: TextInput.AlignVCenter
                                font.pixelSize: themeManager.fontSm
                                color: themeManager.textPrimary
                                selectByMouse: true
                                onTextChanged: root.updatePreview()

                                Text {
                                    anchors.fill: parent
                                    verticalAlignment: Text.AlignVCenter
                                    visible: !findInput.text && !findInput.activeFocus
                                    text: qsTr("Find text...")
                                    font.pixelSize: themeManager.fontSm
                                    color: themeManager.textMuted
                                }
                            }
                        }

                        Rectangle {
                            width: (parent.width - 70) / 2
                            height: 30
                            radius: themeManager.radiusSm
                            color: themeManager.surface
                            border.color: themeManager.border
                            border.width: 1

                            TextInput {
                                id: replaceInput
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                verticalAlignment: TextInput.AlignVCenter
                                font.pixelSize: themeManager.fontSm
                                color: themeManager.textPrimary
                                selectByMouse: true
                                onTextChanged: root.updatePreview()

                                Text {
                                    anchors.fill: parent
                                    verticalAlignment: Text.AlignVCenter
                                    visible: !replaceInput.text && !replaceInput.activeFocus
                                    text: qsTr("Replace with...")
                                    font.pixelSize: themeManager.fontSm
                                    color: themeManager.textMuted
                                }
                            }
                        }

                        CheckBox {
                            id: matchCaseCheck
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Aa")
                            font.pixelSize: 10
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Case Sensitive Find")
                            onToggled: root.updatePreview()
                        }
                    }
                }
            }

            // Preview Section Title
            Item {
                width: parent.width
                implicitHeight: 18

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("RENAME PREVIEW TABLE")
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: themeManager.textMuted
                    font.letterSpacing: 1.0
                }

                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    Text {
                        text: qsTr("✓ %1 Ready").arg(root.okCount)
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: "#22c55e"
                    }

                    Text {
                        visible: root.conflictCount > 0
                        text: qsTr("⚠ %1 Conflict(s)").arg(root.conflictCount)
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: themeManager.danger
                    }
                }
            }

            // Preview Table View
            Rectangle {
                width: parent.width
                height: 220
                radius: themeManager.radiusMd
                color: themeManager.surface
                border.color: themeManager.border
                border.width: 1
                clip: true

                // Header
                Rectangle {
                    id: tableHeader
                    width: parent.width
                    height: 28
                    color: themeManager.surfaceElevated
                    border.color: themeManager.border
                    border.width: 1

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10

                        Text {
                            width: (parent.width - 120) * 0.45
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Current Name")
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: themeManager.textSecondary
                        }

                        Text {
                            width: 20
                            anchors.verticalCenter: parent.verticalCenter
                            text: "→"
                            font.pixelSize: 11
                            color: themeManager.textMuted
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Text {
                            width: (parent.width - 120) * 0.55
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Proposed Name")
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: themeManager.textSecondary
                        }

                        Text {
                            width: 100
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Status")
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: themeManager.textSecondary
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }

                // Table List
                ListView {
                    id: previewList
                    anchors.top: tableHeader.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    model: root.previewCandidates
                    clip: true
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: Rectangle {
                        id: rowRect
                        required property var modelData
                        required property int index

                        width: previewList.width
                        height: 30
                        color: index % 2 === 0 ? "transparent" : themeManager.surfaceElevated

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10

                            Text {
                                width: (parent.width - 120) * 0.45
                                anchors.verticalCenter: parent.verticalCenter
                                text: rowRect.modelData.originalFileName
                                font.pixelSize: 11
                                color: themeManager.textPrimary
                                elide: Text.ElideMiddle
                            }

                            Text {
                                width: 20
                                anchors.verticalCenter: parent.verticalCenter
                                text: "→"
                                font.pixelSize: 10
                                color: themeManager.textMuted
                                horizontalAlignment: Text.AlignHCenter
                            }

                            Row {
                                width: (parent.width - 120) * 0.55
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 4

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: rowRect.modelData.proposedFileName
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                    color: rowRect.modelData.canRename ? themeManager.accent : themeManager.danger
                                    elide: Text.ElideMiddle
                                    width: Math.min(implicitWidth, parent.width - (rowRect.modelData.hasSidecar ? 40 : 0))
                                }

                                Rectangle {
                                    visible: rowRect.modelData.hasSidecar
                                    anchors.verticalCenter: parent.verticalCenter
                                    height: 14
                                    width: xmpBadgeText.implicitWidth + 6
                                    radius: 3
                                    color: themeManager.surfaceElevated
                                    border.color: themeManager.border
                                    border.width: 1

                                    Text {
                                        id: xmpBadgeText
                                        anchors.centerIn: parent
                                        text: "+XMP"
                                        font.pixelSize: 8
                                        font.weight: Font.Bold
                                        color: themeManager.textMuted
                                    }
                                }
                            }

                            // Status Badge
                            Rectangle {
                                width: 90
                                height: 20
                                radius: 4
                                anchors.verticalCenter: parent.verticalCenter
                                color: rowRect.modelData.canRename
                                       ? (rowRect.modelData.statusMessage === "Unchanged" ? themeManager.surfaceElevated : "#166534")
                                       : "#991b1b"

                                Text {
                                    anchors.centerIn: parent
                                    text: rowRect.modelData.statusMessage
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                    color: "#ffffff"
                                    elide: Text.ElideRight
                                    width: parent.width - 6
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                ToolTip.visible: rowMouseArea.containsMouse && !rowRect.modelData.canRename
                                ToolTip.text: rowRect.modelData.statusMessage
                            }
                        }

                        MouseArea {
                            id: rowMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                    }
                }
            }

            // Bottom Actions Bar
            Item {
                width: parent.width
                implicitHeight: 36

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.isExecuting
                          ? qsTr("Renaming in progress...")
                          : qsTr("Companion .xmp sidecars will be renamed in sync.")
                    font.pixelSize: 11
                    color: themeManager.textMuted
                }

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
                            enabled: !root.isExecuting
                            onClicked: sessionController.closeBatchRenameDialog()
                        }
                    }

                    // Execute Rename Button
                    Rectangle {
                        width: renameBtnText.implicitWidth + 24
                        height: 32
                        radius: themeManager.radiusMd
                        color: (root.okCount > 0 && !root.isExecuting)
                               ? (renameBtnMouse.containsMouse ? themeManager.accentHover : themeManager.accent)
                               : themeManager.surfaceElevated
                        opacity: (root.okCount > 0 && !root.isExecuting) ? 1.0 : 0.5
                        border.color: themeManager.accent
                        border.width: 1

                        Text {
                            id: renameBtnText
                            anchors.centerIn: parent
                            text: root.isExecuting
                                  ? qsTr("Renaming...")
                                  : qsTr("Rename %1 Photo(s)").arg(root.okCount)
                            font.pixelSize: themeManager.fontSm
                            font.weight: Font.Bold
                            color: (root.okCount > 0 && !root.isExecuting) ? "#ffffff" : themeManager.textMuted
                        }

                        MouseArea {
                            id: renameBtnMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            enabled: root.okCount > 0 && !root.isExecuting
                            onClicked: {
                                root.isExecuting = true;
                                var rule = {
                                    pattern: patternInput.text,
                                    startSequence: parseInt(startSeqInput.text) || 1,
                                    sequenceStep: parseInt(stepInput.text) || 1,
                                    findText: findInput.text,
                                    replaceText: replaceInput.text,
                                    matchCase: matchCaseCheck.checked,
                                    caseConversion: caseCombo.currentIndex
                                };
                                sessionController.executeBatchRename(rule);
                                root.isExecuting = false;
                                sessionController.closeBatchRenameDialog();
                            }
                        }
                    }
                }
            }
        }
    }
}
