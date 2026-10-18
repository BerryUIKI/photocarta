// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sessioncontroller.h"
#include "export/imageexporter.h"
#include "histogram/histogramcalculator.h"

#include <algorithm>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QQuickWindow>
#include <QStandardPaths>

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
    , m_exporter(new ImageExporter(this))
    , m_histogramCalculator(new HistogramCalculator(this))
{
    if (m_model)
    {
        connect(m_model, &CatalogModel::countChanged, this, &SessionController::onModelCountChanged);
        connect(m_model, &CatalogModel::fileLoadError, this, &SessionController::onFileLoadError);
        m_model->setXmpSyncEnabled(m_xmpSyncEnabled);
    }

    if (m_selectionManager)
    {
        connect(m_selectionManager, &SelectionManager::selectionChanged, this, [this]() {
            resetViewerZoom();
            resetViewerRotation();
            if (m_selectionManager && m_selectionManager->hasSelection())
            {
                const QVariantMap item = m_selectionManager->selectedItem();
                const QString filePath = item.value(QStringLiteral("filePath")).toString();
                const QString previewUrl = item.value(QStringLiteral("previewUrl")).toString();
                const QString targetPath = !filePath.isEmpty() ? filePath : previewUrl;
                if (!targetPath.isEmpty())
                {
                    requestHistogram(targetPath);
                }
            }
            else
            {
                m_currentHistogram.clear();
                emit currentHistogramChanged();
            }
        });
    }

    connect(m_histogramCalculator, &HistogramCalculator::histogramReady, this, [this](const QString &filePath, const QVariantMap &data) {
        Q_UNUSED(filePath)
        m_currentHistogram = data;
        emit currentHistogramChanged();
    });

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

    connect(m_exporter, &ImageExporter::progressChanged, this, [this](int completed, int total) {
        Q_UNUSED(completed)
        Q_UNUSED(total)
        emit exportProgressChanged();
    });
    connect(m_exporter, &ImageExporter::exportStateChanged, this, [this]() {
        emit exportProgressChanged();
    });
    connect(m_exporter, &ImageExporter::exportFinished, this, [this](const ExportResult &result) {
        emit exportProgressChanged();
        setStatusMessage(QStringLiteral("Export complete: %1 exported, %2 failed")
                         .arg(result.successCount).arg(result.failCount));
        emit exportCompleted(result.successCount, result.failCount, QString());
    });
    connect(m_exporter, &ImageExporter::exportCanceled, this, [this]() {
        emit exportProgressChanged();
        setStatusMessage(QStringLiteral("Export canceled"));
    });
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
    if (active && m_compareMode)
    {
        m_compareMode = false;
        emit compareModeChanged();
    }
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

void SessionController::setItemTitle(int proxyIndex, const QString &title)
{
    const int srcRow = sourceRowForProxy(proxyIndex);
    if (m_model && srcRow >= 0)
    {
        m_model->updateItemTitle(srcRow, title);
    }
}

void SessionController::setItemCaption(int proxyIndex, const QString &caption)
{
    const int srcRow = sourceRowForProxy(proxyIndex);
    if (m_model && srcRow >= 0)
    {
        m_model->updateItemCaption(srcRow, caption);
    }
}

void SessionController::setItemColorLabel(int proxyIndex, int colorLabel)
{
    const int srcRow = sourceRowForProxy(proxyIndex);
    if (m_model && srcRow >= 0)
    {
        m_model->updateItemColorLabel(srcRow, colorLabel);
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

void SessionController::addTagToItem(int proxyIndex, const QString &tag)
{
    const int srcRow = sourceRowForProxy(proxyIndex);
    if (m_model)
    {
        m_model->addTagToItem(srcRow, tag);
    }
}

void SessionController::removeTagFromItem(int proxyIndex, const QString &tag)
{
    const int srcRow = sourceRowForProxy(proxyIndex);
    if (m_model)
    {
        m_model->removeTagFromItem(srcRow, tag);
    }
}

void SessionController::setTagFilter(const QString &tag)
{
    if (m_filterModel)
    {
        m_filterModel->setTagFilter(tag);
        if (!tag.isEmpty())
        {
            setStatusMessage(QStringLiteral("Filtered by tag: %1 (%2 photos)").arg(tag).arg(m_filterModel->count()));
        }
        else
        {
            setStatusMessage(QStringLiteral("Tag filter cleared (%1 photos)").arg(m_filterModel->count()));
        }
    }
}

void SessionController::clearTagFilter()
{
    setTagFilter(QString());
}

void SessionController::setRatingFilter(int minRating)
{
    if (m_filterModel)
    {
        m_filterModel->setMinRating(minRating);
        if (minRating > 0)
        {
            setStatusMessage(QStringLiteral("Filtered by rating: %1★+ (%2 photos)").arg(minRating).arg(m_filterModel->count()));
        }
        else
        {
            setStatusMessage(QStringLiteral("Rating filter reset (%1 photos)").arg(m_filterModel->count()));
        }
    }
}

void SessionController::clearRatingFilter()
{
    setRatingFilter(0);
}

void SessionController::setColorLabelFilter(int colorLabel)
{
    if (m_filterModel)
    {
        m_filterModel->setColorLabelFilter(colorLabel);
        const QString labelName = (colorLabel == 1) ? QStringLiteral("Red")
                                : (colorLabel == 2) ? QStringLiteral("Yellow")
                                : (colorLabel == 3) ? QStringLiteral("Green")
                                : (colorLabel == 4) ? QStringLiteral("Blue")
                                : (colorLabel == 5) ? QStringLiteral("Purple")
                                : QStringLiteral("All");
        if (colorLabel > 0)
        {
            setStatusMessage(QStringLiteral("Filtered by color label: %1 (%2 photos)").arg(labelName).arg(m_filterModel->count()));
        }
        else
        {
            setStatusMessage(QStringLiteral("Color label filter reset (%1 photos)").arg(m_filterModel->count()));
        }
    }
}

void SessionController::clearColorLabelFilter()
{
    setColorLabelFilter(0);
}

void SessionController::clearAllFilters()
{
    if (m_filterModel)
    {
        m_filterModel->setSearchQuery(QString());
        m_filterModel->setCategoryFilter(QStringLiteral("all"));
        m_filterModel->setTagFilter(QString());
        m_filterModel->setMinRating(0);
        m_filterModel->setColorLabelFilter(0);
        m_filterModel->setCollectionFilter(0);
    }
    selectCollection(0);
    setStatusMessage(QStringLiteral("All filters cleared (%1 photos)").arg(m_filterModel ? m_filterModel->count() : 0));
}

void SessionController::setTitleForSelected(const QString &title)
{
    if (!m_selectionManager || !m_model)
    {
        return;
    }

    const QList<int> proxyRows = m_selectionManager->selectedProxyRows();
    if (proxyRows.isEmpty())
    {
        return;
    }

    QList<int> srcRows;
    srcRows.reserve(proxyRows.size());
    for (int proxyRow : proxyRows)
    {
        const int srcRow = sourceRowForProxy(proxyRow);
        if (srcRow >= 0)
        {
            srcRows.append(srcRow);
        }
    }

    m_model->updateItemsTitle(srcRows, title);
    setStatusMessage(QStringLiteral("Updated title for %1 photo(s)").arg(srcRows.size()));
}

void SessionController::setCaptionForSelected(const QString &caption)
{
    if (!m_selectionManager || !m_model)
    {
        return;
    }

    const QList<int> proxyRows = m_selectionManager->selectedProxyRows();
    if (proxyRows.isEmpty())
    {
        return;
    }

    QList<int> srcRows;
    srcRows.reserve(proxyRows.size());
    for (int proxyRow : proxyRows)
    {
        const int srcRow = sourceRowForProxy(proxyRow);
        if (srcRow >= 0)
        {
            srcRows.append(srcRow);
        }
    }

    m_model->updateItemsCaption(srcRows, caption);
    setStatusMessage(QStringLiteral("Updated caption for %1 photo(s)").arg(srcRows.size()));
}

void SessionController::setColorLabelForSelected(int colorLabel)
{
    if (!m_selectionManager || !m_model)
    {
        return;
    }

    const QList<int> proxyRows = m_selectionManager->selectedProxyRows();
    if (proxyRows.isEmpty())
    {
        return;
    }

    QList<int> srcRows;
    srcRows.reserve(proxyRows.size());
    for (int proxyRow : proxyRows)
    {
        const int srcRow = sourceRowForProxy(proxyRow);
        if (srcRow >= 0)
        {
            srcRows.append(srcRow);
        }
    }

    m_model->updateItemsColorLabel(srcRows, colorLabel);
    const QString labelName = (colorLabel == 1) ? QStringLiteral("Red")
                            : (colorLabel == 2) ? QStringLiteral("Yellow")
                            : (colorLabel == 3) ? QStringLiteral("Green")
                            : (colorLabel == 4) ? QStringLiteral("Blue")
                            : (colorLabel == 5) ? QStringLiteral("Purple")
                            : QStringLiteral("None");
    setStatusMessage(QStringLiteral("Color label '%1' applied to %2 photo(s)").arg(labelName).arg(srcRows.size()));
}

void SessionController::setRatingForSelected(int rating)
{
    if (!m_selectionManager || !m_model)
    {
        return;
    }

    const QList<int> proxyRows = m_selectionManager->selectedProxyRows();
    if (proxyRows.isEmpty())
    {
        return;
    }

    QList<int> srcRows;
    srcRows.reserve(proxyRows.size());
    for (int proxyRow : proxyRows)
    {
        const int srcRow = sourceRowForProxy(proxyRow);
        if (srcRow >= 0)
        {
            srcRows.append(srcRow);
        }
    }

    m_model->updateItemsRating(srcRows, rating);
    setStatusMessage(QStringLiteral("Rating %1★ applied to %2 photo(s)").arg(rating).arg(srcRows.size()));
}

void SessionController::setFlaggedForSelected(bool flagged)
{
    if (!m_selectionManager || !m_model)
    {
        return;
    }

    const QList<int> proxyRows = m_selectionManager->selectedProxyRows();
    if (proxyRows.isEmpty())
    {
        return;
    }

    QList<int> srcRows;
    srcRows.reserve(proxyRows.size());
    for (int proxyRow : proxyRows)
    {
        const int srcRow = sourceRowForProxy(proxyRow);
        if (srcRow >= 0)
        {
            srcRows.append(srcRow);
        }
    }

    m_model->updateItemsFlagged(srcRows, flagged);
    setStatusMessage(flagged
        ? QStringLiteral("Flagged %1 photo(s)").arg(srcRows.size())
        : QStringLiteral("Unflagged %1 photo(s)").arg(srcRows.size()));
}

void SessionController::toggleFlaggedForSelected()
{
    if (!m_selectionManager || !m_model)
    {
        return;
    }

    const bool currentFlagged = m_selectionManager->selectedItem().value(QStringLiteral("isFlagged")).toBool();
    setFlaggedForSelected(!currentFlagged);
}

void SessionController::addTagToSelected(const QString &tag)
{
    if (!m_selectionManager || !m_model)
    {
        return;
    }

    const QString trimmed = tag.trimmed();
    if (trimmed.isEmpty())
    {
        return;
    }

    const QList<int> proxyRows = m_selectionManager->selectedProxyRows();
    if (proxyRows.isEmpty())
    {
        return;
    }

    QList<int> srcRows;
    srcRows.reserve(proxyRows.size());
    for (int proxyRow : proxyRows)
    {
        const int srcRow = sourceRowForProxy(proxyRow);
        if (srcRow >= 0)
        {
            srcRows.append(srcRow);
        }
    }

    m_model->addTagToItems(srcRows, trimmed);
    setStatusMessage(QStringLiteral("Tag '%1' added to %2 photo(s)").arg(trimmed).arg(srcRows.size()));
}

void SessionController::removeTagFromSelected(const QString &tag)
{
    if (!m_selectionManager || !m_model)
    {
        return;
    }

    const QString trimmed = tag.trimmed();
    if (trimmed.isEmpty())
    {
        return;
    }

    const QList<int> proxyRows = m_selectionManager->selectedProxyRows();
    if (proxyRows.isEmpty())
    {
        return;
    }

    QList<int> srcRows;
    srcRows.reserve(proxyRows.size());
    for (int proxyRow : proxyRows)
    {
        const int srcRow = sourceRowForProxy(proxyRow);
        if (srcRow >= 0)
        {
            srcRows.append(srcRow);
        }
    }

    m_model->removeTagFromItems(srcRows, trimmed);
    setStatusMessage(QStringLiteral("Tag '%1' removed from %2 photo(s)").arg(trimmed).arg(srcRows.size()));
}

void SessionController::copySelectedPathsToClipboard()
{
    if (!m_selectionManager || !m_filterModel)
    {
        return;
    }

    const QList<int> proxyRows = m_selectionManager->selectedProxyRows();
    if (proxyRows.isEmpty())
    {
        return;
    }

    QStringList paths;
    for (int proxyRow : proxyRows)
    {
        const QModelIndex idx = m_filterModel->index(proxyRow, 0);
        const QString p = idx.data(CatalogModel::FilePathRole).toString();
        if (!p.isEmpty())
        {
            paths.append(p);
        }
    }

    if (!paths.isEmpty())
    {
        QClipboard *clipboard = QGuiApplication::clipboard();
        if (clipboard)
        {
            clipboard->setText(paths.join(QLatin1Char('\n')));
            setStatusMessage(QStringLiteral("Copied %1 path(s) to clipboard").arg(paths.size()));
        }
    }
}

void SessionController::revealInFileManager(int proxyIndex)
{
    if (!m_filterModel)
    {
        return;
    }

    if (proxyIndex < 0 && m_selectionManager)
    {
        proxyIndex = m_selectionManager->currentIndex();
    }

    if (proxyIndex < 0 || proxyIndex >= m_filterModel->rowCount())
    {
        return;
    }

    const QModelIndex idx = m_filterModel->index(proxyIndex, 0);
    const QString filePath = idx.data(CatalogModel::FilePathRole).toString();
    if (filePath.isEmpty() || filePath.startsWith(QStringLiteral("qrc:")))
    {
        setStatusMessage(QStringLiteral("Demo fixture has no physical file on disk."));
        return;
    }

    const QFileInfo info(filePath);
    if (!info.exists())
    {
        setStatusMessage(QStringLiteral("File does not exist on disk: %1").arg(filePath));
        return;
    }

    const QString dirPath = info.dir().absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
    setStatusMessage(QStringLiteral("Opened folder: %1").arg(dirPath));
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

bool SessionController::isExporting() const
{
    return m_exporter ? m_exporter->isExporting() : false;
}

int SessionController::exportProgress() const
{
    return m_exporter ? m_exporter->progress() : 0;
}

int SessionController::exportTotal() const
{
    return m_exporter ? m_exporter->total() : 0;
}

bool SessionController::exportDialogOpen() const
{
    return m_exportDialogOpen;
}

void SessionController::setExportDialogOpen(bool open)
{
    if (m_exportDialogOpen != open)
    {
        m_exportDialogOpen = open;
        emit exportDialogOpenChanged();
    }
}

void SessionController::openExportDialog()
{
    setExportDialogOpen(true);
}

void SessionController::closeExportDialog()
{
    setExportDialogOpen(false);
}

QString SessionController::defaultExportDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
}

void SessionController::cancelExport()
{
    if (m_exporter)
    {
        m_exporter->cancel();
    }
}

void SessionController::startExportSelected(const QVariantMap &options)
{
    if (!m_exporter || !m_model)
    {
        return;
    }

    if (m_exporter->isExporting())
    {
        return;
    }

    ExportSettings settings;
    settings.outputDirectory = options.value(QStringLiteral("outputDirectory")).toString();
    if (settings.outputDirectory.isEmpty())
    {
        settings.outputDirectory = defaultExportDirectory() + QStringLiteral("/PhotoCarta_Export");
    }

    // Clean up file:// prefix if user picked via QML FolderDialog
    if (settings.outputDirectory.startsWith(QStringLiteral("file:///")))
    {
        settings.outputDirectory = QUrl(settings.outputDirectory).toLocalFile();
    }

    settings.format = options.value(QStringLiteral("format"), QStringLiteral("original")).toString();
    settings.quality = options.value(QStringLiteral("quality"), 90).toInt();
    settings.maxDimension = options.value(QStringLiteral("maxDimension"), 0).toInt();
    settings.preserveMetadata = options.value(QStringLiteral("preserveMetadata"), true).toBool();
    settings.filenamePrefix = options.value(QStringLiteral("filenamePrefix")).toString();
    settings.filenameSuffix = options.value(QStringLiteral("filenameSuffix")).toString();
    settings.overwriteExisting = options.value(QStringLiteral("overwriteExisting"), false).toBool();

    QList<CatalogItem> itemsToExport;

    const QList<int> proxyRows = m_selectionManager ? m_selectionManager->selectedProxyRows() : QList<int>();
    if (proxyRows.isEmpty())
    {
        if (m_selectionManager && m_selectionManager->currentIndex() >= 0)
        {
            const int srcRow = sourceRowForProxy(m_selectionManager->currentIndex());
            if (const CatalogItem *item = m_model->itemAt(srcRow))
            {
                itemsToExport.append(*item);
            }
        }
    }
    else
    {
        for (int pRow : proxyRows)
        {
            const int srcRow = sourceRowForProxy(pRow);
            if (const CatalogItem *item = m_model->itemAt(srcRow))
            {
                itemsToExport.append(*item);
            }
        }
    }

    if (itemsToExport.isEmpty())
    {
        showError(QStringLiteral("No photos selected for export."));
        return;
    }

    if (m_exporter->startExport(itemsToExport, settings))
    {
        setStatusMessage(QStringLiteral("Exporting %1 photo(s)...").arg(itemsToExport.size()));
    }
}

bool SessionController::compareMode() const
{
    return m_compareMode;
}

void SessionController::setCompareMode(bool active)
{
    if (m_compareMode == active)
    {
        return;
    }

    m_compareMode = active;

    if (m_compareMode)
    {
        if (m_previewMode)
        {
            m_previewMode = false;
            emit previewModeChanged();
        }

        m_compareProxyRows.clear();
        m_winnerId.clear();
        m_compareZoom = 1.0;
        m_comparePanX = 0.0;
        m_comparePanY = 0.0;
        m_activeCompareSlot = 0;

        if (m_filterModel && m_filterModel->rowCount() > 0)
        {
            const QList<int> selRows = m_selectionManager ? m_selectionManager->selectedProxyRows() : QList<int>();
            if (selRows.size() >= 2)
            {
                // Take up to 4 selected photos
                const int count = std::min(4, static_cast<int>(selRows.size()));
                for (int i = 0; i < count; ++i)
                {
                    m_compareProxyRows.append(selRows[i]);
                }
            }
            else if (selRows.size() == 1)
            {
                const int first = selRows.first();
                m_compareProxyRows.append(first);
                // Pair with next item, or previous if at end
                if (first + 1 < m_filterModel->rowCount())
                {
                    m_compareProxyRows.append(first + 1);
                }
                else if (first - 1 >= 0)
                {
                    m_compareProxyRows.append(first - 1);
                }
            }
            else
            {
                // No selection, take first 2 photos if available
                m_compareProxyRows.append(0);
                if (m_filterModel->rowCount() > 1)
                {
                    m_compareProxyRows.append(1);
                }
            }
        }
    }
    else
    {
        m_compareProxyRows.clear();
        m_winnerId.clear();
    }

    emit compareModeChanged();
    emit compareItemsChanged();
    emit compareZoomChanged();
    emit comparePanChanged();
    emit activeCompareSlotChanged();
    emit winnerIdChanged();
}

void SessionController::toggleCompareMode()
{
    setCompareMode(!m_compareMode);
}

bool SessionController::syncZoomPan() const
{
    return m_syncZoomPan;
}

void SessionController::setSyncZoomPan(bool sync)
{
    if (m_syncZoomPan == sync)
    {
        return;
    }
    m_syncZoomPan = sync;
    emit syncZoomPanChanged();
}

void SessionController::toggleSyncZoomPan()
{
    setSyncZoomPan(!m_syncZoomPan);
}

QVariantList SessionController::compareItems() const
{
    QVariantList items;
    if (!m_filterModel)
    {
        return items;
    }

    static const QStringList slotLabels = {
        QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C"), QStringLiteral("D")
    };

    for (int i = 0; i < m_compareProxyRows.size(); ++i)
    {
        const int proxyRow = m_compareProxyRows.at(i);
        if (proxyRow >= 0 && proxyRow < m_filterModel->rowCount())
        {
            QVariantMap item = m_filterModel->get(proxyRow);
            item.insert(QStringLiteral("proxyRow"), proxyRow);
            item.insert(QStringLiteral("slotIndex"), i);
            item.insert(QStringLiteral("slotLabel"), (i < slotLabels.size()) ? slotLabels[i] : QString::number(i + 1));
            const QString id = item.value(QStringLiteral("id")).toString();
            item.insert(QStringLiteral("isWinner"), (!m_winnerId.isEmpty() && m_winnerId == id));
            items.append(item);
        }
    }
    return items;
}

int SessionController::compareCount() const
{
    return static_cast<int>(m_compareProxyRows.size());
}

double SessionController::compareZoom() const
{
    return m_compareZoom;
}

void SessionController::setCompareZoom(double zoom)
{
    const double clamped = std::clamp(zoom, 0.2, 8.0);
    if (qFuzzyCompare(m_compareZoom, clamped))
    {
        return;
    }
    m_compareZoom = clamped;
    emit compareZoomChanged();
}

void SessionController::zoomInCompare(double step)
{
    setCompareZoom(m_compareZoom + step);
}

void SessionController::zoomOutCompare(double step)
{
    setCompareZoom(m_compareZoom - step);
}

void SessionController::resetCompareZoom()
{
    setCompareZoom(1.0);
    setComparePan(0.0, 0.0);
}

double SessionController::comparePanX() const
{
    return m_comparePanX;
}

void SessionController::setComparePanX(double x)
{
    if (qFuzzyCompare(m_comparePanX, x))
    {
        return;
    }
    m_comparePanX = x;
    emit comparePanChanged();
}

double SessionController::comparePanY() const
{
    return m_comparePanY;
}

void SessionController::setComparePanY(double y)
{
    if (qFuzzyCompare(m_comparePanY, y))
    {
        return;
    }
    m_comparePanY = y;
    emit comparePanChanged();
}

void SessionController::setComparePan(double x, double y)
{
    if (qFuzzyCompare(m_comparePanX, x) && qFuzzyCompare(m_comparePanY, y))
    {
        return;
    }
    m_comparePanX = x;
    m_comparePanY = y;
    emit comparePanChanged();
}

int SessionController::activeCompareSlot() const
{
    return m_activeCompareSlot;
}

void SessionController::setActiveCompareSlot(int slot)
{
    if (m_activeCompareSlot == slot || slot < 0 || slot >= m_compareProxyRows.size())
    {
        return;
    }
    m_activeCompareSlot = slot;
    emit activeCompareSlotChanged();
}

QString SessionController::winnerId() const
{
    return m_winnerId;
}

void SessionController::setAsWinner(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= m_compareProxyRows.size() || !m_filterModel)
    {
        return;
    }

    const int proxyRow = m_compareProxyRows.at(slotIndex);
    if (proxyRow < 0 || proxyRow >= m_filterModel->rowCount())
    {
        return;
    }

    const QString id = m_filterModel->index(proxyRow, 0).data(CatalogModel::IdRole).toString();
    if (m_winnerId == id)
    {
        m_winnerId.clear();
    }
    else
    {
        m_winnerId = id;
        const int srcRow = sourceRowForProxy(proxyRow);
        if (m_model)
        {
            const CatalogItem *item = m_model->itemAt(srcRow);
            if (item && !item->isFlagged)
            {
                m_model->updateItemFlagged(srcRow, true);
            }
        }
    }

    emit winnerIdChanged();
    emit compareItemsChanged();
}

void SessionController::clearWinner()
{
    if (m_winnerId.isEmpty())
    {
        return;
    }
    m_winnerId.clear();
    emit winnerIdChanged();
    emit compareItemsChanged();
}

void SessionController::removeCompareItem(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= m_compareProxyRows.size())
    {
        return;
    }

    m_compareProxyRows.removeAt(slotIndex);
    if (m_activeCompareSlot >= m_compareProxyRows.size())
    {
        m_activeCompareSlot = std::max(0, static_cast<int>(m_compareProxyRows.size()) - 1);
        emit activeCompareSlotChanged();
    }

    if (m_compareProxyRows.size() < 2)
    {
        setCompareMode(false);
        return;
    }

    emit compareItemsChanged();
}

void SessionController::swapCompareItems(int slotA, int slotB)
{
    if (slotA >= 0 && slotA < m_compareProxyRows.size() &&
        slotB >= 0 && slotB < m_compareProxyRows.size() && slotA != slotB)
    {
        m_compareProxyRows.swapItemsAt(slotA, slotB);
        emit compareItemsChanged();
    }
}

void SessionController::setCompareRating(int slotIndex, int rating)
{
    if (slotIndex < 0 || slotIndex >= m_compareProxyRows.size())
    {
        return;
    }
    const int proxyRow = m_compareProxyRows.at(slotIndex);
    setItemRating(proxyRow, rating);
    emit compareItemsChanged();
}

void SessionController::toggleCompareFlagged(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= m_compareProxyRows.size())
    {
        return;
    }
    const int proxyRow = m_compareProxyRows.at(slotIndex);
    toggleItemFlagged(proxyRow);
    emit compareItemsChanged();
}

