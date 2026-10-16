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
    Q_INVOKABLE void setItemTags(int proxyIndex, const QStringList &tags);
    Q_INVOKABLE void loadDatabaseCatalog(int collectionId = 0);
    Q_INVOKABLE void selectCollection(int collectionId);
    Q_INVOKABLE void rescanCollection(int collectionId);
    Q_INVOKABLE void removeCollection(int collectionId);
    Q_INVOKABLE void toggleCollectionAvailable(int collectionId);
    Q_INVOKABLE void setCollectionAvailable(const QString &rootPath, bool available);
    Q_INVOKABLE void clearSession();
    Q_INVOKABLE void dismissError();
    Q_INVOKABLE void showError(const QString &message);
    Q_INVOKABLE bool captureScreenshot(QObject *windowObject, const QString &filePath);

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
    void screenshotCompleted(const QString &path, bool success);

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
};

} // namespace PhotoCarta
