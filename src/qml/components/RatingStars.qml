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

    signal ratingSelected(int rating)

    spacing: 3

    Repeater {
        model: root.maxStars

        Item {
            id: starItem
            width: root.starSize
            height: root.starSize

            readonly property int starRating: index + 1

            activeFocusOnTab: root.interactive
            Accessible.ignored: !root.interactive
            Accessible.role: Accessible.RadioButton
            Accessible.name: qsTr("Rate %1 of %2 stars").arg(starItem.starRating).arg(root.maxStars)
            Accessible.description: qsTr("Current rating: %1 of %2. Activating the current rating clears it.")
                                        .arg(root.rating).arg(root.maxStars)
            Accessible.checkable: true
            Accessible.checked: root.rating === starItem.starRating
            Accessible.onPressAction: starItem.activate()
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    starItem.activate()
                    event.accepted = true
                }
            }

            function activate() {
                if (root.interactive) {
                    root.ratingSelected(starItem.starRating === root.rating ? 0 : starItem.starRating)
                }
            }

            Image {
                anchors.fill: parent
                source: (index < root.rating)
                        ? "qrc:/assets/icons/star_filled.svg"
                        : "qrc:/assets/icons/star_outline.svg"
                sourceSize.width: root.starSize
                sourceSize.height: root.starSize
                fillMode: Image.PreserveAspectFit
                opacity: (index < root.rating) ? 1.0 : 0.35
            }

            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: themeManager.borderFocus
                border.width: 1
                visible: starItem.activeFocus
                Accessible.ignored: true
            }

            ToolTip.visible: starMouseArea.containsMouse
            ToolTip.text: qsTr("Rating: %1 / 5").arg(root.rating)

            MouseArea {
                id: starMouseArea
                anchors.fill: parent
                hoverEnabled: root.interactive
                cursorShape: root.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: starItem.activate()
            }
        }
    }
}