void SessionController::setCompareColorLabel(int slotIndex, int colorLabel)
{
    if (slotIndex < 0 || slotIndex >= m_compareProxyRows.size())
    {
        return;
    }
    const int proxyRow = m_compareProxyRows.at(slotIndex);
    setItemColorLabel(proxyRow, colorLabel);
    emit compareItemsChanged();
}

void SessionController::addAdjacentToCompare(bool next)
{
    if (!m_filterModel || m_compareProxyRows.size() >= 4)
    {
        return;
    }

    const int total = m_filterModel->rowCount();
    if (total == 0)
    {
        return;
    }

    int candidate = -1;
    if (next)
    {
        const int last = m_compareProxyRows.isEmpty() ? 0 : m_compareProxyRows.last();
        for (int i = last + 1; i < total; ++i)
        {
            if (!m_compareProxyRows.contains(i))
            {
                candidate = i;
                break;
            }
        }
    }
    else
    {
        const int first = m_compareProxyRows.isEmpty() ? 0 : m_compareProxyRows.first();
        for (int i = first - 1; i >= 0; --i)
        {
            if (!m_compareProxyRows.contains(i))
            {
                candidate = i;
                break;
            }
        }
    }

    if (candidate >= 0)
    {
        m_compareProxyRows.append(candidate);
        emit compareItemsChanged();
    }
}

