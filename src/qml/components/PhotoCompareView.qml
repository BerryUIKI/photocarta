// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var compareList: sessionController.compareItems
    property int compareCount: sessionController.compareCount
    property real localPanX0: 0
    property real localPanY0: 0
    property real localZoom0: 1.0
    property real localPanX1: 0
    property real localPanY1: 0
    property real localZoom1: 1.0
    property real localPanX2: 0
    property real localPanY2: 0
    property real localZoom2: 1.0
    property real localPanX3: 0
    property real localPanY3: 0
    property real localZoom3: 1.0

    anchors.fill: parent
    focus: sessionController.compareMode

    // Deep neutral dark canvas
    Rectangle {
        anchors.fill: parent
        color: "#08090d"
    }

    // ==========================================
    // TOP FLOATING COMPARISON HEADER BAR
    // ==========================================
    Rectangle {
        id: compareHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 50
        color: "#e612141d"
        border.color: "#222634"
        border.width: 1
        z: 30

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            // Back to Grid View Button
            Rectangle {
                width: backRow.implicitWidth + 18
                height: 32
                radius: themeManager.radiusMd
                color: backMouse.containsMouse ? "#2a2e40" : "#1e2230"
                border.color: backMouse.containsMouse ? themeManager.accent : "#343a4e"
                border.width: 1

                Row {
                    id: backRow
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "←"
                        font.pixelSize: 13
                        font.weight: Font.Bold
                        color: "#f1f5f9"
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Grid (Esc)")
                        font.pixelSize: themeManager.fontSm
                        font.weight: Font.Medium
                        color: "#f1f5f9"
                    }
                }

                MouseArea {
                    id: backMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.setCompareMode(false)
                }
            }

            Rectangle {
                width: 1
                height: 20
                anchors.verticalCenter: parent.verticalCenter
                color: "#2e3448"
            }

            // Mode Title
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Light Table Comparison")
                font.pixelSize: themeManager.fontSm
                font.weight: Font.DemiBold
                color: "#f1f5f9"
            }

            // Photo Count Badge
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: countText.implicitWidth + 12
                height: 20
                radius: 10
                color: "#1e293b"
                border.color: "#334155"
                border.width: 1

                Text {
                    id: countText
                    anchors.centerIn: parent
                    text: qsTr("%1 Photos").arg(root.compareCount)
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    color: "#94a3b8"
                }
            }

            // Active Winner Badge (if any)
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                visible: !!sessionController.winnerId
                width: winnerRow.implicitWidth + 12
                height: 22
                radius: 6
                color: "#451a03"
                border.color: "#f59e0b"
                border.width: 1

                Row {
                    id: winnerRow
                    anchors.centerIn: parent
                    spacing: 4

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "★"
                        font.pixelSize: 11
                        color: "#f59e0b"
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Pick Selected")
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        color: "#fbbf24"
                    }
                }
            }
        }

        // Center / Right Controls: Sync Toggle, Zoom Controls, Add Photo
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            // Sync Zoom & Pan Toggle Button
            Rectangle {
                width: syncRow.implicitWidth + 16
                height: 32
                radius: themeManager.radiusMd
                color: sessionController.syncZoomPan ? "#1e3a5f" : (syncMouse.containsMouse ? "#262b3a" : "#181b24")
                border.color: sessionController.syncZoomPan ? "#38bdf8" : "#33384a"
                border.width: 1

                Row {
                    id: syncRow
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: sessionController.syncZoomPan ? "🔗" : "🔓"
                        font.pixelSize: 12
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: sessionController.syncZoomPan ? qsTr("Synced Zoom & Pan") : qsTr("Unlinked Views")
                        font.pixelSize: themeManager.fontSm
                        font.weight: Font.Medium
                        color: sessionController.syncZoomPan ? "#38bdf8" : "#94a3b8"
                    }
                }

                ToolTip.visible: syncMouse.containsMouse
                ToolTip.text: sessionController.syncZoomPan ? qsTr("Pan and zoom all photos in sync (S)") : qsTr("Pan and zoom photos independently (S)")

                MouseArea {
                    id: syncMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.toggleSyncZoomPan()
                }
            }

            Rectangle {
                width: 1
                height: 20
                anchors.verticalCenter: parent.verticalCenter
                color: "#2e3448"
            }

            // Zoom Out
            Rectangle {
                width: 30
                height: 30
                radius: 15
                anchors.verticalCenter: parent.verticalCenter
                color: zoomOutMouse.containsMouse ? "#272e40" : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "−"
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: "#f1f5f9"
                }

                ToolTip.visible: zoomOutMouse.containsMouse
                ToolTip.text: qsTr("Zoom Out (-)")

                MouseArea {
                    id: zoomOutMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.zoomOutCompare(0.25)
                }
            }

            // Zoom Percentage Indicator
            Rectangle {
                width: zoomText.implicitWidth + 14
                height: 24
                radius: 12
                anchors.verticalCenter: parent.verticalCenter
                color: zoomResetMouse.containsMouse ? "#2a3246" : "#1a1f2c"
                border.color: "#374158"
                border.width: 1

                Text {
                    id: zoomText
                    anchors.centerIn: parent
                    text: (Math.abs(sessionController.compareZoom - 1.0) < 0.05)
                          ? qsTr("Fit")
                          : qsTr("%1%").arg(Math.round(sessionController.compareZoom * 100))
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    color: (Math.abs(sessionController.compareZoom - 1.0) < 0.05) ? "#38bdf8" : "#f1f5f9"
                }

                ToolTip.visible: zoomResetMouse.containsMouse
                ToolTip.text: qsTr("Reset Zoom (0)")

                MouseArea {
                    id: zoomResetMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.resetCompareZoom()
                }
            }

            // Zoom In
            Rectangle {
                width: 30
                height: 30
                radius: 15
                anchors.verticalCenter: parent.verticalCenter
                color: zoomInMouse.containsMouse ? "#272e40" : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "+"
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: "#f1f5f9"
                }

                ToolTip.visible: zoomInMouse.containsMouse
                ToolTip.text: qsTr("Zoom In (+)")

                MouseArea {
                    id: zoomInMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.zoomInCompare(0.25)
                }
            }

            // Fit Button
            Rectangle {
                width: fitText.implicitWidth + 12
                height: 24
                radius: 12
                anchors.verticalCenter: parent.verticalCenter
                color: (Math.abs(sessionController.compareZoom - 1.0) < 0.05)
                       ? themeManager.surfaceSelected
                       : (fitBtnMouse.containsMouse ? "#272e40" : "transparent")

                Text {
                    id: fitText
                    anchors.centerIn: parent
                    text: qsTr("Fit")
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    color: "#f1f5f9"
                }

                ToolTip.visible: fitBtnMouse.containsMouse
                ToolTip.text: qsTr("Fit All to Screen (0)")

                MouseArea {
                    id: fitBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.resetCompareZoom()
                }
            }

            // 1:1 Actual Pixels Button
            Rectangle {
                width: actualText.implicitWidth + 12
                height: 24
                radius: 12
                anchors.verticalCenter: parent.verticalCenter
                color: (Math.abs(sessionController.compareZoom - 2.0) < 0.05)
                       ? themeManager.surfaceSelected
                       : (actualBtnMouse.containsMouse ? "#272e40" : "transparent")

                Text {
                    id: actualText
                    anchors.centerIn: parent
                    text: qsTr("1:1")
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    color: "#f1f5f9"
                }

                ToolTip.visible: actualBtnMouse.containsMouse
                ToolTip.text: qsTr("100% Actual Pixels (1)")

                MouseArea {
                    id: actualBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.setCompareZoom(2.0)
                }
            }

            // Add Next Photo to Compare Slot
            Rectangle {
                visible: root.compareCount < 4
                width: addPhotoRow.implicitWidth + 14
                height: 30
                radius: themeManager.radiusMd
                anchors.verticalCenter: parent.verticalCenter
                color: addPhotoMouse.containsMouse ? "#283042" : "#1a202c"
                border.color: "#3b4863"
                border.width: 1

                Row {
                    id: addPhotoRow
                    anchors.centerIn: parent
                    spacing: 5

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "+"
                        font.pixelSize: 13
                        font.weight: Font.Bold
                        color: "#38bdf8"
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Add Photo")
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        color: "#e2e8f0"
                    }
                }

                ToolTip.visible: addPhotoMouse.containsMouse
                ToolTip.text: qsTr("Add next photo from catalog to compare (up to 4)")

                MouseArea {
                    id: addPhotoMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.addAdjacentToCompare(true)
                }
            }
        }
    }

    // ==========================================
    // MAIN COMPARISON MULTI-PANE CANVAS
    // ==========================================
    Item {
        id: compareCanvas
        anchors.top: compareHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: compareFooter.top
        anchors.margins: 6
        clip: true

        // Grid layout calculation
        readonly property int cols: root.compareCount <= 2 ? root.compareCount : 2
        readonly property int rows: root.compareCount <= 2 ? 1 : Math.ceil(root.compareCount / 2)
        readonly property real cellWidth: (width - (cols - 1) * 6) / Math.max(1, cols)
        readonly property real cellHeight: (height - (rows - 1) * 6) / Math.max(1, rows)

        Repeater {
            model: root.compareList

            Item {
                id: slotCard
                property var itemData: modelData
                property int slotIndex: modelData.slotIndex !== undefined ? modelData.slotIndex : index
                property bool isWinner: !!modelData.isWinner
                property bool isActive: sessionController.activeCompareSlot === slotIndex

                // Position calculation based on grid
                readonly property int colIndex: slotIndex % compareCanvas.cols
                readonly property int rowIndex: Math.floor(slotIndex / compareCanvas.cols)

                x: colIndex * (compareCanvas.cellWidth + 6)
                y: rowIndex * (compareCanvas.cellHeight + 6)
                width: compareCanvas.cellWidth
                height: compareCanvas.cellHeight

                // Card background and border styling
                Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: "#0f1118"
                    border.color: slotCard.isWinner ? "#f59e0b" : (slotCard.isActive ? themeManager.accent : "#232838")
                    border.width: slotCard.isWinner ? 2 : (slotCard.isActive ? 2 : 1)
                }

                // ==========================================
                // ZOOMABLE & PANNABLE IMAGE CONTAINER
                // ==========================================
                Item {
                    id: slotImageArea
                    anchors.top: slotHeader.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: slotFooter.top
                    clip: true

                    // Local pan and zoom accessors
                    property real panX: sessionController.syncZoomPan ? sessionController.comparePanX : getLocalPanX(slotCard.slotIndex)
                    property real panY: sessionController.syncZoomPan ? sessionController.comparePanY : getLocalPanY(slotCard.slotIndex)
                    property real zoom: sessionController.syncZoomPan ? sessionController.compareZoom : getLocalZoom(slotCard.slotIndex)

                    function getLocalPanX(idx) {
                        if (idx === 0) return root.localPanX0;
                        if (idx === 1) return root.localPanX1;
                        if (idx === 2) return root.localPanX2;
                        return root.localPanX3;
                    }
                    function getLocalPanY(idx) {
                        if (idx === 0) return root.localPanY0;
                        if (idx === 1) return root.localPanY1;
                        if (idx === 2) return root.localPanY2;
                        return root.localPanY3;
                    }
                    function getLocalZoom(idx) {
                        if (idx === 0) return root.localZoom0;
                        if (idx === 1) return root.localZoom1;
                        if (idx === 2) return root.localZoom2;
                        return root.localZoom3;
                    }
                    function setLocalPan(idx, px, py) {
                        if (idx === 0) { root.localPanX0 = px; root.localPanY0 = py; }
                        else if (idx === 1) { root.localPanX1 = px; root.localPanY1 = py; }
                        else if (idx === 2) { root.localPanX2 = px; root.localPanY2 = py; }
                        else { root.localPanX3 = px; root.localPanY3 = py; }
                    }
                    function setLocalZoom(idx, z) {
                        const cl = Math.max(0.2, Math.min(8.0, z));
                        if (idx === 0) root.localZoom0 = cl;
                        else if (idx === 1) root.localZoom1 = cl;
                        else if (idx === 2) root.localZoom2 = cl;
                        else root.localZoom3 = cl;
                    }

                    // Scaled image element
                    Item {
                        id: imageWrapper
                        x: slotImageArea.width / 2 + slotImageArea.panX - width / 2
                        y: slotImageArea.height / 2 + slotImageArea.panY - height / 2
                        width: Math.max(50, (slotImageArea.width - 20) * slotImageArea.zoom)
                        height: Math.max(50, (slotImageArea.height - 20) * slotImageArea.zoom)

                        Image {
                            id: slotImage
                            anchors.fill: parent
                            source: slotCard.itemData.previewUrl ? slotCard.itemData.previewUrl : ""
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                            cache: true
                            sourceSize.width: 3840
                            sourceSize.height: 2560
                            smooth: true
                            mipmap: true

                            // Loading state overlay
                            Rectangle {
                                anchors.fill: parent
                                color: "#0a0c12"
                                opacity: slotImage.status === Image.Loading ? 0.8 : 0.0
                                Behavior on opacity { NumberAnimation { duration: 150 } }

                                BusyIndicator {
                                    anchors.centerIn: parent
                                    running: slotImage.status === Image.Loading
                                }
                            }
                        }
                    }

                    // Mouse Wheel Zoom
                    WheelHandler {
                        target: null
                        onWheel: (event) => {
                            sessionController.setActiveCompareSlot(slotCard.slotIndex);
                            const factor = (event.angleDelta.y > 0) ? 1.25 : 0.8;
                            if (sessionController.syncZoomPan) {
                                sessionController.setCompareZoom(sessionController.compareZoom * factor);
                            } else {
                                slotImageArea.setLocalZoom(slotCard.slotIndex, slotImageArea.zoom * factor);
                            }
                        }
                    }

                    // Drag Pan and Double Click Handler
                    MouseArea {
                        id: slotMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton
                        cursorShape: slotImageArea.zoom > 1.05
                                     ? (slotMouseArea.pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                                     : Qt.ArrowCursor

                        property real startX: 0
                        property real startY: 0
                        property real initPanX: 0
                        property real initPanY: 0

                        onPressed: (mouse) => {
                            sessionController.setActiveCompareSlot(slotCard.slotIndex);
                            startX = mouse.x;
                            startY = mouse.y;
                            initPanX = slotImageArea.panX;
                            initPanY = slotImageArea.panY;
                        }

                        onPositionChanged: (mouse) => {
                            if (pressed && slotImageArea.zoom > 1.05) {
                                const newPx = initPanX + (mouse.x - startX);
                                const newPy = initPanY + (mouse.y - startY);
                                if (sessionController.syncZoomPan) {
                                    sessionController.setComparePan(newPx, newPy);
                                } else {
                                    slotImageArea.setLocalPan(slotCard.slotIndex, newPx, newPy);
                                }
                            }
                        }

                        onDoubleClicked: (mouse) => {
                            sessionController.setActiveCompareSlot(slotCard.slotIndex);
                            if (slotImageArea.zoom <= 1.05) {
                                if (sessionController.syncZoomPan) {
                                    sessionController.setCompareZoom(2.0);
                                } else {
                                    slotImageArea.setLocalZoom(slotCard.slotIndex, 2.0);
                                }
                            } else {
                                if (sessionController.syncZoomPan) {
                                    sessionController.resetCompareZoom();
                                } else {
                                    slotImageArea.setLocalZoom(slotCard.slotIndex, 1.0);
                                    slotImageArea.setLocalPan(slotCard.slotIndex, 0.0, 0.0);
                                }
                            }
                        }
                    }

                    // Pinch handler for trackpad gestures
                    PinchArea {
                        anchors.fill: parent
                        pinch.target: null
                        property real initZ: 1.0

                        onPinchStarted: {
                            sessionController.setActiveCompareSlot(slotCard.slotIndex);
                            initZ = slotImageArea.zoom;
                        }

                        onPinchUpdated: (pinch) => {
                            const newZ = initZ * pinch.scale;
                            if (sessionController.syncZoomPan) {
                                sessionController.setCompareZoom(newZ);
                                sessionController.setComparePan(
                                    sessionController.comparePanX + (pinch.previousCenter.x - pinch.center.x),
                                    sessionController.comparePanY + (pinch.previousCenter.y - pinch.center.y)
                                );
                            } else {
                                slotImageArea.setLocalZoom(slotCard.slotIndex, newZ);
                                slotImageArea.setLocalPan(
                                    slotCard.slotIndex,
                                    slotImageArea.panX + (pinch.previousCenter.x - pinch.center.x),
                                    slotImageArea.panY + (pinch.previousCenter.y - pinch.center.y)
                                );
                            }
                        }
                    }
                }

                // ==========================================
                // SLOT HEADER OVERLAY
                // ==========================================
                Rectangle {
                    id: slotHeader
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 36
                    color: slotCard.isWinner ? "#e62e1a06" : (slotCard.isActive ? "#e6181c28" : "#d912151f")
                    radius: 8
                    z: 10

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8

                        // Slot Letter Pill (A, B, C, D)
                        Rectangle {
                            width: 22
                            height: 22
                            radius: 11
                            anchors.verticalCenter: parent.verticalCenter
                            color: slotCard.isWinner ? "#f59e0b" : (slotCard.isActive ? themeManager.accent : "#33384a")

                            Text {
                                anchors.centerIn: parent
                                text: slotCard.itemData.slotLabel ? slotCard.itemData.slotLabel : (slotCard.slotIndex + 1)
                                font.pixelSize: 11
                                font.weight: Font.Bold
                                color: slotCard.isWinner ? "#1e1303" : "#ffffff"
                            }
                        }

                        // Photo Name / Title
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: slotCard.itemData.fileName ? slotCard.itemData.fileName : ""
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            color: "#f8fafc"
                            elide: Text.ElideRight
                            maximumLineCount: 1
                            width: Math.min(220, slotCard.width - 200)
                        }

                        // Dimensions / Megapixel Badge
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            visible: !!slotCard.itemData.formattedDimensions
                            width: slotDimText.implicitWidth + 8
                            height: 18
                            radius: 4
                            color: "#1e2230"
                            border.color: "#31374a"
                            border.width: 1

                            Text {
                                id: slotDimText
                                anchors.centerIn: parent
                                text: slotCard.itemData.formattedDimensions
                                font.pixelSize: 9
                                font.weight: Font.Medium
                                color: "#94a3b8"
                            }
                        }
                    }

                    // Header Right Actions: Pick Best Winner & Remove Slot
                    Row {
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6

                        // "Pick Best" Button
                        Rectangle {
                            width: pickRow.implicitWidth + 12
                            height: 24
                            radius: 6
                            anchors.verticalCenter: parent.verticalCenter
                            color: slotCard.isWinner ? "#f59e0b" : (pickMouse.containsMouse ? "#272f42" : "#1a1f2c")
                            border.color: slotCard.isWinner ? "#fbbf24" : (pickMouse.containsMouse ? themeManager.accent : "#343d52")
                            border.width: 1

                            Row {
                                id: pickRow
                                anchors.centerIn: parent
                                spacing: 4

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "★"
                                    font.pixelSize: 11
                                    color: slotCard.isWinner ? "#1e1303" : "#f59e0b"
                                }

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: slotCard.isWinner ? qsTr("BEST PICK") : qsTr("Pick Best")
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    color: slotCard.isWinner ? "#1e1303" : "#f1f5f9"
                                }
                            }

                            ToolTip.visible: pickMouse.containsMouse
                            ToolTip.text: slotCard.isWinner ? qsTr("Selected as best candidate (W to toggle)") : qsTr("Mark this photo as winner candidate (W)")

                            MouseArea {
                                id: pickMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: sessionController.setAsWinner(slotCard.slotIndex)
                            }
                        }

                        // Remove from Compare Slot
                        Rectangle {
                            width: 24
                            height: 24
                            radius: 12
                            anchors.verticalCenter: parent.verticalCenter
                            color: removeMouse.containsMouse ? "#ef4444" : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: "✕"
                                font.pixelSize: 11
                                font.weight: Font.Bold
                                color: removeMouse.containsMouse ? "#ffffff" : "#94a3b8"
                            }

                            ToolTip.visible: removeMouse.containsMouse
                            ToolTip.text: qsTr("Remove from comparison")

                            MouseArea {
                                id: removeMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: sessionController.removeCompareItem(slotCard.slotIndex)
                            }
                        }
                    }
                }

                // ==========================================
                // SLOT FOOTER OVERLAY (RATING & EXIF HUD)
                // ==========================================
                Rectangle {
                    id: slotFooter
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 38
                    color: "#e60f121a"
                    radius: 8
                    z: 10

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8

                        // Interactive Flag
                        Rectangle {
                            width: 24
                            height: 24
                            radius: 6
                            anchors.verticalCenter: parent.verticalCenter
                            color: slotCard.itemData.isFlagged ? (themeManager.isDark ? "#451a03" : "#fef3c7") : (flagSlotMouse.containsMouse ? "#222533" : "transparent")
                            border.color: slotCard.itemData.isFlagged ? "#f59e0b" : "transparent"
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: "⚑"
                                font.pixelSize: 13
                                color: slotCard.itemData.isFlagged ? "#f59e0b" : "#94a3b8"
                            }

                            ToolTip.visible: flagSlotMouse.containsMouse
                            ToolTip.text: slotCard.itemData.isFlagged ? qsTr("Flagged") : qsTr("Flag photo (F)")

                            MouseArea {
                                id: flagSlotMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: sessionController.toggleCompareFlagged(slotCard.slotIndex)
                            }
                        }

                        // Rating Stars
                        RatingStars {
                            anchors.verticalCenter: parent.verticalCenter
                            rating: slotCard.itemData.rating ? slotCard.itemData.rating : 0
                            starSize: 13
                            interactive: true
                            onRatingSelected: (newRating) => {
                                sessionController.setCompareRating(slotCard.slotIndex, newRating)
                            }
                        }
                    }

                    // Compact EXIF HUD on Right Side of Slot Footer
                    Row {
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6
                        visible: !!slotCard.itemData.cameraModel || !!slotCard.itemData.aperture

                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: slotExifText.implicitWidth + 10
                            height: 22
                            radius: 4
                            color: "#181d29"
                            border.color: "#293246"
                            border.width: 1

                            Text {
                                id: slotExifText
                                anchors.centerIn: parent
                                text: qsTr("%1 %2 %3 ISO %4")
                                      .arg(slotCard.itemData.focalLength ? slotCard.itemData.focalLength : "")
                                      .arg(slotCard.itemData.aperture ? slotCard.itemData.aperture : "")
                                      .arg(slotCard.itemData.shutterSpeed ? slotCard.itemData.shutterSpeed : "")
                                      .arg(slotCard.itemData.iso ? slotCard.itemData.iso : 0)
                                font.pixelSize: 10
                                font.weight: Font.Medium
                                color: "#38bdf8"
                            }
                        }
                    }
                }
            }
        }
    }

    // ==========================================
    // BOTTOM SHORTCUTS BAR / STATUS
    // ==========================================
    Rectangle {
        id: compareFooter
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 28
        color: "#10121a"
        border.color: "#1e2230"
        border.width: 1
        z: 30

        Row {
            anchors.centerIn: parent
            spacing: 16

            Text {
                text: qsTr("Esc: Exit to Grid")
                font.pixelSize: 11
                color: "#64748b"
            }
            Text {
                text: "•"
                font.pixelSize: 11
                color: "#334155"
            }
            Text {
                text: qsTr("S: Toggle Sync Zoom & Pan")
                font.pixelSize: 11
                color: "#64748b"
            }
            Text {
                text: "•"
                font.pixelSize: 11
                color: "#334155"
            }
            Text {
                text: qsTr("W: Mark Active Slot as Winner")
                font.pixelSize: 11
                color: "#64748b"
            }
            Text {
                text: "•"
                font.pixelSize: 11
                color: "#334155"
            }
            Text {
                text: qsTr("1-5: Rate Active")
                font.pixelSize: 11
                color: "#64748b"
            }
            Text {
                text: "•"
                font.pixelSize: 11
                color: "#334155"
            }
            Text {
                text: qsTr("F: Flag Active")
                font.pixelSize: 11
                color: "#64748b"
            }
            Text {
                text: "•"
                font.pixelSize: 11
                color: "#334155"
            }
            Text {
                text: qsTr("Wheel / Double-Click: Zoom")
                font.pixelSize: 11
                color: "#64748b"
            }
        }
    }

    // ==========================================
    // KEYBOARD SHORTCUTS FOR COMPARE VIEW
    // ==========================================
    Shortcut {
        sequence: "S"
        enabled: sessionController.compareMode
        onActivated: sessionController.toggleSyncZoomPan()
    }

    Shortcut {
        sequence: "W"
        enabled: sessionController.compareMode
        onActivated: sessionController.setAsWinner(sessionController.activeCompareSlot)
    }

    Shortcut {
        sequence: "F"
        enabled: sessionController.compareMode
        onActivated: sessionController.toggleCompareFlagged(sessionController.activeCompareSlot)
    }

    Shortcut {
        sequence: "1"
        enabled: sessionController.compareMode
        onActivated: sessionController.setCompareRating(sessionController.activeCompareSlot, 1)
    }

    Shortcut {
        sequence: "2"
        enabled: sessionController.compareMode
        onActivated: sessionController.setCompareRating(sessionController.activeCompareSlot, 2)
    }

    Shortcut {
        sequence: "3"
        enabled: sessionController.compareMode
        onActivated: sessionController.setCompareRating(sessionController.activeCompareSlot, 3)
    }

    Shortcut {
        sequence: "4"
        enabled: sessionController.compareMode
        onActivated: sessionController.setCompareRating(sessionController.activeCompareSlot, 4)
    }

    Shortcut {
        sequence: "5"
        enabled: sessionController.compareMode
        onActivated: sessionController.setCompareRating(sessionController.activeCompareSlot, 5)
    }

    Shortcut {
        sequence: "6"
        enabled: sessionController.compareMode
        onActivated: sessionController.setCompareColorLabel(sessionController.activeCompareSlot, 1)
    }

    Shortcut {
        sequence: "7"
        enabled: sessionController.compareMode
        onActivated: sessionController.setCompareColorLabel(sessionController.activeCompareSlot, 2)
    }

    Shortcut {
        sequence: "8"
        enabled: sessionController.compareMode
        onActivated: sessionController.setCompareColorLabel(sessionController.activeCompareSlot, 3)
    }

    Shortcut {
        sequence: "9"
        enabled: sessionController.compareMode
        onActivated: sessionController.setCompareColorLabel(sessionController.activeCompareSlot, 4)
    }

    Shortcut {
        sequence: "0"
        enabled: sessionController.compareMode
        onActivated: sessionController.resetCompareZoom()
    }

    Shortcut {
        sequence: "="
        enabled: sessionController.compareMode
        onActivated: sessionController.zoomInCompare(0.25)
    }

    Shortcut {
        sequence: "-"
        enabled: sessionController.compareMode
        onActivated: sessionController.zoomOutCompare(0.25)
    }
}
