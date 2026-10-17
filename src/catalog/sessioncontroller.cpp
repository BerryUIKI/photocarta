// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sessioncontroller.h"

#include <algorithm>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QQuickWindow>

namespace PhotoCarta
{

SessionController::SessionController(CatalogModel *model,
                                     CatalogFilterModel *filterModel,
                                     SelectionManager *selectionManager,
                                     QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_filterModel(filterModel)
    , m_selectionManager(selectionManager)
    , m_scanner(new DirectoryScanner(this))
{
    if (m_model)
    {
        connect(m_model, &CatalogModel::countChanged, this, &SessionController::onModelCountChanged);
        connect(m_model, &CatalogModel::fileLoadError, this, &SessionController::onFileLoadError);
    }

    if (m_selectionManager)
    {
        connect(m_selectionManager, &SelectionManager::selectionChanged, this, [this]() {
            resetViewerZoom();
            resetViewerRotation();
        });
    }

    connect(m_scanner, &DirectoryScanner::scanStarted, this, [this](const QString &path) {
        m_scanProgress = 0;
        emit isScanningChanged();
        emit scanProgressChanged();
        setStatusMessage(QStringLiteral("Scanning directory: ") + path);
    });
    connect(m_scanner, &DirectoryScanner::batchDiscovered, this, &SessionController::onBatchDiscovered);
    connect(m_scanner, &DirectoryScanner::progress, this, &SessionController::onScanProgress);
    connect(m_scanner, &DirectoryScanner::scanFinished, this, &SessionController::onScanFinished);
    connect(m_scanner, &DirectoryScanner::scanCanceled, this, &SessionController::onScanCanceled);
}

SessionController::~SessionController() = default;

bool SessionController::previewMode() const
{
    return m_previewMode;
}

void SessionController::setPreviewMode(bool active)
{
    if (m_previewMode == active)
    {
        return;
    }
    m_previewMode = active;
    if (!active && m_isFullscreen)
    {
        setFullscreen(false);
    }
    resetViewerZoom();
    resetViewerRotation();
    emit previewModeChanged();
}

bool SessionController::leftPaneVisible() const
{
    return m_leftPaneVisible;
}

void SessionController::setLeftPaneVisible(bool visible)
{
    if (m_leftPaneVisible == visible)
    {
        return;
    }
    m_leftPaneVisible = visible;
    emit leftPaneVisibleChanged();
}

bool SessionController::rightPaneVisible() const
{
    return m_rightPaneVisible;
}

void SessionController::setRightPaneVisible(bool visible)
{
    if (m_rightPaneVisible == visible)
    {
        return;
    }
    m_rightPaneVisible = visible;
    emit rightPaneVisibleChanged();
}

QString SessionController::statusMessage() const
{
    return m_statusMessage;
}

void SessionController::setStatusMessage(const QString &msg)
{
    if (m_statusMessage == msg)
    {
        return;
    }
    m_statusMessage = msg;
    emit statusMessageChanged();
}

bool SessionController::hasError() const
{
    return m_hasError;
}

QString SessionController::errorMessage() const
{
    return m_errorMessage;
}

double SessionController::zoomFactor() const
{
    return m_zoomFactor;
}

void SessionController::setZoomFactor(double factor)
{
    const double clamped = qBound(0.6, factor, 2.0);
    if (qFuzzyCompare(m_zoomFactor, clamped))
    {
        return;
    }
    m_zoomFactor = clamped;
    emit zoomFactorChanged();
}

bool SessionController::isEphemeralSession() const
{
    return !(m_database && m_database->isOpen());
}

QString SessionController::sessionTypeDescription() const
{
    if (!m_model || m_model->count() == 0)
    {
        return QStringLiteral("Empty Session");
    }
    if (m_model->hasDemoContent())
    {
        return QStringLiteral("Demo Fixtures (Procedural SVGs - Display Only)");
    }
    if (m_database && m_database->isOpen())
    {
        return QStringLiteral("Persistent SQLite Catalog (%1 items)").arg(m_model->count());
    }
    return QStringLiteral("Temporary Local Photos (%1 items)").arg(m_model->count());
}

bool SessionController::isScanning() const
{
    return m_scanner && m_scanner->isScanning();
}

int SessionController::scanProgress() const
{
    return m_scanProgress;
}

bool SessionController::hasDatabase() const
{
    return m_database && m_database->isOpen();
}

int SessionController::databaseItemCount() const
{
    return (m_database && m_database->isOpen()) ? m_database->itemCount() : 0;
}

int SessionController::currentCollectionId() const
{
    return m_currentCollectionId;
}

void SessionController::setDatabase(CatalogDatabase *database)
{
    m_database = database;
    if (m_model)
    {
        m_model->setDatabase(database);
    }
    if (m_database)
    {
        connect(m_database, &CatalogDatabase::itemsChanged, this, &SessionController::databaseStateChanged);
        connect(m_database, &CatalogDatabase::collectionsChanged, this, &SessionController::databaseStateChanged);
    }
    emit databaseStateChanged();
    emit sessionTypeChanged();
}

CatalogDatabase *SessionController::database() const
{
    return m_database;
}

DirectoryScanner *SessionController::scanner() const
{
    return m_scanner;
}

void SessionController::togglePreviewMode()
{
    setPreviewMode(!m_previewMode);
}

void SessionController::toggleLeftPane()
{
    setLeftPaneVisible(!m_leftPaneVisible);
}

void SessionController::toggleRightPane()
{
    setRightPaneVisible(!m_rightPaneVisible);
}

void SessionController::loadDemoSamples()
{
    if (m_model)
    {
        m_model->loadDemoFixtures();
        if (m_selectionManager)
        {
            m_selectionManager->select(0);
        }
        setStatusMessage(QStringLiteral("Loaded 6 procedural demo fixtures"));
        emit sessionTypeChanged();
    }
}

void SessionController::openLocalFiles(const QList<QUrl> &urls)
{
    if (m_model)
    {
        m_model->openUrls(urls);
        setStatusMessage(QString::asprintf("Session updated: %d photos loaded", m_model->count()));
        emit sessionTypeChanged();
    }
}

void SessionController::scanFolder(const QUrl &folderUrl)
{
    if (folderUrl.isLocalFile())
    {
        scanDirectory(folderUrl.toLocalFile(), true);
    }
    else
    {
        scanDirectory(folderUrl.toString(QUrl::PreferLocalFile), true);
    }
}

void SessionController::scanDirectory(const QString &directoryPath, bool recursive)
{
    const QString cleaned = QDir::cleanPath(directoryPath);
    const QFileInfo fi(cleaned);
    if (!fi.exists() || !fi.isDir())
    {
        showError(QStringLiteral("Cannot scan directory: %1 does not exist").arg(cleaned));
        return;
    }

    if (m_model && m_model->hasDemoContent())
    {
        m_model->clearSession();
    }

    if (m_database && m_database->isOpen())
    {
        m_currentCollectionId = m_database->addOrGetCollection(cleaned, fi.fileName(), QStringLiteral("local"));
    }
    else
    {
        m_currentCollectionId = 0;
    }

    m_scanner->startScan(cleaned, recursive);
}

void SessionController::cancelScan()
{
    if (m_scanner)
    {
        m_scanner->cancel();
    }
}

int SessionController::sourceRowForProxy(int proxyIndex) const
{
    if (m_filterModel && proxyIndex >= 0)
    {
        return m_filterModel->mapToSourceRow(proxyIndex);
    }
    return proxyIndex;
}

void SessionController::setItemRating(int proxyIndex, int rating)
{
    const int srcRow = sourceRowForProxy(proxyIndex);
    if (m_model)
    {
        m_model->updateItemRating(srcRow, rating);
    }
}

void SessionController::toggleItemFlagged(int proxyIndex)
{
    const int srcRow = sourceRowForProxy(proxyIndex);
    if (m_model)
    {
        const CatalogItem *item = m_model->itemAt(srcRow);
        if (item)
        {
            m_model->updateItemFlagged(srcRow, !item->isFlagged);
        }
    }
}

void SessionController::setItemTags(int proxyIndex, const QStringList &tags)
{
    const int srcRow = sourceRowForProxy(proxyIndex);
    if (m_model)
    {
        m_model->updateItemTags(srcRow, tags);
    }
}

void SessionController::loadDatabaseCatalog(int collectionId)
{
    if (m_model)
    {
        m_model->loadFromDatabase(collectionId);
        if (m_selectionManager && m_model->count() > 0)
        {
            m_selectionManager->select(0);
        }
        setStatusMessage(QStringLiteral("Loaded %1 photos from catalog database").arg(m_model->count()));
        emit sessionTypeChanged();
    }
}

void SessionController::openDatabase(const QString &databasePath)
{
    if (!m_database)
    {
        return;
    }

    if (!m_database->open(databasePath))
    {
        showError(QStringLiteral("Failed to open database: %1").arg(m_database->lastError()));
        return;
    }

    if (m_model)
    {
        m_model->loadFromDatabase();
    }

    const QString schemaDesc = m_database->isDigikamDatabase()
                                   ? QStringLiteral("digiKam SQLite Catalog")
                                   : QStringLiteral("PhotoCarta SQLite Catalog");
    setStatusMessage(QStringLiteral("Connected to %1 (%2 photos)").arg(schemaDesc).arg(m_database->itemCount()));
    emit databaseStateChanged();
    emit sessionTypeChanged();
}

void SessionController::importDigikamDatabase(const QString &databasePath)
{
    if (!m_database || !m_database->isOpen())
    {
        showError(QStringLiteral("Target catalog database is not open"));
        return;
    }

    if (!m_database->importFromDigikamDatabase(databasePath))
    {
        showError(QStringLiteral("Failed to import digiKam database: %1").arg(m_database->lastError()));
        return;
    }

    if (m_model)
    {
        m_model->loadFromDatabase();
    }

    setStatusMessage(QStringLiteral("Successfully imported digiKam catalog (%1 photos total)").arg(m_database->itemCount()));
    emit databaseStateChanged();
    emit sessionTypeChanged();
}

void SessionController::selectCollection(int collectionId)
{
    m_currentCollectionId = collectionId;
    emit currentCollectionChanged();

    if (m_filterModel)
    {
        m_filterModel->setCollectionFilter(collectionId);
    }

    if (m_selectionManager)
    {
        if (m_filterModel && m_filterModel->count() > 0)
        {
            m_selectionManager->select(0);
        }
        else
        {
            m_selectionManager->clearSelection();
        }
    }

    if (collectionId > 0 && m_database && m_database->isOpen())
    {
        const auto collections = m_database->getCollections();
        for (const auto &c : collections)
        {
            if (c.id == collectionId)
            {
                setStatusMessage(QStringLiteral("Collection: %1 (%2 photo%3)")
                                     .arg(c.name)
                                     .arg(c.itemCount)
                                     .arg(c.itemCount == 1 ? QString() : QStringLiteral("s")));
                return;
            }
        }
    }
    else if (collectionId == 0)
    {
        setStatusMessage(QStringLiteral("All collections (%1 photo%2)")
                             .arg(m_model ? m_model->count() : 0)
                             .arg((m_model && m_model->count() == 1) ? QString() : QStringLiteral("s")));
    }
}

void SessionController::rescanCollection(int collectionId)
{
    if (!m_database || !m_database->isOpen() || collectionId <= 0)
    {
        return;
    }

    const auto collections = m_database->getCollections();
    for (const auto &c : collections)
    {
        if (c.id == collectionId)
        {
            setStatusMessage(QStringLiteral("Rescanning collection: %1...").arg(c.name));
            scanDirectory(c.rootPath, true);
            return;
        }
    }
}

void SessionController::removeCollection(int collectionId)
{
    if (!m_database || !m_database->isOpen() || collectionId <= 0)
    {
        return;
    }

    if (m_currentCollectionId == collectionId)
    {
        selectCollection(0);
    }

    m_database->removeCollection(collectionId);

    if (m_model)
    {
        m_model->loadFromDatabase(0);
    }
    setStatusMessage(QStringLiteral("Removed album from catalog"));
}

void SessionController::toggleCollectionAvailable(int collectionId)
{
    if (!m_database || !m_database->isOpen() || collectionId <= 0)
    {
        return;
    }

    const auto collections = m_database->getCollections();
    for (const auto &c : collections)
    {
        if (c.id == collectionId)
        {
            const bool newAvail = !c.isAvailable;
            setCollectionAvailable(c.rootPath, newAvail);
            setStatusMessage(newAvail ? QStringLiteral("Album '%1' marked as online").arg(c.name)
                                      : QStringLiteral("Album '%1' marked as offline").arg(c.name));
            return;
        }
    }
}

void SessionController::setCollectionAvailable(const QString &rootPath, bool available)
{
    if (m_model)
    {
        m_model->setCollectionAvailable(rootPath, available);
    }
}

void SessionController::clearSession()
{
    if (m_model)
    {
        m_model->clearSession();
        if (m_selectionManager)
        {
            m_selectionManager->clearSelection();
        }
        setPreviewMode(false);
        setStatusMessage(QStringLiteral("Session cleared"));
        emit sessionTypeChanged();
    }
}

void SessionController::dismissError()
{
    if (!m_hasError)
    {
        return;
    }
    m_hasError = false;
    m_errorMessage.clear();
    emit errorStateChanged();
}

void SessionController::showError(const QString &message)
{
    m_hasError = true;
    m_errorMessage = message;
    emit errorStateChanged();
}

bool SessionController::captureScreenshot(QObject *windowObject, const QString &filePath)
{
    auto *window = qobject_cast<QQuickWindow *>(windowObject);
    if (!window)
    {
        showError(QStringLiteral("Screenshot failed: invalid window target"));
        emit screenshotCompleted(filePath, false);
        return false;
    }

    const QImage img = window->grabWindow();
    if (img.isNull())
    {
        showError(QStringLiteral("Screenshot failed: grabWindow returned null image"));
        emit screenshotCompleted(filePath, false);
        return false;
    }

    const bool saved = img.save(filePath);
    if (saved)
    {
        setStatusMessage(QStringLiteral("Saved screenshot to ") + filePath);
    }
    else
    {
        showError(QStringLiteral("Screenshot failed: could not write to ") + filePath);
    }

    emit screenshotCompleted(filePath, saved);
    return saved;
}

double SessionController::viewerZoom() const
{
    return m_viewerZoom;
}

void SessionController::setViewerZoom(double zoom)
{
    const double clamped = std::clamp(zoom, 0.1, 10.0);
    if (qFuzzyCompare(m_viewerZoom, clamped))
    {
        return;
    }
    m_viewerZoom = clamped;
    emit viewerZoomChanged();
}

void SessionController::zoomIn(double step)
{
    setViewerZoom(m_viewerZoom + step);
}

void SessionController::zoomOut(double step)
{
    setViewerZoom(m_viewerZoom - step);
}

void SessionController::resetViewerZoom()
{
    setViewerZoom(1.0);
}

int SessionController::viewerRotation() const
{
    return m_viewerRotation;
}

void SessionController::setViewerRotation(int degrees)
{
    const int normalized = ((degrees % 360) + 360) % 360;
    if (m_viewerRotation == normalized)
    {
        return;
    }
    m_viewerRotation = normalized;
    emit viewerRotationChanged();
}

void SessionController::rotateClockwise()
{
    setViewerRotation(m_viewerRotation + 90);
}

void SessionController::rotateCounterClockwise()
{
    setViewerRotation(m_viewerRotation - 90);
}

void SessionController::resetViewerRotation()
{
    setViewerRotation(0);
}

bool SessionController::isFullscreen() const
{
    return m_isFullscreen;
}

void SessionController::setFullscreen(bool fullscreen)
{
    if (m_isFullscreen == fullscreen)
    {
        return;
    }
    m_isFullscreen = fullscreen;
    emit fullscreenChanged();
}

void SessionController::toggleFullscreen()
{
    setFullscreen(!m_isFullscreen);
}

void SessionController::onModelCountChanged()
{
    if (!m_model)
    {
        return;
    }
    const int count = m_model->count();
    setStatusMessage(QString::asprintf("%d photo%s in session • %s",
                                       count,
                                       count == 1 ? "" : "s",
                                       sessionTypeDescription().toUtf8().constData()));
    emit sessionTypeChanged();
}

void SessionController::onFileLoadError(const QString &fileName, const QString &errorMsg)
{
    showError(QStringLiteral("Error loading ") + fileName + QStringLiteral(": ") + errorMsg);
}

void SessionController::onBatchDiscovered(const QList<CatalogItem> &items)
{
    if (m_database && m_database->isOpen())
    {
        m_database->insertOrUpdateItems(items, m_currentCollectionId);
    }

    if (m_model)
    {
        m_model->addItems(items);
    }

    if (m_selectionManager && !m_selectionManager->hasSelection() && m_model && m_model->count() > 0)
    {
        m_selectionManager->select(0);
    }
}

void SessionController::onScanProgress(int count, const QString &currentFile)
{
    Q_UNUSED(currentFile);
    m_scanProgress = count;
    emit scanProgressChanged();
    setStatusMessage(QStringLiteral("Scanning... %1 photos indexed").arg(count));
}

void SessionController::onScanFinished(int totalFound, qint64 elapsedMs)
{
    if (m_database && m_database->isOpen() && m_currentCollectionId > 0)
    {
        m_database->updateCollectionScanTime(m_currentCollectionId);
    }

    m_scanProgress = totalFound;
    emit scanProgressChanged();
    emit isScanningChanged();
    emit databaseStateChanged();
    setStatusMessage(QStringLiteral("Scan complete: %1 photos indexed in %2 ms").arg(totalFound).arg(elapsedMs));
    emit sessionTypeChanged();
}

void SessionController::onScanCanceled()
{
    emit isScanningChanged();
    setStatusMessage(QStringLiteral("Scan canceled"));
}

} // namespace PhotoCarta
