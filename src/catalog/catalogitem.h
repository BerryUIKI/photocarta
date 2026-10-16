// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

namespace PhotoCarta
{

struct CatalogItem
{
    QString id;
    QString filePath;
    QString fileName;
    QString title;
    qint64 fileSizeBytes = 0;
    QString formattedFileSize;
    QString mimeType;
    QDateTime dateTaken; // Invalid if unknown / no EXIF
    QString formattedDate;
    QDateTime fileLastModified;
    QString formattedLastModified;
    QSize dimensions;
    QString formattedDimensions;
    QString cameraModel;
    QString lensModel;
    QString focalLength;
    QString aperture;
    QString shutterSpeed;
    int iso = 0;
    int rating = 0; // Display-only fixture data
    bool isFlagged = false;
    QStringList tags; // Display-only fixture data
    QUrl thumbnailUrl;
    QUrl previewUrl;
    bool isDemoFixture = false; // Clearly identifies demo content vs user-opened files
    bool isMissing = false;     // Identifies items whose collection is temporarily offline/unmounted
    int collectionId = 0;       // Database collection ID

    [[nodiscard]] QVariantMap toVariantMap() const;
    static QString formatBytes(qint64 bytes);
};

} // namespace PhotoCarta