QVariantMap SessionController::currentHistogram() const
{
    return m_currentHistogram;
}

QString SessionController::histogramChannel() const
{
    return m_histogramChannel;
}

void SessionController::setHistogramChannel(const QString &channel)
{
    const QString lowered = channel.toLower();
    if (m_histogramChannel == lowered)
    {
        return;
    }
    m_histogramChannel = lowered;
    emit histogramChannelChanged();
}

bool SessionController::histogramLogScale() const
{
    return m_histogramLogScale;
}

void SessionController::setHistogramLogScale(bool logScale)
{
    if (m_histogramLogScale == logScale)
    {
        return;
    }
    m_histogramLogScale = logScale;
    emit histogramLogScaleChanged();
}

void SessionController::toggleHistogramLogScale()
{
    setHistogramLogScale(!m_histogramLogScale);
}

bool SessionController::histogramVisible() const
{
    return m_histogramVisible;
}

void SessionController::setHistogramVisible(bool visible)
{
    if (m_histogramVisible == visible)
    {
        return;
    }
    m_histogramVisible = visible;
    emit histogramVisibleChanged();
}

void SessionController::toggleHistogramVisible()
{
    setHistogramVisible(!m_histogramVisible);
}

void SessionController::requestHistogram(const QString &filePath)
{
    if (m_histogramCalculator && !filePath.isEmpty())
    {
        m_histogramCalculator->requestHistogram(filePath);
    }
}

