// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "catalogfiltermodel.h"
#include "catalogmodel.h"
#include "selectionmanager.h"
#include "database/catalogdatabase.h"
#include "ingestion/directoryscanner.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

class QQuickWindow;

namespace PhotoCarta
{

class ImageExporter;
class HistogramCalculator;

class SessionController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool previewMode READ previewMode WRITE setPreviewMode NOTIFY previewModeChanged)
    Q_PROPERTY(bool leftPaneVisible READ leftPaneVisible WRITE setLeftPaneVisible NOTIFY leftPaneVisibleChanged)
    Q_PROPERTY(bool rightPaneVisible READ rightPaneVisible WRITE setRightPaneVisible NOTIFY rightPaneVisibleChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorStateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorStateChanged)
    Q_PROPERTY(double zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)
    Q_PROPERTY(bool isEphemeralSession READ isEphemeralSession NOTIFY databaseStateChanged)
    Q_PROPERTY(QString sessionTypeDescription READ sessionTypeDescription NOTIFY sessionTypeChanged)
    Q_PROPERTY(bool isScanning READ isScanning NOTIFY isScanningChanged)
    Q_PROPERTY(int scanProgress READ scanProgress NOTIFY scanProgressChanged)
    Q_PROPERTY(bool hasDatabase READ hasDatabase NOTIFY databaseStateChanged)
    Q_PROPERTY(int databaseItemCount READ databaseItemCount NOTIFY databaseStateChanged)
    Q_PROPERTY(int currentCollectionId READ currentCollectionId NOTIFY currentCollectionChanged)
    Q_PROPERTY(double viewerZoom READ viewerZoom WRITE setViewerZoom NOTIFY viewerZoomChanged)
    Q_PROPERTY(int viewerRotation READ viewerRotation WRITE setViewerRotation NOTIFY viewerRotationChanged)
    Q_PROPERTY(bool isFullscreen READ isFullscreen WRITE setFullscreen NOTIFY fullscreenChanged)
    Q_PROPERTY(bool isExporting READ isExporting NOTIFY exportProgressChanged)
    Q_PROPERTY(int exportProgress READ exportProgress NOTIFY exportProgressChanged)
    Q_PROPERTY(bool exportDialogOpen READ exportDialogOpen WRITE setExportDialogOpen NOTIFY exportDialogOpenChanged)
    Q_PROPERTY(bool compareMode READ compareMode WRITE setCompareMode NOTIFY compareModeChanged)
    Q_PROPERTY(bool syncZoomPan READ syncZoomPan WRITE setSyncZoomPan NOTIFY syncZoomPanChanged)
    Q_PROPERTY(QVariantList compareItems READ compareItems NOTIFY compareItemsChanged)
    Q_PROPERTY(int compareCount READ compareCount NOTIFY compareItemsChanged)
    Q_PROPERTY(double compareZoom READ compareZoom WRITE setCompareZoom NOTIFY compareZoomChanged)
    Q_PROPERTY(double comparePanX READ comparePanX WRITE setComparePanX NOTIFY comparePanChanged)
    Q_PROPERTY(double comparePanY READ comparePanY WRITE setComparePanY NOTIFY comparePanChanged)
    Q_PROPERTY(int activeCompareSlot READ activeCompareSlot WRITE setActiveCompareSlot NOTIFY activeCompareSlotChanged)
    Q_PROPERTY(QString winnerId READ winnerId NOTIFY winnerIdChanged)
    Q_PROPERTY(QVariantMap currentHistogram READ currentHistogram NOTIFY currentHistogramChanged)
    Q_PROPERTY(QString histogramChannel READ histogramChannel WRITE setHistogramChannel NOTIFY histogramChannelChanged)
    Q_PROPERTY(bool histogramLogScale READ histogramLogScale WRITE setHistogramLogScale NOTIFY histogramLogScaleChanged)
    Q_PROPERTY(bool histogramVisible READ histogramVisible WRITE setHistogramVisible NOTIFY histogramVisibleChanged)

