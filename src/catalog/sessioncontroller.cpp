// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sessioncontroller.h"

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
{
    if (m_model)
    {
        connect(m_model, &CatalogModel::countChanged, this, &SessionController::onModelCountChanged);
        connect(m_model, &CatalogModel::fileLoadError, this, &SessionController::onFileLoadError);
    }
}

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
    return QStringLiteral("Ephemeral Local Photos (Non-destructive Read-Only)");
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

} // namespace PhotoCarta