HistogramCalculator *SessionController::histogramCalculator() const
{
    return m_histogramCalculator;
}

bool SessionController::xmpSyncEnabled() const
{
    return m_xmpSyncEnabled;
}

void SessionController::setXmpSyncEnabled(bool enabled)
{
    if (m_xmpSyncEnabled == enabled)
    {
        return;
    }
    m_xmpSyncEnabled = enabled;
    if (m_model)
    {
        m_model->setXmpSyncEnabled(enabled);
    }
    emit xmpSyncEnabledChanged();
}

void SessionController::exportXmpSidecar(int proxyIndex)
{
    if (!m_model)
    {
        return;
    }
    const int srcRow = sourceRowForProxy(proxyIndex);
    if (srcRow >= 0)
    {
        if (m_model->writeXmpSidecar(srcRow))
        {
            setStatusMessage(QStringLiteral("Exported XMP sidecar for photo"));
        }
    }
}

void SessionController::importXmpSidecar(int proxyIndex)
{
    if (!m_model)
    {
        return;
    }
    const int srcRow = sourceRowForProxy(proxyIndex);
    if (srcRow >= 0)
    {
        if (m_model->readXmpSidecar(srcRow))
        {
            setStatusMessage(QStringLiteral("Imported metadata from XMP sidecar"));
        }
    }
}