public:
    explicit SessionController(CatalogModel *model,
                               CatalogFilterModel *filterModel,
                               SelectionManager *selectionManager,
                               QObject *parent = nullptr);
    ~SessionController() override;

    [[nodiscard]] bool previewMode() const;
    Q_INVOKABLE void setPreviewMode(bool active);

    [[nodiscard]] bool leftPaneVisible() const;
    void setLeftPaneVisible(bool visible);

    [[nodiscard]] bool rightPaneVisible() const;
    void setRightPaneVisible(bool visible);

    [[nodiscard]] QString statusMessage() const;
    void setStatusMessage(const QString &msg);

    [[nodiscard]] bool hasError() const;
    [[nodiscard]] QString errorMessage() const;

    [[nodiscard]] double zoomFactor() const;
    void setZoomFactor(double factor);

    [[nodiscard]] bool isEphemeralSession() const;
    [[nodiscard]] QString sessionTypeDescription() const;

    [[nodiscard]] bool isScanning() const;
    [[nodiscard]] int scanProgress() const;
    [[nodiscard]] bool hasDatabase() const;
    [[nodiscard]] int databaseItemCount() const;
    [[nodiscard]] int currentCollectionId() const;

    void setDatabase(CatalogDatabase *database);
    [[nodiscard]] CatalogDatabase *database() const;
    [[nodiscard]] DirectoryScanner *scanner() const;

    Q_INVOKABLE void togglePreviewMode();
    Q_INVOKABLE void toggleLeftPane();
    Q_INVOKABLE void toggleRightPane();
    Q_INVOKABLE void loadDemoSamples();
    Q_INVOKABLE void openLocalFiles(const QList<QUrl> &urls);
    Q_INVOKABLE void scanFolder(const QUrl &folderUrl);
    Q_INVOKABLE void scanDirectory(const QString &directoryPath, bool recursive = true);
    Q_INVOKABLE void cancelScan();
    Q_INVOKABLE void setItemRating(int proxyIndex, int rating);
    Q_INVOKABLE void toggleItemFlagged(int proxyIndex);
    Q_INVOKABLE void setItemTitle(int proxyIndex, const QString &title);
    Q_INVOKABLE void setItemCaption(int proxyIndex, const QString &caption);
    Q_INVOKABLE void setItemColorLabel(int proxyIndex, int colorLabel);
    Q_INVOKABLE void setItemTags(int proxyIndex, const QStringList &tags);
    Q_INVOKABLE void addTagToItem(int proxyIndex, const QString &tag);
    Q_INVOKABLE void removeTagFromItem(int proxyIndex, const QString &tag);
    Q_INVOKABLE void setTagFilter(const QString &tag);
    Q_INVOKABLE void clearTagFilter();
    Q_INVOKABLE void setRatingFilter(int minRating);
    Q_INVOKABLE void clearRatingFilter();
    Q_INVOKABLE void setColorLabelFilter(int colorLabel);
    Q_INVOKABLE void clearColorLabelFilter();
    Q_INVOKABLE void clearAllFilters();
    Q_INVOKABLE void setTitleForSelected(const QString &title);
    Q_INVOKABLE void setCaptionForSelected(const QString &caption);
    Q_INVOKABLE void setColorLabelForSelected(int colorLabel);
    Q_INVOKABLE void setRatingForSelected(int rating);
    Q_INVOKABLE void setFlaggedForSelected(bool flagged);
    Q_INVOKABLE void toggleFlaggedForSelected();
    Q_INVOKABLE void addTagToSelected(const QString &tag);
    Q_INVOKABLE void removeTagFromSelected(const QString &tag);
    Q_INVOKABLE void copySelectedPathsToClipboard();
    Q_INVOKABLE void revealInFileManager(int proxyIndex = -1);
    Q_INVOKABLE void loadDatabaseCatalog(int collectionId = 0);
    Q_INVOKABLE void openDatabase(const QString &databasePath);
    Q_INVOKABLE void importDigikamDatabase(const QString &databasePath);
    Q_INVOKABLE void selectCollection(int collectionId);
    Q_INVOKABLE void rescanCollection(int collectionId);
    Q_INVOKABLE void removeCollection(int collectionId);
    Q_INVOKABLE void toggleCollectionAvailable(int collectionId);
    Q_INVOKABLE void setCollectionAvailable(const QString &rootPath, bool available);
    Q_INVOKABLE void clearSession();
    Q_INVOKABLE void dismissError();
    Q_INVOKABLE void showError(const QString &message);
    Q_INVOKABLE bool captureScreenshot(QObject *windowObject, const QString &filePath);

    // Image Viewer Zoom & Rotation
    [[nodiscard]] double viewerZoom() const;
    Q_INVOKABLE void setViewerZoom(double zoom);
    Q_INVOKABLE void zoomIn(double step = 0.25);
    Q_INVOKABLE void zoomOut(double step = 0.25);
    Q_INVOKABLE void resetViewerZoom();

    [[nodiscard]] int viewerRotation() const;
    Q_INVOKABLE void setViewerRotation(int degrees);
    Q_INVOKABLE void rotateClockwise();
    Q_INVOKABLE void rotateCounterClockwise();
    Q_INVOKABLE void resetViewerRotation();

    [[nodiscard]] bool isFullscreen() const;
    Q_INVOKABLE void setFullscreen(bool fullscreen);
    Q_INVOKABLE void toggleFullscreen();

    // Batch Photo Export
    [[nodiscard]] bool isExporting() const;
    [[nodiscard]] int exportProgress() const;
    [[nodiscard]] int exportTotal() const;
    [[nodiscard]] bool exportDialogOpen() const;
    void setExportDialogOpen(bool open);

    Q_INVOKABLE void openExportDialog();
    Q_INVOKABLE void closeExportDialog();
    Q_INVOKABLE void startExportSelected(const QVariantMap &options);
    Q_INVOKABLE void cancelExport();
    Q_INVOKABLE QString defaultExportDirectory() const;

    // Side-by-Side Photo Comparison (Light Table)
    [[nodiscard]] bool compareMode() const;
    Q_INVOKABLE void setCompareMode(bool active);
    Q_INVOKABLE void toggleCompareMode();

    [[nodiscard]] bool syncZoomPan() const;
    Q_INVOKABLE void setSyncZoomPan(bool sync);
    Q_INVOKABLE void toggleSyncZoomPan();

    [[nodiscard]] QVariantList compareItems() const;
    [[nodiscard]] int compareCount() const;

    [[nodiscard]] double compareZoom() const;
    Q_INVOKABLE void setCompareZoom(double zoom);
    Q_INVOKABLE void zoomInCompare(double step = 0.25);
    Q_INVOKABLE void zoomOutCompare(double step = 0.25);
    Q_INVOKABLE void resetCompareZoom();

    [[nodiscard]] double comparePanX() const;
    Q_INVOKABLE void setComparePanX(double x);

    [[nodiscard]] double comparePanY() const;
    Q_INVOKABLE void setComparePanY(double y);
    Q_INVOKABLE void setComparePan(double x, double y);

    [[nodiscard]] int activeCompareSlot() const;
    Q_INVOKABLE void setActiveCompareSlot(int slot);

    [[nodiscard]] QString winnerId() const;
    Q_INVOKABLE void setAsWinner(int slotIndex);
    Q_INVOKABLE void clearWinner();

    Q_INVOKABLE void removeCompareItem(int slotIndex);
    Q_INVOKABLE void swapCompareItems(int slotA, int slotB);
    Q_INVOKABLE void setCompareRating(int slotIndex, int rating);
    Q_INVOKABLE void toggleCompareFlagged(int slotIndex);
    Q_INVOKABLE void setCompareColorLabel(int slotIndex, int colorLabel);
    Q_INVOKABLE void addAdjacentToCompare(bool next = true);

    // Image Histogram Service
    [[nodiscard]] QVariantMap currentHistogram() const;
    [[nodiscard]] QString histogramChannel() const;
    Q_INVOKABLE void setHistogramChannel(const QString &channel);
    [[nodiscard]] bool histogramLogScale() const;
    Q_INVOKABLE void setHistogramLogScale(bool logScale);
    Q_INVOKABLE void toggleHistogramLogScale();
    [[nodiscard]] bool histogramVisible() const;
    Q_INVOKABLE void setHistogramVisible(bool visible);
    Q_INVOKABLE void toggleHistogramVisible();
    Q_INVOKABLE void requestHistogram(const QString &filePath);
    [[nodiscard]] HistogramCalculator *histogramCalculator() const;

