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
 * @brief Case conversion modes for filename transformation.
 */
enum class CaseConversion
{
    Unchanged = 0,
    Lowercase,
    Uppercase,
    TitleCase
};

/**
 * @brief Status indicator for a candidate file rename.
 */
enum class RenameStatus
{
    Ok = 0,
    ConflictFileExists,
    ConflictBatchDuplicate,
    DemoFixture,
    SourceFileNotFound,
    InvalidFileName,
    RenameFailed
};

/**
 * @brief Rules and template configuration for batch renaming.
 */
struct RenameRule
{
    QString pattern = QStringLiteral("{date}_{original}_{seq:3}");
    int startSequence = 1;
    int sequenceStep = 1;
    QString findText;
    QString replaceText;
    bool matchCase = false;
    CaseConversion caseConversion = CaseConversion::Unchanged;

    static RenameRule fromVariantMap(const QVariantMap &map);
    [[nodiscard]] QVariantMap toVariantMap() const;
};

/**
 * @brief Proposed rename operation for a specific catalog item.
 */
struct RenameCandidate
{
    QString itemId;
    int sourceIndex = -1;
    QString originalFilePath;
    QString originalFileName;
    QString proposedFileName;
    QString proposedFilePath;
    RenameStatus status = RenameStatus::Ok;
    QString statusMessage;
    bool hasSidecar = false;
    QString originalSidecarPath;
    QString proposedSidecarPath;

    [[nodiscard]] bool canRename() const { return status == RenameStatus::Ok; }
    [[nodiscard]] QVariantMap toVariantMap() const;
    static RenameCandidate fromVariantMap(const QVariantMap &map);
};

/**
 * @brief Summary results from executing a batch rename operation.
 */
struct BatchRenameResult
{
    int totalCount = 0;
    int successCount = 0;
    int failCount = 0;
    QList<RenameCandidate> successfulRenames;
    QList<RenameCandidate> failedRenames;
    QStringList errorMessages;

    [[nodiscard]] QVariantMap toVariantMap() const;
};

/**
 * @brief Batch photo renaming and file organization service.
 *
 * Supports tokenized template formatting ({date}, {original}, {seq:N}, {camera},
 * {rating}, etc.), find-and-replace, case conversion, pre-execution conflict
 * detection, synchronized companion XMP sidecar renaming, and non-destructive
 * error handling.
 */
class BatchRenamer : public QObject
{
    Q_OBJECT

public:
    explicit BatchRenamer(QObject *parent = nullptr);
    ~BatchRenamer() override = default;

    /// Generates preview of rename operations with collision and duplicate detection
    [[nodiscard]] static QList<RenameCandidate> generatePreview(const QList<CatalogItem> &items,
                                                               const RenameRule &rule);

    /// Formats a proposed filename using tokens, case conversion, and search/replace
    [[nodiscard]] static QString formatFileName(const CatalogItem &item,
                                                int sequenceNumber,
                                                const RenameRule &rule);

    /// Executes the renames on the filesystem (both images and companion .xmp sidecars)
    [[nodiscard]] static BatchRenameResult executeRename(const QList<RenameCandidate> &candidates);

    /// Sanitizes filename by removing/replacing illegal filesystem characters
    [[nodiscard]] static QString sanitizeFileName(const QString &fileName);

    /// Applies case conversion
    [[nodiscard]] static QString applyCaseConversion(const QString &str, CaseConversion conversion);
};

} // namespace PhotoCarta
