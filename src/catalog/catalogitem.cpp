// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalogitem.h"

#include <cmath>

namespace PhotoCarta
{

QString CatalogItem::formatBytes(qint64 bytes)
{
    if (bytes <= 0)
    {
        return QStringLiteral("0 B");
    }

    constexpr qint64 kb = 1024;
    constexpr qint64 mb = 1024 * 1024;
    constexpr qint64 gb = 1024 * 1024 * 1024;

    if (bytes >= gb)
    {
        return QString::asprintf("%.2f GB", static_cast<double>(bytes) / static_cast<double>(gb));
    }
    if (bytes >= mb)
    {
        return QString::asprintf("%.1f MB", static_cast<double>(bytes) / static_cast<double>(mb));
    }
    if (bytes >= kb)
    {
        return QString::asprintf("%.1f KB", static_cast<double>(bytes) / static_cast<double>(kb));
    }
    return QString::asprintf("%lld B", static_cast<long long>(bytes));
}

QVariantMap CatalogItem::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("id"), id);
    map.insert(QStringLiteral("filePath"), filePath);
    map.insert(QStringLiteral("fileName"), fileName);
    map.insert(QStringLiteral("title"), title);
    map.insert(QStringLiteral("fileSizeBytes"), fileSizeBytes);
    map.insert(QStringLiteral("formattedFileSize"), formattedFileSize);
    map.insert(QStringLiteral("mimeType"), mimeType);
    map.insert(QStringLiteral("dateTaken"), dateTaken);
    map.insert(QStringLiteral("formattedDate"), formattedDate);
    map.insert(QStringLiteral("fileLastModified"), fileLastModified);
    map.insert(QStringLiteral("formattedLastModified"), formattedLastModified);
    map.insert(QStringLiteral("width"), dimensions.width());
    map.insert(QStringLiteral("height"), dimensions.height());
    map.insert(QStringLiteral("formattedDimensions"), formattedDimensions);
    map.insert(QStringLiteral("cameraModel"), cameraModel);
    map.insert(QStringLiteral("lensModel"), lensModel);
    map.insert(QStringLiteral("focalLength"), focalLength);
    map.insert(QStringLiteral("aperture"), aperture);
    map.insert(QStringLiteral("shutterSpeed"), shutterSpeed);
    map.insert(QStringLiteral("iso"), iso);
    map.insert(QStringLiteral("rating"), rating);
    map.insert(QStringLiteral("isFlagged"), isFlagged);
    map.insert(QStringLiteral("tags"), tags);
    map.insert(QStringLiteral("thumbnailUrl"), thumbnailUrl);
    map.insert(QStringLiteral("previewUrl"), previewUrl);
    map.insert(QStringLiteral("isDemoFixture"), isDemoFixture);
    map.insert(QStringLiteral("isMissing"), isMissing);
    return map;
}

} // namespace PhotoCarta
