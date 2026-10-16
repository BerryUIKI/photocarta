// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAtomicInt>
#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QQuickTextureFactory>
#include <QSize>
#include <QString>

namespace PhotoCarta
{

class ThumbnailImageResponse : public QQuickImageResponse
{
    Q_OBJECT

public:
    ThumbnailImageResponse(const QString &id, const QSize &requestedSize);
    ~ThumbnailImageResponse() override;

    [[nodiscard]] QQuickTextureFactory *textureFactory() const override;
    [[nodiscard]] QString errorString() const override;
    void cancel() override;

private:
    void execute();
    [[nodiscard]] static QString resolveFilePath(const QString &id);

    QString m_id;
    QSize m_requestedSize;
    QQuickTextureFactory *m_textureFactory = nullptr;
    QString m_error;
    QAtomicInt m_isCanceled{0};
};

class ThumbnailImageProvider : public QQuickAsyncImageProvider
{
public:
    explicit ThumbnailImageProvider();
    ~ThumbnailImageProvider() override = default;

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;
};

} // namespace PhotoCarta
