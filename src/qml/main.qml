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
    width: 1280
    height: 800
    minimumWidth: 840
    minimumHeight: 560
    visible: true
    color: themeManager.background

    header: AppToolbar {
        id: appToolbar
        onRequestOpenFiles: openDialog.open()
    }

    footer: AppStatusBar {
        id: appStatusBar
    }

    // Top Error / Warning notification banner
    ErrorBanner {
        id: errorBanner
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        z: 20
    }

    // Main 3-Pane Responsive Layout
    Item {
        anchors.top: errorBanner.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        // Left: Collections / Navigation Sidebar
        CollectionNavigationPane {
            id: leftPane
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            visible: sessionController.leftPaneVisible
        }

        // Right: Metadata & Details Sidebar
        MetadataDetailsPane {
            id: rightPane
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            visible: sessionController.rightPaneVisible
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
                anchors.fill: parent
                visible: !sessionController.previewMode
                onRequestOpenFiles: openDialog.open()
            }

            ImagePreviewView {
                anchors.fill: parent
                visible: sessionController.previewMode
            }
        }
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

    // Global Application Shortcuts
    Shortcut {
        sequence: "Ctrl+O"
        onActivated: openDialog.open()
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
        sequence: "Escape"
        onActivated: {
            if (sessionController.previewMode) {
                sessionController.setPreviewMode(false)
            } else if (filterModel.searchQuery) {
                filterModel.searchQuery = ""
            }
        }
    }
}
