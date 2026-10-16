// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Row {
    id: root

    property int rating: 0
    property int maxStars: 5
    property int starSize: 16
    property bool interactive: false
    property color starColor: "#f59e0b"

    spacing: 3

    Repeater {
        model: root.maxStars

        Image {
            id: starIcon
            width: root.starSize
            height: root.starSize
            source: (index < root.rating)
                    ? "qrc:/assets/icons/star_filled.svg"
                    : "qrc:/assets/icons/star_outline.svg"
            sourceSize.width: root.starSize
            sourceSize.height: root.starSize
            fillMode: Image.PreserveAspectFit
            opacity: (index < root.rating) ? 1.0 : 0.35

            ToolTip.visible: starMouseArea.containsMouse
            ToolTip.text: root.interactive
                          ? qsTr("Rating: %1 / 5 (Prototype: Display-only fixture)").arg(root.rating)
                          : qsTr("Rating: %1 / 5").arg(root.rating)

            MouseArea {
                id: starMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: root.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: {
                    if (root.interactive) {
                        sessionController.setStatusMessage(
                            qsTr("Rating changed to %1 stars (Display-only fixture; write unpersisted)").arg(index + 1)
                        );
                    }
                }
            }
        }
    }
}
