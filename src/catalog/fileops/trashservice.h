// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "catalogitem.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace PhotoCarta
{

/**
 * @brief Reason a candidate cannot be moved to the trash.
 */
enum class TrashStatus
{
    Ok = 0,
    DemoFixture,
    CollectionOffline,
    SourceFileNotFound,
    MoveFailed
};

/**
 * @brief Per-item evaluation/execution result of a move-to-trash operation.
 */
struct TrashCandidate
{
    QString itemId;
    QString filePath;
    QString fileName;
    TrashStatus status = TrashStatus::Ok;
    QString statusMessage;

    [[nodiscard]] bool canTrash() const { return status == TrashStatus::Ok; }
    [[nodiscard]] QVariantMap toVariantMap() const;
};

/**
 * @brief Aggregate results from executing a move-to-trash operation.
 */
struct TrashResult
{
    int totalCount = 0;
    int successCount = 0;
    int failCount = 0;
    QStringList trashedPaths;
    QStringList errorMessages;

    [[nodiscard]] QVariantMap toVariantMap() const;
};

/**
 * @brief Move-to-trash file operation service.
 *
 * Validates candidates (demo fixtures, qrc resources, offline collections and
 * missing files are always refused) and moves accepted files to the platform
 * trash via QFile::moveToTrash, along with companion XMP sidecars. Items on
 * media where the platform provides no trash are reported as failures rather
 * than deleted permanently.
 */
class TrashService : public QObject
{
    Q_OBJECT

public:
    explicit TrashService(QObject *parent = nullptr);
    ~TrashService() override = default;

    /// Evaluates which items are eligible for trashing without touching the filesystem
    [[nodiscard]] static QList<TrashCandidate> evaluateCandidates(const QList<CatalogItem> &items);

    /// Executes the move-to-trash for all eligible items and returns per-item results
    [[nodiscard]] static TrashResult moveToTrash(const QList<CatalogItem> &items);

    /// Human-readable description of a trash status
    [[nodiscard]] static QString statusMessage(TrashStatus status, const QString &fileName);
};

} // namespace PhotoCarta