void SessionController::exportXmpSidecarsForSelected()
{
    if (!m_model || !m_selectionManager)
    {
        return;
    }

    const QList<int> proxyRows = m_selectionManager->selectedProxyRows();
    if (proxyRows.isEmpty())
    {
        const int curr = m_selectionManager->currentIndex();
        if (curr >= 0)
        {
            exportXmpSidecar(curr);
        }
        return;
    }

    QList<int> srcRows;
    srcRows.reserve(proxyRows.size());
    for (int pRow : proxyRows)
    {
        const int sRow = sourceRowForProxy(pRow);
        if (sRow >= 0)
        {
            srcRows.append(sRow);
        }
    }

    const int count = m_model->writeXmpSidecars(srcRows);
    setStatusMessage(QStringLiteral("Exported %1 XMP sidecar(s)").arg(count));
}

void SessionController::importXmpSidecarsForSelected()
{
    if (!m_model || !m_selectionManager)
    {
        return;
    }

    const QList<int> proxyRows = m_selectionManager->selectedProxyRows();
    if (proxyRows.isEmpty())
    {
        const int curr = m_selectionManager->currentIndex();
        if (curr >= 0)
        {
            importXmpSidecar(curr);
        }
        return;
    }

    QList<int> srcRows;
    srcRows.reserve(proxyRows.size());
    for (int pRow : proxyRows)
    {
        const int sRow = sourceRowForProxy(pRow);
        if (sRow >= 0)
        {
            srcRows.append(sRow);
        }
    }

    const int count = m_model->readXmpSidecars(srcRows);
    setStatusMessage(QStringLiteral("Imported metadata from %1 XMP sidecar(s)").arg(count));
}

} // namespace PhotoCarta
