// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "trashservice.h"

#include "database/catalogdatabase.h"
#include "metadata/xmpsidecar.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>

namespace PhotoCarta
{

namespace
{
Q_LOGGING_CATEGORY(lcTrash, "photocarta.trash", QtWarningMsg)
}

QVariantMap TrashCandidate::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("itemId"), itemId);
    map.insert(QStringLiteral("filePath"), filePath);
    map.insert(QStringLiteral("fileName"), fileName);
    map.insert(QStringLiteral("status"), static_cast<int>(status));
    map.insert(QStringLiteral("statusMessage"), statusMessage);
    return map;
}

QVariantMap TrashResult::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("totalCount"), totalCount);
    map.insert(QStringLiteral("successCount"), successCount);
    map.insert(QStringLiteral("failCount"), failCount);
    map.insert(QStringLiteral("trashedPaths"), trashedPaths);
    map.insert(QStringLiteral("errorMessages"), errorMessages);
    return map;
}

TrashService::TrashService(QObject *parent)
    : QObject(parent)
{
}

QString TrashService::statusMessage(TrashStatus status, const QString &fileName)
{
    switch (status)
    {
    case TrashStatus::Ok:
        return QObject::tr("Ready to move to trash");
    case TrashStatus::DemoFixture:
        return QObject::tr("%1 is a built-in demo photo and cannot be moved to trash").arg(fileName);
    case TrashStatus::CollectionOffline:
        return QObject::tr("%1 is on a collection that is currently offline; offline items are never removed").arg(fileName);
    case TrashStatus::SourceFileNotFound:
        return QObject::tr("%1 could not be found on disk").arg(fileName);
    case TrashStatus::MoveFailed:
        return QObject::tr("%1 could not be moved to trash (the platform may not provide a trash for this location)").arg(fileName);
    }
    return QString();
}

QList<TrashCandidate> TrashService::evaluateCandidates(const QList<CatalogItem> &items)
{
    QList<TrashCandidate> candidates;
    candidates.reserve(items.size());

    for (const CatalogItem &item : items)
    {
        TrashCandidate candidate;
        candidate.itemId = item.id;
        candidate.filePath = item.filePath;
        candidate.fileName = item.fileName;

        if (item.isDemoFixture || item.filePath.startsWith(QStringLiteral("qrc:")))
        {
            candidate.status = TrashStatus::DemoFixture;
        }
        else if (item.isMissing)
        {
            candidate.status = TrashStatus::CollectionOffline;
        }
        else if (!QFileInfo::exists(item.filePath))
        {
            candidate.status = TrashStatus::SourceFileNotFound;
        }
        else
        {
            candidate.status = TrashStatus::Ok;
        }

        candidate.statusMessage = statusMessage(candidate.status, candidate.fileName);
        candidates.append(candidate);
    }

    return candidates;
}

TrashResult TrashService::moveToTrash(const QList<CatalogItem> &items)
{
    TrashResult result;
    result.totalCount = items.size();

    const QList<TrashCandidate> candidates = evaluateCandidates(items);

    for (const TrashCandidate &candidate : candidates)
    {
        if (candidate.status != TrashStatus::Ok)
        {
            ++result.failCount;
            result.errorMessages.append(candidate.statusMessage);
            qCWarning(lcTrash) << "Refusing to trash:" << candidate.filePath << ":" << candidate.statusMessage;
            continue;
        }

        QFile file(candidate.filePath);
        if (file.moveToTrash())
        {
            ++result.successCount;
            result.trashedPaths.append(candidate.filePath);

            // Move the companion XMP sidecar along with the image so metadata travels with the file
            const QString sidecarPath = XmpSidecar::sidecarPathForImage(candidate.filePath);
            if (!sidecarPath.isEmpty() && QFileInfo::exists(sidecarPath))
            {
                QFile sidecar(sidecarPath);
                if (!sidecar.moveToTrash())
                {
                    qCWarning(lcTrash) << "Failed to move sidecar to trash:" << sidecarPath << ":" << sidecar.errorString();
                }
            }
        }
        else
        {
            ++result.failCount;
            result.errorMessages.append(statusMessage(TrashStatus::MoveFailed, candidate.fileName));
            qCWarning(lcTrash) << "Move to trash failed for:" << candidate.filePath << ":" << file.errorString();
        }
    }

    return result;
}

} // namespace PhotoCarta
