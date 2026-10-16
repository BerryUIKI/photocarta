// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Item {
    id: root

    signal requestOpenFiles()

    readonly property real baseWidth: 220 * sessionController.zoomFactor
    readonly property real baseHeight: 200 * sessionController.zoomFactor
    readonly property int columns: Math.max(1, Math.floor(width / baseWidth))
    readonly property real dynamicCellWidth: Math.floor(width / columns)
    readonly property real dynamicCellHeight: Math.floor(dynamicCellWidth * (baseHeight / baseWidth))

    GridView {
        id: grid
        anchors.fill: parent
        anchors.margins: 8
        clip: true
        focus: !sessionController.previewMode

        model: filterModel
        delegate: ThumbnailCard {}

        cellWidth: root.dynamicCellWidth
        cellHeight: root.dynamicCellHeight

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        // Keyboard Navigation Support
        Keys.onRightPressed: selectionManager.selectNext()
        Keys.onLeftPressed: selectionManager.selectPrevious()
        Keys.onDownPressed: {
            const nextIdx = selectionManager.currentIndex + root.columns
            if (nextIdx < filterModel.count) {
                selectionManager.select(nextIdx)
            } else {
                selectionManager.selectLast()
            }
        }
        Keys.onUpPressed: {
            const prevIdx = selectionManager.currentIndex - root.columns
            if (prevIdx >= 0) {
                selectionManager.select(prevIdx)
            } else {
                selectionManager.selectFirst()
            }
        }
        Keys.onReturnPressed: {
            if (selectionManager.hasSelection) {
                sessionController.setPreviewMode(true)
            }
        }
        Keys.onSpacePressed: {
            if (selectionManager.currentIndex >= 0) {
                selectionManager.toggleSelect(selectionManager.currentIndex)
            }
        }
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Home) {
                selectionManager.selectFirst();
                event.accepted = true;
            } else if (event.key === Qt.Key_End) {
                selectionManager.selectLast();
                event.accepted = true;
            }
        }

        // Keep active item in visible view
        Connections {
            target: selectionManager
            function onSelectionChanged() {
                if (selectionManager.currentIndex >= 0 && !grid.moving) {
                    grid.positionViewAtIndex(selectionManager.currentIndex, GridView.Contain)
                }
            }
        }
    }

    // Contextual Empty State
    EmptyStateView {
        visible: filterModel.count === 0
        onRequestOpenFiles: root.requestOpenFiles()
    }
}
