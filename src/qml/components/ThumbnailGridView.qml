// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Item {
    id: root

    signal requestOpenFiles()

    readonly property real availableGridWidth: Math.max(100, width - 16)
    readonly property real baseWidth: 220 * sessionController.zoomFactor
    readonly property int columns: Math.max(1, Math.floor(availableGridWidth / baseWidth))
    readonly property real dynamicCellWidth: Math.floor(availableGridWidth / columns)
    readonly property real dynamicCellHeight: Math.floor(dynamicCellWidth * 0.85)

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
                selectionManager.selectFirst()
                event.accepted = true
            } else if (event.key === Qt.Key_End) {
                selectionManager.selectLast()
                event.accepted = true
            } else if (event.key >= Qt.Key_0 && event.key <= Qt.Key_5 && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier))) {
                const rating = event.key - Qt.Key_0
                if (selectionManager.selectedCount > 1) {
                    sessionController.setRatingForSelected(rating)
                } else if (selectionManager.currentIndex >= 0) {
                    sessionController.setItemRating(selectionManager.currentIndex, rating)
                }
                event.accepted = true
            } else if (event.key >= Qt.Key_6 && event.key <= Qt.Key_9 && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier))) {
                const colorLabel = (event.key - Qt.Key_6) + 1 // 6->1 (Red), 7->2 (Yellow), 8->3 (Green), 9->4 (Blue)
                if (selectionManager.selectedCount > 1) {
                    sessionController.setColorLabelForSelected(colorLabel)
                } else if (selectionManager.currentIndex >= 0) {
                    sessionController.setItemColorLabel(selectionManager.currentIndex, colorLabel)
                }
                event.accepted = true
            } else if (event.key === Qt.Key_F && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier))) {
                if (selectionManager.selectedCount > 1) {
                    sessionController.toggleFlaggedForSelected()
                } else if (selectionManager.currentIndex >= 0) {
                    sessionController.toggleItemFlagged(selectionManager.currentIndex)
                }
                event.accepted = true
            } else if (event.key === Qt.Key_A && (event.modifiers & Qt.ControlModifier)) {
                selectionManager.selectAll()
                event.accepted = true
            } else if (event.key === Qt.Key_C && (event.modifiers & Qt.ControlModifier)) {
                sessionController.copySelectedPathsToClipboard()
                event.accepted = true
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
