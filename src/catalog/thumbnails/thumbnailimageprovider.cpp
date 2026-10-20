// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumbnailimageprovider.h"
#include "thumbnailcache.h"

#include <QFileInfo>
#include <QLoggingCategory>
#include <QThreadPool>
#include <QUrl>

namespace
{
Q_LOGGING_CATEGORY(lcThumbProvider, "photocarta.thumbnails.provider")
}

namespace PhotoCarta
{

ThumbnailImageResponse::ThumbnailImageResponse(const QString &id, const QSize &requestedSize)
    : m_id(id), m_requestedSize(requestedSize)
{
    // Offload thumbnail retrieval and decoding to global thread pool
    QThreadPool::globalInstance()->start(QRunnable::create([this]() {
        execute();
    }));
}

ThumbnailImageResponse::~ThumbnailImageResponse()
{
    delete m_textureFactory;
}

QQuickTextureFactory *ThumbnailImageResponse::textureFactory() const
{
    return m_textureFactory;
}

QString ThumbnailImageResponse::errorString() const
{
    return m_error;
}

void ThumbnailImageResponse::cancel()
{
    m_isCanceled.storeRelaxed(1);
}

QString ThumbnailImageResponse::resolveFilePath(const QString &id)
{
    QString path = id;

    // Handle URL decoding if passed percent-encoded or with file scheme
    if (path.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive))
    {
        const QUrl url(path);
        if (url.isLocalFile())
        {
            path = url.toLocalFile();
        }
    }
    else if (path.contains(QLatin1Char('%')))
    {
        path = QUrl::fromPercentEncoding(path.toUtf8());
    }

    if (QFileInfo::exists(path))
    {
        return path;
    }

    // Try raw id in case it wasn't encoded
    if (QFileInfo::exists(id))
    {
        return id;
    }

    return path;
}

void ThumbnailImageResponse::execute()
{
    if (m_isCanceled.loadRelaxed())
    {
        emit finished();
        return;
    }

    const QString filePath = resolveFilePath(m_id);
    const QSize targetSize = (m_requestedSize.isValid() && m_requestedSize.width() > 0 && m_requestedSize.height() > 0)
                                 ? m_requestedSize
                                 : QSize(256, 256);

    const QImage image = ThumbnailCache::instance().getOrCreateThumbnail(filePath, targetSize);

    if (m_isCanceled.loadRelaxed())
    {
        emit finished();
        return;
    }

    if (!image.isNull())
    {
        m_textureFactory = QQuickTextureFactory::textureFactoryForImage(image);
    }
    else
    {
        m_error = QStringLiteral("Failed to load thumbnail for %1").arg(filePath);
        qCDebug(lcThumbProvider) << m_error;
    }

    emit finished();
}

ThumbnailImageProvider::ThumbnailImageProvider() = default;

QQuickImageResponse *ThumbnailImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    return new ThumbnailImageResponse(id, requestedSize);
}

} // namespace PhotoCarta
