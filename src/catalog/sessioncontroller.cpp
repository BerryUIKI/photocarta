// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sessioncontroller.h"

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
