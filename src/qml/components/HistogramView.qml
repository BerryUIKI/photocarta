// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property var histogram: sessionController.currentHistogram
    property string activeChannel: sessionController.histogramChannel // "rgb", "luma", "red", "green", "blue"
    property bool logScale: sessionController.histogramLogScale
    property bool compact: false

    implicitWidth: 260
    implicitHeight: compact ? 120 : 160
    radius: themeManager.radiusMd
    color: "#0e111a"
    border.color: "#222738"
    border.width: 1
    clip: true

    readonly property bool hasData: !!root.histogram && root.histogram.valid === true
    readonly property real shadowClip: hasData ? (root.histogram.clippedShadowsPercent || 0.0) : 0.0
    readonly property real highlightClip: hasData ? (root.histogram.clippedHighlightsPercent || 0.0) : 0.0
    readonly property real meanLuma: hasData ? (root.histogram.meanLuminance || 0.0) : 0.0

    // Redraw canvas whenever histogram data or settings change
    onHistogramChanged: histCanvas.requestPaint()
    onActiveChannelChanged: histCanvas.requestPaint()
    onLogScaleChanged: histCanvas.requestPaint()

    Column {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // Header: Channel Selector & Scale Toggle
        Row {
            width: parent.width

            // Channel Selector Pills
            Row {
                spacing: 2
                anchors.verticalCenter: parent.verticalCenter

                Repeater {
                    model: [
                        { id: "rgb", label: qsTr("RGB") },
                        { id: "luma", label: qsTr("Luma") },
                        { id: "red", label: qsTr("R") },
                        { id: "green", label: qsTr("G") },
                        { id: "blue", label: qsTr("B") }
                    ]

                    Rectangle {
                        width: chText.implicitWidth + 8
                        height: 20
                        radius: 4
                        color: root.activeChannel === modelData.id
                               ? (modelData.id === "red" ? "#dc2626" : (modelData.id === "green" ? "#16a34a" : (modelData.id === "blue" ? "#2563eb" : themeManager.accent)))
                               : (chMouse.containsMouse ? "#1e2436" : "transparent")

                        Text {
                            id: chText
                            anchors.centerIn: parent
                            text: modelData.label
                            font.pixelSize: 10
                            font.weight: root.activeChannel === modelData.id ? Font.Bold : Font.Normal
                            color: root.activeChannel === modelData.id ? "#ffffff" : "#94a3b8"
                        }

                        MouseArea {
                            id: chMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sessionController.setHistogramChannel(modelData.id)
                        }
                    }
                }
            }

            Item {
                width: 1
                height: 1
                Layout.fillWidth: true
            }

            // Linear / Logarithmic Scale Toggle Button
            Rectangle {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: scaleText.implicitWidth + 8
                height: 20
                radius: 4
                color: root.logScale ? "#334155" : (scaleMouse.containsMouse ? "#1e2436" : "transparent")
                border.color: "#334155"
                border.width: 1

                Text {
                    id: scaleText
                    anchors.centerIn: parent
                    text: root.logScale ? qsTr("Log") : qsTr("Lin")
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    color: root.logScale ? "#38bdf8" : "#94a3b8"
                }

                ToolTip.visible: scaleMouse.containsMouse
                ToolTip.text: root.logScale ? qsTr("Logarithmic vertical scale (click for Linear)") : qsTr("Linear vertical scale (click for Log)")

                MouseArea {
                    id: scaleMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionController.toggleHistogramLogScale()
                }
            }
        }

        // Main Drawing Canvas
        Canvas {
            id: histCanvas
            width: parent.width
            height: parent.height - 48
            antialiasing: true

            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();
                ctx.clearRect(0, 0, width, height);

                // Grid background
                ctx.lineWidth = 1;
                ctx.strokeStyle = "#171c2b";

                // Vertical quarter grid lines
                for (var q = 1; q < 4; ++q) {
                    var gx = Math.round(width * (q / 4.0));
                    ctx.beginPath();
                    ctx.moveTo(gx, 0);
                    ctx.lineTo(gx, height);
                    ctx.stroke();
                }

                // Horizontal mid grid line
                ctx.beginPath();
                ctx.moveTo(0, Math.round(height * 0.5));
                ctx.lineTo(width, Math.round(height * 0.5));
                ctx.stroke();

                if (!root.hasData) {
                    return;
                }

                function transformVal(v) {
                    if (v <= 0) return 0.0;
                    if (root.logScale) {
                        return Math.min(1.0, Math.log(1.0 + 31.0 * v) / Math.log(32.0));
                    }
                    return Math.min(1.0, v);
                }

                function drawCurve(binArray, strokeColor, fillColor) {
                    if (!binArray || binArray.length < 2) return;
                    var step = width / (binArray.length - 1);

                    ctx.beginPath();
                    ctx.moveTo(0, height);

                    for (var i = 0; i < binArray.length; ++i) {
                        var val = transformVal(binArray[i]);
                        var y = height - (val * (height - 2));
                        ctx.lineTo(i * step, y);
                    }

                    ctx.lineTo(width, height);
                    ctx.closePath();

                    if (fillColor) {
                        ctx.fillStyle = fillColor;
                        ctx.fill();
                    }

                    ctx.beginPath();
                    for (var j = 0; j < binArray.length; ++j) {
                        var v2 = transformVal(binArray[j]);
                        var y2 = height - (v2 * (height - 2));
                        if (j === 0) ctx.moveTo(0, y2);
                        else ctx.lineTo(j * step, y2);
                    }
                    ctx.strokeStyle = strokeColor;
                    ctx.lineWidth = 1.5;
                    ctx.stroke();
                }

                if (root.activeChannel === "rgb") {
                    ctx.globalCompositeOperation = "screen";
                    drawCurve(root.histogram.red, "#ef4444", "#40ef4444");
                    drawCurve(root.histogram.green, "#22c55e", "#4022c55e");
                    drawCurve(root.histogram.blue, "#3b82f6", "#403b82f6");
                    ctx.globalCompositeOperation = "source-over";
                } else if (root.activeChannel === "luma") {
                    drawCurve(root.histogram.luminance, "#f8fafc", "#33f8fafc");
                } else if (root.activeChannel === "red") {
                    drawCurve(root.histogram.red, "#ef4444", "#33ef4444");
                } else if (root.activeChannel === "green") {
                    drawCurve(root.histogram.green, "#22c55e", "#3322c55e");
                } else if (root.activeChannel === "blue") {
                    drawCurve(root.histogram.blue, "#3b82f6", "#333b82f6");
                }
            }

            // Placeholder Text if calculating or empty
            Text {
                anchors.centerIn: parent
                visible: !root.hasData
                text: qsTr("No histogram data")
                font.pixelSize: 10
                color: "#64748b"
            }
        }

        // Footer: Clipping Indicators & Exposure Stats
        Row {
            width: parent.width
            spacing: 8

            // Shadow Clipping (Blacks)
            Row {
                spacing: 4
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    width: 7
                    height: 7
                    radius: 3.5
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.shadowClip > 0.05 ? "#38bdf8" : "#334155"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.hasData ? qsTr("Shadows: %1%").arg(root.shadowClip.toFixed(1)) : qsTr("Shadows: -")
                    font.pixelSize: 9
                    color: root.shadowClip > 0.05 ? "#38bdf8" : "#64748b"
                }
            }

            Item {
                width: 1
                height: 1
                Layout.fillWidth: true
            }

            // Mean Luminance
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: !root.compact
                text: root.hasData ? qsTr("Mean: %1%").arg(Math.round(root.meanLuma * 100)) : ""
                font.pixelSize: 9
                color: "#94a3b8"
            }

            Item {
                width: 1
                height: 1
                Layout.fillWidth: true
            }

            // Highlight Clipping (Whites)
            Row {
                spacing: 4
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.hasData ? qsTr("Highlights: %1%").arg(root.highlightClip.toFixed(1)) : qsTr("Highlights: -")
                    font.pixelSize: 9
                    color: root.highlightClip > 0.05 ? "#ef4444" : "#64748b"
                }

                Rectangle {
                    width: 7
                    height: 7
                    radius: 3.5
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.highlightClip > 0.05 ? "#ef4444" : "#334155"
                }
            }
        }
    }
}
