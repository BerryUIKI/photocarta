// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "catalogfiltermodel.h"
#include "catalogmodel.h"
#include "selectionmanager.h"

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
    Q_PROPERTY(bool isEphemeralSession READ isEphemeralSession CONSTANT)
    Q_PROPERTY(QString sessionTypeDescription READ sessionTypeDescription NOTIFY sessionTypeChanged)

public:
    explicit SessionController(CatalogModel *model,
                               CatalogFilterModel *filterModel,
                               SelectionManager *selectionManager,
                               QObject *parent = nullptr);
    ~SessionController() override = default;

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

    [[nodiscard]] static constexpr bool isEphemeralSession() { return true; }
    [[nodiscard]] QString sessionTypeDescription() const;

    Q_INVOKABLE void togglePreviewMode();
    Q_INVOKABLE void toggleLeftPane();
    Q_INVOKABLE void toggleRightPane();
    Q_INVOKABLE void loadDemoSamples();
    Q_INVOKABLE void openLocalFiles(const QList<QUrl> &urls);
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
    void screenshotCompleted(const QString &path, bool success);

private slots:
    void onModelCountChanged();
    void onFileLoadError(const QString &fileName, const QString &errorMsg);

private:
    CatalogModel *m_model = nullptr;
    CatalogFilterModel *m_filterModel = nullptr;
    SelectionManager *m_selectionManager = nullptr;

    bool m_previewMode = false;
    bool m_leftPaneVisible = true;
    bool m_rightPaneVisible = true;
    QString m_statusMessage = QStringLiteral("Ready • PhotoCarta GUI Shell Prototype");
    bool m_hasError = false;
    QString m_errorMessage;
    double m_zoomFactor = 1.0;
};

} // namespace PhotoCarta