signals:
    void previewModeChanged();
    void leftPaneVisibleChanged();
    void rightPaneVisibleChanged();
    void statusMessageChanged();
    void errorStateChanged();
    void zoomFactorChanged();
    void sessionTypeChanged();
    void isScanningChanged();
    void scanProgressChanged();
    void databaseStateChanged();
    void currentCollectionChanged();
    void viewerZoomChanged();
    void viewerRotationChanged();
    void fullscreenChanged();
    void screenshotCompleted(const QString &path, bool success);
    void exportProgressChanged();
    void exportDialogOpenChanged();
    void exportCompleted(int successCount, int failCount, const QString &outputDir);
    void compareModeChanged();
    void syncZoomPanChanged();
    void compareItemsChanged();
    void compareZoomChanged();
    void comparePanChanged();
    void activeCompareSlotChanged();
    void winnerIdChanged();
    void currentHistogramChanged();
    void histogramChannelChanged();
    void histogramLogScaleChanged();
    void histogramVisibleChanged();

private slots:
    void onModelCountChanged();
    void onFileLoadError(const QString &fileName, const QString &errorMsg);
    void onBatchDiscovered(const QList<CatalogItem> &items);
    void onScanProgress(int count, const QString &currentFile);
    void onScanFinished(int totalFound, qint64 elapsedMs);
    void onScanCanceled();

