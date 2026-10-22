// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtCore
import "components"
import "styles"

ApplicationWindow {
    id: window

    title: qsTr("PhotoCarta — Photo Management")
    width: settingsManager ? settingsManager.windowWidth : 1280
    height: settingsManager ? settingsManager.windowHeight : 800
    minimumWidth: 840
    minimumHeight: 560
    visible: true
    visibility: sessionController.isFullscreen
                ? Window.FullScreen
                : (settingsManager && settingsManager.windowMaximized ? Window.Maximized : Window.Windowed)
    color: themeManager.background

    onClosing: {
        if (settingsManager && !sessionController.isFullscreen) {
            settingsManager.saveWindowGeometry(x, y, width, height, visibility === Window.Maximized)
        }
    }

    header: AppToolbar {
        id: appToolbar
        visible: !sessionController.isFullscreen
        onRequestOpenFiles: openDialog.open()
        onRequestScanFolder: scanFolderDialog.open()
    }

    footer: AppStatusBar {
        id: appStatusBar
        visible: !sessionController.isFullscreen
    }

    // Top Error / Warning notification banner
    ErrorBanner {
        id: errorBanner
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        visible: sessionController.hasError && !sessionController.isFullscreen
        z: 20
    }

    // Main 3-Pane Responsive Layout
    Item {
        anchors.top: (errorBanner.visible ? errorBanner.bottom : parent.top)
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        // Left: Collections / Navigation Sidebar
        CollectionNavigationPane {
            id: leftPane
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            visible: sessionController.leftPaneVisible && !sessionController.isFullscreen
            onRequestScanFolder: scanFolderDialog.open()
        }

        // Right: Metadata & Details Sidebar
        MetadataDetailsPane {
            id: rightPane
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            visible: sessionController.rightPaneVisible && !sessionController.isFullscreen
        }

        // Center: Content Canvas (Grid or Preview Mode)
        Item {
            id: centerContent
            anchors.left: leftPane.visible ? leftPane.right : parent.left
            anchors.right: rightPane.visible ? rightPane.left : parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            clip: true

            ThumbnailGridView {
                id: thumbnailGrid
                anchors.fill: parent
                visible: !sessionController.previewMode && !sessionController.compareMode
                onRequestOpenFiles: openDialog.open()
            }

            ImagePreviewView {
                anchors.fill: parent
                visible: sessionController.previewMode && !sessionController.compareMode
            }

            PhotoCompareView {
                anchors.fill: parent
                visible: sessionController.compareMode
            }
        }
    }

    // Modal Export Dialog
    ExportDialog {
        id: exportDialog
        onClosed: thumbnailGrid.focusGrid()
    }

    // Modal Batch Rename Dialog
    BatchRenameDialog {
        id: batchRenameDialog
        onClosed: thumbnailGrid.focusGrid()
    }

    // Modal Move-to-Trash Confirmation Dialog
    TrashConfirmDialog {
        id: trashConfirmDialog
        onClosed: thumbnailGrid.focusGrid()
    }

    // Real Native File Dialog for Temporary Session
    FileDialog {
        id: openDialog
        title: qsTr("Open Local Photos into Temporary Session")
        currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("Supported Photo Formats (*.jpg *.jpeg *.png *.webp *.tiff *.tif *.bmp *.svg)"),
            qsTr("JPEG Images (*.jpg *.jpeg)"),
            qsTr("PNG Images (*.png)"),
            qsTr("WebP Images (*.webp)"),
            qsTr("All Files (*)")
        ]
        onAccepted: {
            sessionController.openLocalFiles(selectedFiles)
        }
    }

    // Native Directory Dialog for Recursive Catalog Scanning
    FolderDialog {
        id: scanFolderDialog
        title: qsTr("Scan Photo Directory into Catalog")
        currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        onAccepted: {
            sessionController.scanFolder(selectedFolder)
        }
    }

    // Global Application Shortcuts
    Shortcut {
        sequence: "Ctrl+O"
        onActivated: openDialog.open()
    }

    Shortcut {
        sequence: "Ctrl+Shift+O"
        onActivated: scanFolderDialog.open()
    }

    Shortcut {
        sequence: "Ctrl+1"
        onActivated: sessionController.toggleLeftPane()
    }

    Shortcut {
        sequence: "Ctrl+2"
        onActivated: sessionController.toggleRightPane()
    }

    Shortcut {
        sequence: "Ctrl+D"
        onActivated: sessionController.loadDemoSamples()
    }

    Shortcut {
        sequence: "Ctrl+L"
        onActivated: themeManager.toggleTheme()
    }

    Shortcut {
        sequence: StandardKey.Find
        onActivated: appToolbar.focusSearch()
    }

    Shortcut {
        sequence: "F11"
        onActivated: sessionController.toggleFullscreen()
    }

    Shortcut {
        sequence: "Ctrl+E"
        onActivated: {
            if (selectionManager.hasSelection || selectionManager.selectedCount > 0) {
                sessionController.openExportDialog()
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+Shift+E"
        onActivated: {
            if (selectionManager.hasSelection || selectionManager.selectedCount > 0) {
                sessionController.openSelectedInEditor()
            }
        }
    }

    Shortcut {
        sequence: "F2"
        onActivated: {
            if (selectionManager.hasSelection || selectionManager.selectedCount > 0 || catalogModel.count > 0) {
                sessionController.openBatchRenameDialog()
            }
        }
    }

    Shortcut {
        sequence: StandardKey.SelectAll
        onActivated: {
            if (!sessionController.previewMode) {
                selectionManager.selectAll()
            }
        }
    }

    Shortcut {
        sequence: StandardKey.Copy
        onActivated: {
            if (selectionManager.hasSelection) {
                sessionController.copySelectedPathsToClipboard()
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+Alt+C"
        onActivated: {
            if (sessionController.compareMode) {
                sessionController.setCompareMode(false)
            } else if (selectionManager.selectedCount >= 2 || selectionManager.hasSelection) {
                sessionController.setCompareMode(true)
            }
        }
    }

    Shortcut {
        sequence: "C"
        onActivated: {
            if (!sessionController.previewMode) {
                if (sessionController.compareMode) {
                    sessionController.setCompareMode(false)
                } else if (selectionManager.selectedCount >= 2 || selectionManager.hasSelection) {
                    sessionController.setCompareMode(true)
                }
            }
        }
    }

    Shortcut {
        sequence: "Delete"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (!sessionController.previewMode && (selectionManager.hasSelection || selectionManager.selectedCount > 0)) {
                sessionController.openTrashDialog()
            }
        }
    }

    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (sessionController.isFullscreen) {
                sessionController.setFullscreen(false)
            } else if (sessionController.compareMode) {
                sessionController.setCompareMode(false)
            } else if (sessionController.previewMode) {
                sessionController.setPreviewMode(false)
            } else if (selectionManager.selectedCount > 1) {
                selectionManager.clearSelection()
            } else if (filterModel.searchQuery) {
                filterModel.searchQuery = ""
            }
        }
    }
}
