// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var currentItem: selectionManager.selectedItem
    property real panX: 0
    property real panY: 0
    property bool filmstripVisible: true

    anchors.fill: parent
    focus: sessionController.previewMode

    // Reset pan whenever current photo changes
    Connections {
        target: selectionManager
        function onSelectionChanged() {
            root.panX = 0;
            root.panY = 0;
        }
    }

    // Reset pan if zoom drops back to fit
    Connections {
        target: sessionController
        function onViewerZoomChanged() {
            if (sessionController.viewerZoom <= 1.0) {
                root.panX = 0;
                root.panY = 0;
            }
        }
    }

    // Deep neutral dark canvas
    Rectangle {
        anchors.fill: parent
        color: "#090a0f"
    }

    // ==========================================
    // TOP FLOATING HEADER BAR
    // ==========================================
    Rectangle {
        id: previewHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 48
        color: "#d912141c"
        border.color: "#252836"
        border.width: 1
        z: 20

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
                color: backMouse.containsMouse ? "#2a2d3d" : "#1e212d"
                border.color: backMouse.containsMouse ? themeManager.accent : "#34384a"
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
                    onClicked: {
                        if (sessionController.isFullscreen) {
                            sessionController.setFullscreen(false);
                        }
                        sessionController.setPreviewMode(false);
                    }
                }
            }

            Rectangle {
                width: 1
                height: 20
                anchors.verticalCenter: parent.verticalCenter
                color: "#2e3244"
            }

            // Photo Title / File Name
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.currentItem.title ? root.currentItem.title : (root.currentItem.fileName ? root.currentItem.fileName : "")
                font.pixelSize: themeManager.fontSm
                font.weight: Font.DemiBold
                color: "#f1f5f9"
                elide: Text.ElideRight
                maximumLineCount: 1
                width: Math.min(320, root.width - 600)
            }

            // Dimensions / MP Badge
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                visible: !!root.currentItem.formattedDimensions
                width: dimText.implicitWidth + 10
                height: 20
                radius: 4
                color: "#1e2230"
                border.color: "#31374a"
                border.width: 1

                Text {
                    id: dimText
                    anchors.centerIn: parent
                    text: root.currentItem.formattedDimensions
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    color: "#94a3b8"
                }
            }

            // Demo Badge
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.currentItem.isDemoFixture === true
                width: previewDemoText.implicitWidth + 8
                height: 18
                radius: 4
                color: themeManager.badgeDemoBg
                border.color: themeManager.badgeDemoText
                border.width: 1

                Text {
                    id: previewDemoText
                    anchors.centerIn: parent
                    text: qsTr("DEMO")
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: themeManager.badgeDemoText
                }
            }
        }

        // Right side: Interactive Rating, Flag & Position Counter
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            // Histogram HUD Toggle Button
            Rectangle {
                width: 28
                height: 28
                radius: 6
                anchors.verticalCenter: parent.verticalCenter
                color: sessionController.histogramVisible ? themeManager.surfaceSelected : (histBtnMouse.containsMouse ? "#222533" : "transparent")
                border.color: sessionController.histogramVisible ? themeManager.accent : (histBtnMouse.containsMouse ? "#3b4256" : "transparent")
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "📊"
                    font.pixelSize: 13
                }

                ToolTip.visible: histBtnMouse.containsMouse
                ToolTip.text: sessionController.histogramVisible ? qsTr("Hide Histogram HUD (H)") : qsTr("Show Histogram HUD (H)")

                MouseArea {
                    id: histBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.toggleHistogramVisible()
                }
            }

            // Interactive Flag Button
            Rectangle {
                width: 28
                height: 28
                radius: 6
                anchors.verticalCenter: parent.verticalCenter
                color: root.currentItem.isFlagged ? (themeManager.isDark ? "#451a03" : "#fef3c7") : (flagMouse.containsMouse ? "#222533" : "transparent")
                border.color: root.currentItem.isFlagged ? "#f59e0b" : (flagMouse.containsMouse ? "#3b4256" : "transparent")
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "⚑"
                    font.pixelSize: 14
                    color: root.currentItem.isFlagged ? "#f59e0b" : "#94a3b8"
                }

                ToolTip.visible: flagMouse.containsMouse
                ToolTip.text: root.currentItem.isFlagged ? qsTr("Flagged (Space/F to toggle)") : qsTr("Flag photo (Space/F)")

                MouseArea {
                    id: flagMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.toggleItemFlagged(selectionManager.currentIndex)
                }
            }

            // Interactive Rating Stars
            RatingStars {
                anchors.verticalCenter: parent.verticalCenter
                rating: root.currentItem.rating ? root.currentItem.rating : 0
                starSize: 14
                interactive: true
                onRatingSelected: (newRating) => {
                    sessionController.setItemRating(selectionManager.currentIndex, newRating)
                }
            }

            Rectangle {
                width: 1
                height: 20
                anchors.verticalCenter: parent.verticalCenter
                color: "#2e3244"
            }

            // Photo Index Counter
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("%1 of %2").arg(selectionManager.currentIndex + 1).arg(filterModel.count)
                font.pixelSize: themeManager.fontSm
                font.weight: Font.Medium
                color: "#94a3b8"
            }
        }
    }

    // ==========================================
    // INTERACTIVE ZOOM & PAN CANVAS
    // ==========================================
    Item {
        id: canvasArea
        anchors.top: previewHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: (root.filmstripVisible ? filmstripBar.top : parent.bottom)
        clip: true

        // Viewport dimensions accounting for breathing room margin
        readonly property real viewportW: Math.max(100, width - 40)
        readonly property real viewportH: Math.max(100, height - 40)

        // Raw source image aspect ratio
        readonly property real rawWidth: (root.currentItem.dimensions && root.currentItem.dimensions.width > 0)
                                         ? root.currentItem.dimensions.width
                                         : (mainPreviewImage.implicitWidth > 0 ? mainPreviewImage.implicitWidth : 1920)
        readonly property real rawHeight: (root.currentItem.dimensions && root.currentItem.dimensions.height > 0)
                                          ? root.currentItem.dimensions.height
                                          : (mainPreviewImage.implicitHeight > 0 ? mainPreviewImage.implicitHeight : 1080)

        // Swap dimensions if rotated 90 or 270 degrees
        readonly property bool isRotated90: (sessionController.viewerRotation % 180 !== 0)
        readonly property real effectiveWidth: isRotated90 ? rawHeight : rawWidth
        readonly property real effectiveHeight: isRotated90 ? rawWidth : rawHeight
        readonly property real aspectRatio: effectiveWidth / Math.max(1.0, effectiveHeight)

        // Fitted dimensions inside viewport
        readonly property real fitWidth: (viewportW / viewportH > aspectRatio)
                                         ? (viewportH * aspectRatio)
                                         : viewportW
        readonly property real fitHeight: (viewportW / viewportH > aspectRatio)
                                          ? viewportH
                                          : (viewportW / aspectRatio)

        // Current zoomed dimensions
        readonly property real currentWidth: fitWidth * sessionController.viewerZoom
        readonly property real currentHeight: fitHeight * sessionController.viewerZoom

        // Center coordinates with pan offset
        readonly property real imageCenterX: canvasArea.width / 2 + root.panX
        readonly property real imageCenterY: canvasArea.height / 2 + root.panY

        // Pan bounds constraint
        readonly property real maxPanX: Math.max(0, (currentWidth - viewportW) / 2 + 100)
        readonly property real maxPanY: Math.max(0, (currentHeight - viewportH) / 2 + 100)

        // Clamp pan values within bounds
        function clampPan() {
            if (sessionController.viewerZoom <= 1.0) {
                root.panX = 0;
                root.panY = 0;
            } else {
                root.panX = Math.max(-maxPanX, Math.min(maxPanX, root.panX));
                root.panY = Math.max(-maxPanY, Math.min(maxPanY, root.panY));
            }
        }

        // Image Container for Zoom, Pan and Rotation
        Item {
            id: imageContainer
            x: canvasArea.imageCenterX - canvasArea.currentWidth / 2
            y: canvasArea.imageCenterY - canvasArea.currentHeight / 2
            width: canvasArea.currentWidth
            height: canvasArea.currentHeight

            Image {
                id: mainPreviewImage
                anchors.centerIn: parent
                width: canvasArea.isRotated90 ? parent.height : parent.width
                height: canvasArea.isRotated90 ? parent.width : parent.height
                rotation: sessionController.viewerRotation
                source: root.currentItem.previewUrl ? root.currentItem.previewUrl : ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                cache: true
                // Bound max decoded texture to prevent GPU memory exhaust on massive RAW/60MP files
                sourceSize.width: 3840
                sourceSize.height: 2560
                smooth: true
                mipmap: true

                // Loading Spinner / Dark Overlay
                Rectangle {
                    anchors.fill: parent
                    color: "#090a0f"
                    opacity: mainPreviewImage.status === Image.Loading ? 0.7 : 0.0
                    Behavior on opacity { NumberAnimation { duration: 150 } }

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: mainPreviewImage.status === Image.Loading
                    }
                }
            }
        }

        // Mouse Wheel Zoom centered on cursor position
        WheelHandler {
            target: null
            onWheel: (event) => {
                const zoomFactorDelta = (event.angleDelta.y > 0) ? 1.25 : 0.8;
                const oldZoom = sessionController.viewerZoom;
                const newZoom = Math.max(0.2, Math.min(10.0, oldZoom * zoomFactorDelta));

                if (Math.abs(newZoom - oldZoom) > 0.01) {
                    const mouseX = event.position.x;
                    const mouseY = event.position.y;
                    const relX = mouseX - (canvasArea.width / 2 + root.panX);
                    const relY = mouseY - (canvasArea.height / 2 + root.panY);
                    const ratio = newZoom / oldZoom;

                    root.panX = mouseX - (canvasArea.width / 2 + relX * ratio);
                    root.panY = mouseY - (canvasArea.height / 2 + relY * ratio);
                    sessionController.setViewerZoom(newZoom);
                    canvasArea.clampPan();
                }
            }
        }

        // Mouse Area for Pan Dragging & Double Click Zoom Toggle
        MouseArea {
            id: panMouseArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            cursorShape: sessionController.viewerZoom > 1.05
                         ? (panMouseArea.pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                         : Qt.ArrowCursor

            property real startMouseX: 0
            property real startMouseY: 0
            property real initialPanX: 0
            property real initialPanY: 0

            onPressed: (mouse) => {
                startMouseX = mouse.x;
                startMouseY = mouse.y;
                initialPanX = root.panX;
                initialPanY = root.panY;
            }

            onPositionChanged: (mouse) => {
                if (pressed && sessionController.viewerZoom > 1.05) {
                    root.panX = initialPanX + (mouse.x - startMouseX);
                    root.panY = initialPanY + (mouse.y - startMouseY);
                    canvasArea.clampPan();
                }
            }

            onDoubleClicked: (mouse) => {
                if (sessionController.viewerZoom <= 1.05) {
                    // Zoom in to 2.0x centered on double clicked position
                    const oldZoom = sessionController.viewerZoom;
                    const newZoom = 2.0;
                    const mouseX = mouse.x;
                    const mouseY = mouse.y;
                    const relX = mouseX - (canvasArea.width / 2 + root.panX);
                    const relY = mouseY - (canvasArea.height / 2 + root.panY);
                    const ratio = newZoom / oldZoom;

                    root.panX = mouseX - (canvasArea.width / 2 + relX * ratio);
                    root.panY = mouseY - (canvasArea.height / 2 + relY * ratio);
                    sessionController.setViewerZoom(newZoom);
                    canvasArea.clampPan();
                } else {
                    // Smoothly reset back to Fit
                    sessionController.resetViewerZoom();
                    root.panX = 0;
                    root.panY = 0;
                }
            }
        }

        // Touch Trackpad Pinch Zoom & Pan Handler
        PinchArea {
            anchors.fill: parent
            pinch.target: null
            property real initialZoom: 1.0

            onPinchStarted: {
                initialZoom = sessionController.viewerZoom;
            }

            onPinchUpdated: (pinch) => {
                const newZoom = Math.max(0.2, Math.min(10.0, initialZoom * pinch.scale));
                sessionController.setViewerZoom(newZoom);
                root.panX += pinch.previousCenter.x - pinch.center.x;
                root.panY += pinch.previousCenter.y - pinch.center.y;
                canvasArea.clampPan();
            }
        }

        // ==========================================
        // LEFT & RIGHT NAVIGATION CHEVRONS
        // ==========================================
        Rectangle {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            width: 48
            height: 48
            radius: 24
            color: leftNavMouse.containsMouse ? "#e61e2433" : "#80121622"
            border.color: leftNavMouse.containsMouse ? themeManager.accent : "#343d52"
            border.width: 1
            visible: selectionManager.currentIndex > 0
            z: 10

            Text {
                anchors.centerIn: parent
                text: "‹"
                font.pixelSize: 28
                font.weight: Font.Light
                color: "#f8fafc"
            }

            ToolTip.visible: leftNavMouse.containsMouse
            ToolTip.text: qsTr("Previous Photo (Left / A)")

            MouseArea {
                id: leftNavMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: selectionManager.selectPrevious()
            }
        }

        Rectangle {
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            width: 48
            height: 48
            radius: 24
            color: rightNavMouse.containsMouse ? "#e61e2433" : "#80121622"
            border.color: rightNavMouse.containsMouse ? themeManager.accent : "#343d52"
            border.width: 1
            visible: selectionManager.currentIndex + 1 < filterModel.count
            z: 10

            Text {
                anchors.centerIn: parent
                text: "›"
                font.pixelSize: 28
                font.weight: Font.Light
                color: "#f8fafc"
            }

            ToolTip.visible: rightNavMouse.containsMouse
            ToolTip.text: qsTr("Next Photo (Right / D)")

            MouseArea {
                id: rightNavMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: selectionManager.selectNext()
            }
        }

        // ==========================================
        // FLOATING HISTOGRAM HUD PANEL
        // ==========================================
        HistogramView {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 14
            width: 250
            compact: true
            visible: sessionController.histogramVisible
            z: 25
            opacity: 0.94
        }

        // ==========================================
        // FLOATING TECHNICAL EXIF HUD BADGE
        // ==========================================
        Rectangle {
            anchors.bottom: bottomControlBar.top
            anchors.bottomMargin: 12
            anchors.horizontalCenter: parent.horizontalCenter
            width: exifRow.implicitWidth + 24
            height: 30
            radius: 15
            color: "#cc0f131a"
            border.color: "#334155"
            border.width: 1
            visible: !!root.currentItem.cameraModel
            z: 10

            Row {
                id: exifRow
                anchors.centerIn: parent
                spacing: 8

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.currentItem.cameraModel
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    color: "#f8fafc"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "•"
                    font.pixelSize: 10
                    color: "#64748b"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.currentItem.lensModel
                    font.pixelSize: 11
                    color: "#94a3b8"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "•"
                    font.pixelSize: 10
                    color: "#64748b"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("%1  %2  %3  ISO %4")
                          .arg(root.currentItem.focalLength ? root.currentItem.focalLength : "")
                          .arg(root.currentItem.aperture ? root.currentItem.aperture : "")
                          .arg(root.currentItem.shutterSpeed ? root.currentItem.shutterSpeed : "")
                          .arg(root.currentItem.iso ? root.currentItem.iso : 0)
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    color: "#38bdf8"
                }
            }
        }

        // ==========================================
        // FLOATING INTERACTIVE CONTROLS PILL BAR
        // ==========================================
        Rectangle {
            id: bottomControlBar
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 14
            anchors.horizontalCenter: parent.horizontalCenter
            width: controlRow.implicitWidth + 20
            height: 38
            radius: 19
            color: "#e610131d"
            border.color: "#2a3142"
            border.width: 1
            z: 15

            Row {
                id: controlRow
                anchors.centerIn: parent
                spacing: 4

                // Prev Photo
                Rectangle {
                    width: 30
                    height: 30
                    radius: 15
                    color: prevBtnMouse.containsMouse ? "#272e40" : "transparent"
                    enabled: selectionManager.currentIndex > 0
                    opacity: enabled ? 1.0 : 0.4

                    Text {
                        anchors.centerIn: parent
                        text: "◀"
                        font.pixelSize: 10
                        color: "#f1f5f9"
                    }

                    ToolTip.visible: prevBtnMouse.containsMouse
                    ToolTip.text: qsTr("Previous Photo (A / Left)")

                    MouseArea {
                        id: prevBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: selectionManager.selectPrevious()
                    }
                }

                // Next Photo
                Rectangle {
                    width: 30
                    height: 30
                    radius: 15
                    color: nextBtnMouse.containsMouse ? "#272e40" : "transparent"
                    enabled: selectionManager.currentIndex + 1 < filterModel.count
                    opacity: enabled ? 1.0 : 0.4

                    Text {
                        anchors.centerIn: parent
                        text: "▶"
                        font.pixelSize: 10
                        color: "#f1f5f9"
                    }

                    ToolTip.visible: nextBtnMouse.containsMouse
                    ToolTip.text: qsTr("Next Photo (D / Right)")

                    MouseArea {
                        id: nextBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: selectionManager.selectNext()
                    }
                }

                // Vertical Divider
                Rectangle {
                    width: 1
                    height: 18
                    anchors.verticalCenter: parent.verticalCenter
                    color: "#2e364a"
                }

                // Zoom Out Button
                Rectangle {
                    width: 30
                    height: 30
                    radius: 15
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
                        onClicked: sessionController.zoomOut(0.25)
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
                        text: (Math.abs(sessionController.viewerZoom - 1.0) < 0.05)
                              ? qsTr("Fit")
                              : qsTr("%1%").arg(Math.round(sessionController.viewerZoom * 100))
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: (Math.abs(sessionController.viewerZoom - 1.0) < 0.05) ? "#38bdf8" : "#f1f5f9"
                    }

                    ToolTip.visible: zoomResetMouse.containsMouse
                    ToolTip.text: qsTr("Click to toggle 100% / Fit")

                    MouseArea {
                        id: zoomResetMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (Math.abs(sessionController.viewerZoom - 1.0) < 0.05) {
                                sessionController.setViewerZoom(2.0);
                            } else {
                                sessionController.resetViewerZoom();
                                root.panX = 0;
                                root.panY = 0;
                            }
                        }
                    }
                }

                // Zoom In Button
                Rectangle {
                    width: 30
                    height: 30
                    radius: 15
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
                        onClicked: sessionController.zoomIn(0.25)
                    }
                }

                // Fit to Screen Button
                Rectangle {
                    width: fitText.implicitWidth + 12
                    height: 24
                    radius: 12
                    anchors.verticalCenter: parent.verticalCenter
                    color: (Math.abs(sessionController.viewerZoom - 1.0) < 0.05)
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
                    ToolTip.text: qsTr("Fit Photo to Screen (0)")

                    MouseArea {
                        id: fitBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            sessionController.resetViewerZoom();
                            root.panX = 0;
                            root.panY = 0;
                        }
                    }
                }

                // 1:1 Actual Pixels Button
                Rectangle {
                    width: actualText.implicitWidth + 12
                    height: 24
                    radius: 12
                    anchors.verticalCenter: parent.verticalCenter
                    color: (Math.abs(sessionController.viewerZoom - 2.0) < 0.05)
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
                    ToolTip.text: qsTr("Actual Size / 100% Pixels (1)")

                    MouseArea {
                        id: actualBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            sessionController.setViewerZoom(2.0);
                        }
                    }
                }

                // Vertical Divider
                Rectangle {
                    width: 1
                    height: 18
                    anchors.verticalCenter: parent.verticalCenter
                    color: "#2e364a"
                }

                // Rotate Left 90°
                Rectangle {
                    width: 30
                    height: 30
                    radius: 15
                    color: rotLeftMouse.containsMouse ? "#272e40" : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "↺"
                        font.pixelSize: 14
                        color: "#f1f5f9"
                    }

                    ToolTip.visible: rotLeftMouse.containsMouse
                    ToolTip.text: qsTr("Rotate Left 90° ([)")

                    MouseArea {
                        id: rotLeftMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.rotateCounterClockwise()
                    }
                }

                // Rotate Right 90°
                Rectangle {
                    width: 30
                    height: 30
                    radius: 15
                    color: rotRightMouse.containsMouse ? "#272e40" : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "↻"
                        font.pixelSize: 14
                        color: "#f1f5f9"
                    }

                    ToolTip.visible: rotRightMouse.containsMouse
                    ToolTip.text: qsTr("Rotate Right 90° (])")

                    MouseArea {
                        id: rotRightMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.rotateClockwise()
                    }
                }

                // Vertical Divider
                Rectangle {
                    width: 1
                    height: 18
                    anchors.verticalCenter: parent.verticalCenter
                    color: "#2e364a"
                }

                // Toggle Filmstrip Button
                Rectangle {
                    width: 30
                    height: 30
                    radius: 15
                    color: root.filmstripVisible ? (filmBtnMouse.containsMouse ? "#272e40" : "#1e2434") : (filmBtnMouse.containsMouse ? "#272e40" : "transparent")

                    Text {
                        anchors.centerIn: parent
                        text: "🎞"
                        font.pixelSize: 12
                        color: root.filmstripVisible ? "#38bdf8" : "#94a3b8"
                    }

                    ToolTip.visible: filmBtnMouse.containsMouse
                    ToolTip.text: root.filmstripVisible ? qsTr("Hide Filmstrip (T)") : qsTr("Show Filmstrip (T)")

                    MouseArea {
                        id: filmBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.filmstripVisible = !root.filmstripVisible
                    }
                }

                // Toggle Fullscreen Button
                Rectangle {
                    width: 30
                    height: 30
                    radius: 15
                    color: sessionController.isFullscreen ? (fsBtnMouse.containsMouse ? "#272e40" : "#1e2434") : (fsBtnMouse.containsMouse ? "#272e40" : "transparent")

                    Text {
                        anchors.centerIn: parent
                        text: "⛶"
                        font.pixelSize: 13
                        color: sessionController.isFullscreen ? "#38bdf8" : "#f1f5f9"
                    }

                    ToolTip.visible: fsBtnMouse.containsMouse
                    ToolTip.text: sessionController.isFullscreen ? qsTr("Exit Fullscreen (F / F11)") : qsTr("Fullscreen View (F / F11)")

                    MouseArea {
                        id: fsBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.toggleFullscreen()
                    }
                }
            }
        }
    }

    // ==========================================
    // BOTTOM FILMSTRIP MINI-BAR
    // ==========================================
    Rectangle {
        id: filmstripBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.filmstripVisible ? 82 : 0
        visible: root.filmstripVisible
        color: "#12141c"
        border.color: "#222634"
        border.width: 1
        clip: true
        z: 20

        Behavior on height {
            NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
        }

        ListView {
            id: filmstripList
            anchors.fill: parent
            anchors.margins: 8
            orientation: ListView.Horizontal
            spacing: 8
            clip: true
            model: filterModel

            delegate: Rectangle {
                id: stripThumb
                width: 78
                height: 66
                radius: 4
                color: "#1a1d26"
                border.color: (selectionManager.currentIndex === index) ? themeManager.accent : (stripMouse.containsMouse ? "#4b5563" : "#2d3345")
                border.width: (selectionManager.currentIndex === index) ? 2 : 1
                clip: true

                Image {
                    anchors.fill: parent
                    anchors.margins: 2
                    source: model.thumbnailUrl
                    fillMode: Image.PreserveAspectCrop
                    sourceSize.width: 160
                    sourceSize.height: 120
                    asynchronous: true
                }

                MouseArea {
                    id: stripMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: selectionManager.select(index)
                }
            }

            ScrollBar.horizontal: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            Connections {
                target: selectionManager
                function onSelectionChanged() {
                    if (selectionManager.currentIndex >= 0 && !filmstripList.moving) {
                        filmstripList.positionViewAtIndex(selectionManager.currentIndex, ListView.Contain);
                    }
                }
            }
        }
    }

    // ==========================================
    // KEYBOARD SHORTCUTS & NAVIGATION
    // ==========================================
    Keys.onEscapePressed: {
        if (sessionController.isFullscreen) {
            sessionController.setFullscreen(false);
        } else {
            sessionController.setPreviewMode(false);
        }
    }
    Keys.onLeftPressed: selectionManager.selectPrevious()
    Keys.onRightPressed: selectionManager.selectNext()
    Keys.onSpacePressed: sessionController.toggleItemFlagged(selectionManager.currentIndex)

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_A) {
            selectionManager.selectPrevious();
            event.accepted = true;
        } else if (event.key === Qt.Key_D) {
            selectionManager.selectNext();
            event.accepted = true;
        } else if (event.key === Qt.Key_Home) {
            selectionManager.selectFirst();
            event.accepted = true;
        } else if (event.key === Qt.Key_End) {
            selectionManager.selectLast();
            event.accepted = true;
        } else if (event.key === Qt.Key_F || event.key === Qt.Key_F11) {
            sessionController.toggleFullscreen();
            event.accepted = true;
        } else if (event.key === Qt.Key_0) {
            sessionController.resetViewerZoom();
            root.panX = 0;
            root.panY = 0;
            event.accepted = true;
        } else if (event.key === Qt.Key_1) {
            sessionController.setViewerZoom(2.0);
            event.accepted = true;
        } else if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
            sessionController.zoomIn(0.25);
            event.accepted = true;
        } else if (event.key === Qt.Key_Minus || event.key === Qt.Key_Underscore) {
            sessionController.zoomOut(0.25);
            event.accepted = true;
        } else if (event.key === Qt.Key_BracketLeft) {
            sessionController.rotateCounterClockwise();
            event.accepted = true;
        } else if (event.key === Qt.Key_BracketRight) {
            sessionController.rotateClockwise();
            event.accepted = true;
        } else if (event.key === Qt.Key_T) {
            root.filmstripVisible = !root.filmstripVisible;
            event.accepted = true;
        } else if (event.key === Qt.Key_H) {
            sessionController.toggleHistogramVisible();
            event.accepted = true;
        } else if (event.key >= Qt.Key_1 && event.key <= Qt.Key_5 && (event.modifiers & Qt.ControlModifier)) {
            sessionController.setItemRating(selectionManager.currentIndex, event.key - Qt.Key_0);
            event.accepted = true;
        }
    }
}