private:
    [[nodiscard]] int sourceRowForProxy(int proxyIndex) const;

    CatalogModel *m_model = nullptr;
    CatalogFilterModel *m_filterModel = nullptr;
    SelectionManager *m_selectionManager = nullptr;
    CatalogDatabase *m_database = nullptr;
    DirectoryScanner *m_scanner = nullptr;

    bool m_previewMode = false;
    bool m_leftPaneVisible = true;
    bool m_rightPaneVisible = true;
    QString m_statusMessage = QStringLiteral("Ready • PhotoCarta");
    bool m_hasError = false;
    QString m_errorMessage;
    double m_zoomFactor = 1.0;
    int m_scanProgress = 0;
    int m_currentCollectionId = 0;
    double m_viewerZoom = 1.0;
    int m_viewerRotation = 0;
    bool m_isFullscreen = false;

    ImageExporter *m_exporter = nullptr;
    bool m_exportDialogOpen = false;

    // Side-by-side comparison (Light Table)
    bool m_compareMode = false;
    bool m_syncZoomPan = true;
    double m_compareZoom = 1.0;
    double m_comparePanX = 0.0;
    double m_comparePanY = 0.0;
    int m_activeCompareSlot = 0;
    QString m_winnerId;
    QList<int> m_compareProxyRows;

    // Image Histogram Service
    HistogramCalculator *m_histogramCalculator = nullptr;
    QVariantMap m_currentHistogram;
    QString m_histogramChannel = QStringLiteral("rgb");
    bool m_histogramLogScale = false;
    bool m_histogramVisible = true;
};

} // namespace PhotoCarta
