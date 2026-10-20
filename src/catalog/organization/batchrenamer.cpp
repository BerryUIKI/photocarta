// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "batchrenamer.h"
#include "metadata/xmpsidecar.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSet>

namespace PhotoCarta
{

Q_LOGGING_CATEGORY(lcRenamer, "photocarta.renamer")

RenameRule RenameRule::fromVariantMap(const QVariantMap &map)
{
    RenameRule rule;
    if (map.contains(QStringLiteral("pattern")))
    {
        rule.pattern = map.value(QStringLiteral("pattern")).toString();
    }
    if (map.contains(QStringLiteral("startSequence")))
    {
        rule.startSequence = map.value(QStringLiteral("startSequence")).toInt();
    }
    if (map.contains(QStringLiteral("sequenceStep")))
    {
        rule.sequenceStep = qMax(1, map.value(QStringLiteral("sequenceStep")).toInt());
    }
    if (map.contains(QStringLiteral("findText")))
    {
        rule.findText = map.value(QStringLiteral("findText")).toString();
    }
    if (map.contains(QStringLiteral("replaceText")))
    {
        rule.replaceText = map.value(QStringLiteral("replaceText")).toString();
    }
    if (map.contains(QStringLiteral("matchCase")))
    {
        rule.matchCase = map.value(QStringLiteral("matchCase")).toBool();
    }
    if (map.contains(QStringLiteral("caseConversion")))
    {
        rule.caseConversion = static_cast<CaseConversion>(map.value(QStringLiteral("caseConversion")).toInt());
    }
    return rule;
}

QVariantMap RenameRule::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("pattern"), pattern);
    map.insert(QStringLiteral("startSequence"), startSequence);
    map.insert(QStringLiteral("sequenceStep"), sequenceStep);
    map.insert(QStringLiteral("findText"), findText);
    map.insert(QStringLiteral("replaceText"), replaceText);
    map.insert(QStringLiteral("matchCase"), matchCase);
    map.insert(QStringLiteral("caseConversion"), static_cast<int>(caseConversion));
    return map;
}

QVariantMap RenameCandidate::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("itemId"), itemId);
    map.insert(QStringLiteral("sourceIndex"), sourceIndex);
    map.insert(QStringLiteral("originalFilePath"), originalFilePath);
    map.insert(QStringLiteral("originalFileName"), originalFileName);
    map.insert(QStringLiteral("proposedFileName"), proposedFileName);
    map.insert(QStringLiteral("proposedFilePath"), proposedFilePath);
    map.insert(QStringLiteral("status"), static_cast<int>(status));
    map.insert(QStringLiteral("statusMessage"), statusMessage);
    map.insert(QStringLiteral("canRename"), canRename());
    map.insert(QStringLiteral("hasSidecar"), hasSidecar);
    map.insert(QStringLiteral("originalSidecarPath"), originalSidecarPath);
    map.insert(QStringLiteral("proposedSidecarPath"), proposedSidecarPath);
    return map;
}

RenameCandidate RenameCandidate::fromVariantMap(const QVariantMap &map)
{
    RenameCandidate c;
    c.itemId = map.value(QStringLiteral("itemId")).toString();
    c.sourceIndex = map.value(QStringLiteral("sourceIndex")).toInt();
    c.originalFilePath = map.value(QStringLiteral("originalFilePath")).toString();
    c.originalFileName = map.value(QStringLiteral("originalFileName")).toString();
    c.proposedFileName = map.value(QStringLiteral("proposedFileName")).toString();
    c.proposedFilePath = map.value(QStringLiteral("proposedFilePath")).toString();
    c.status = static_cast<RenameStatus>(map.value(QStringLiteral("status")).toInt());
    c.statusMessage = map.value(QStringLiteral("statusMessage")).toString();
    c.hasSidecar = map.value(QStringLiteral("hasSidecar")).toBool();
    c.originalSidecarPath = map.value(QStringLiteral("originalSidecarPath")).toString();
    c.proposedSidecarPath = map.value(QStringLiteral("proposedSidecarPath")).toString();
    return c;
}

QVariantMap BatchRenameResult::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("totalCount"), totalCount);
    map.insert(QStringLiteral("successCount"), successCount);
    map.insert(QStringLiteral("failCount"), failCount);
    map.insert(QStringLiteral("errorMessages"), errorMessages);

    QVariantList succList;
    for (const auto &c : successfulRenames)
    {
        succList.append(c.toVariantMap());
    }
    map.insert(QStringLiteral("successfulRenames"), succList);

    QVariantList failList;
    for (const auto &c : failedRenames)
    {
        failList.append(c.toVariantMap());
    }
    map.insert(QStringLiteral("failedRenames"), failList);
    return map;
}

BatchRenamer::BatchRenamer(QObject *parent)
    : QObject(parent)
{
}

QString BatchRenamer::sanitizeFileName(const QString &fileName)
{
    QString sanitized = fileName;
    // Replace characters invalid across Windows, macOS, and Linux
    static const QRegularExpression invalidChars(QStringLiteral(R"([\\/:*?"<>|])"));
    sanitized.replace(invalidChars, QStringLiteral("_"));

    // Trim surrounding spaces and dots
    sanitized = sanitized.trimmed();
    while (sanitized.endsWith(QLatin1Char('.')))
    {
        sanitized.chop(1);
    }
    return sanitized;
}

QString BatchRenamer::applyCaseConversion(const QString &str, CaseConversion conversion)
{
    switch (conversion)
    {
    case CaseConversion::Lowercase:
        return str.toLower();
    case CaseConversion::Uppercase:
        return str.toUpper();
    case CaseConversion::TitleCase:
    {
        QString result = str;
        bool capitalizeNext = true;
        for (int i = 0; i < result.size(); ++i)
        {
            const QChar ch = result.at(i);
            if (ch.isSpace() || ch == QLatin1Char('_') || ch == QLatin1Char('-'))
            {
                capitalizeNext = true;
            }
            else if (capitalizeNext && ch.isLetter())
            {
                result[i] = ch.toUpper();
                capitalizeNext = false;
            }
            else if (ch.isLetter())
            {
                result[i] = ch.toLower();
            }
        }
        return result;
    }
    case CaseConversion::Unchanged:
    default:
        return str;
    }
}

QString BatchRenamer::formatFileName(const CatalogItem &item,
                                     int sequenceNumber,
                                     const RenameRule &rule)
{
    const QFileInfo fi(item.filePath);
    const QString originalBase = fi.completeBaseName().isEmpty() ? fi.baseName() : fi.completeBaseName();
    const QString originalExt = fi.suffix();

    QDateTime date = item.dateTaken;
    if (!date.isValid())
    {
        date = item.fileLastModified;
    }
    if (!date.isValid())
    {
        date = QDateTime::currentDateTime();
    }

    QString formatted = rule.pattern;

    // 1. Date and Time tokens
    formatted.replace(QStringLiteral("{YYYY}"), date.toString(QStringLiteral("yyyy")));
    formatted.replace(QStringLiteral("{YY}"), date.toString(QStringLiteral("yy")));
    formatted.replace(QStringLiteral("{MM}"), date.toString(QStringLiteral("MM")));
    formatted.replace(QStringLiteral("{DD}"), date.toString(QStringLiteral("dd")));
    formatted.replace(QStringLiteral("{hh}"), date.toString(QStringLiteral("HH")));
    formatted.replace(QStringLiteral("{mm}"), date.toString(QStringLiteral("mm")));
    formatted.replace(QStringLiteral("{ss}"), date.toString(QStringLiteral("ss")));
    formatted.replace(QStringLiteral("{date}"), date.toString(QStringLiteral("yyyyMMdd")));
    formatted.replace(QStringLiteral("{time}"), date.toString(QStringLiteral("HHmmss")));

    // 2. Original filename tokens
    formatted.replace(QStringLiteral("{original}"), originalBase);
    formatted.replace(QStringLiteral("{name}"), originalBase);
    formatted.replace(QStringLiteral("{ext}"), originalExt);

    // 3. Sequence tokens: {seq} or {seq:N}
    static const QRegularExpression seqRegex(QStringLiteral(R"(\{seq(?::(\d+))?\})"));
    QRegularExpressionMatch seqMatch = seqRegex.match(formatted);
    while (seqMatch.hasMatch())
    {
        const QString padStr = seqMatch.captured(1);
        int pad = padStr.isEmpty() ? 1 : padStr.toInt();
        pad = qBound(1, pad, 10);
        const QString seqFormatted = QStringLiteral("%1").arg(sequenceNumber, pad, 10, QLatin1Char('0'));
        formatted.replace(seqMatch.capturedStart(), seqMatch.capturedLength(), seqFormatted);
        seqMatch = seqRegex.match(formatted);
    }

    // 4. Metadata tokens
    formatted.replace(QStringLiteral("{camera}"), sanitizeFileName(item.cameraModel));
    formatted.replace(QStringLiteral("{lens}"), sanitizeFileName(item.lensModel));
    formatted.replace(QStringLiteral("{rating}"), QString::number(item.rating));
    formatted.replace(QStringLiteral("{label}"), XmpSidecar::colorLabelToString(item.colorLabel));
    formatted.replace(QStringLiteral("{title}"), sanitizeFileName(item.title));

    // 5. Search and Replace
    if (!rule.findText.isEmpty())
    {
        const Qt::CaseSensitivity cs = rule.matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
        formatted.replace(rule.findText, rule.replaceText, cs);
    }

    // 6. Case Conversion
    formatted = applyCaseConversion(formatted, rule.caseConversion);

    // 7. Sanitize result
    formatted = sanitizeFileName(formatted);

    // 8. Ensure extension is preserved if pattern didn't explicitly include it
    if (!originalExt.isEmpty())
    {
        const QString dotExt = QStringLiteral(".") + originalExt;
        if (!formatted.endsWith(dotExt, Qt::CaseInsensitive))
        {
            formatted += dotExt;
        }
    }

    return formatted;
}

QList<RenameCandidate> BatchRenamer::generatePreview(const QList<CatalogItem> &items,
                                                    const RenameRule &rule)
{
    QList<RenameCandidate> candidates;
    candidates.reserve(items.size());

    QSet<QString> proposedPathsInBatch;
    int currentSeq = rule.startSequence;

    for (int i = 0; i < items.size(); ++i)
    {
        const CatalogItem &item = items.at(i);
        RenameCandidate candidate;
        candidate.itemId = item.id;
        candidate.sourceIndex = i;
        candidate.originalFilePath = item.filePath;
        candidate.originalFileName = item.fileName.isEmpty() ? QFileInfo(item.filePath).fileName() : item.fileName;

        // Check if demo fixture
        if (item.isDemoFixture || item.filePath.startsWith(QStringLiteral("qrc:/")))
        {
            candidate.proposedFileName = candidate.originalFileName;
            candidate.proposedFilePath = candidate.originalFilePath;
            candidate.status = RenameStatus::DemoFixture;
            candidate.statusMessage = QStringLiteral("Demo fixtures cannot be renamed on disk");
            candidates.append(candidate);
            currentSeq += rule.sequenceStep;
            continue;
        }

        // Check if source file exists
        const QFileInfo sourceFi(item.filePath);
        if (!sourceFi.exists())
        {
            candidate.proposedFileName = candidate.originalFileName;
            candidate.proposedFilePath = candidate.originalFilePath;
            candidate.status = RenameStatus::SourceFileNotFound;
            candidate.statusMessage = QStringLiteral("Source file not found on disk");
            candidates.append(candidate);
            currentSeq += rule.sequenceStep;
            continue;
        }

        // Generate proposed filename
        const QString proposedName = formatFileName(item, currentSeq, rule);
        candidate.proposedFileName = proposedName;

        if (proposedName.isEmpty())
        {
            candidate.proposedFilePath = candidate.originalFilePath;
            candidate.status = RenameStatus::InvalidFileName;
            candidate.statusMessage = QStringLiteral("Generated filename is empty or invalid");
            candidates.append(candidate);
            currentSeq += rule.sequenceStep;
            continue;
        }

        const QDir dir = sourceFi.dir();
        const QString proposedPath = QDir::fromNativeSeparators(dir.filePath(proposedName));
        candidate.proposedFilePath = proposedPath;

        // Detect companion XMP sidecar
        const QString existingSidecar = XmpSidecar::sidecarPathForImage(item.filePath, true);
        if (!existingSidecar.isEmpty() && QFile::exists(existingSidecar))
        {
            candidate.hasSidecar = true;
            candidate.originalSidecarPath = existingSidecar;

            // Maintain sidecar naming style:
            // If original was <file>.<ext>.xmp -> proposed is <newfile>.<ext>.xmp
            // If original was <file>.xmp -> proposed is <newfile>.xmp
            if (existingSidecar.endsWith(QStringLiteral(".xmp"), Qt::CaseInsensitive))
            {
                const QString baseSidecar = QFileInfo(existingSidecar).completeBaseName();
                if (baseSidecar.endsWith(sourceFi.suffix(), Qt::CaseInsensitive))
                {
                    // Style: <name>.<ext>.xmp
                    candidate.proposedSidecarPath = dir.filePath(proposedName + QStringLiteral(".xmp"));
                }
                else
                {
                    // Style: <basename>.xmp
                    const QString newBase = QFileInfo(proposedName).completeBaseName();
                    candidate.proposedSidecarPath = dir.filePath(newBase + QStringLiteral(".xmp"));
                }
            }
        }

        // Check for collisions
        const QString normOriginal = QDir::fromNativeSeparators(item.filePath);
        const QString normProposed = QDir::fromNativeSeparators(proposedPath);

        if (normOriginal.compare(normProposed, Qt::CaseInsensitive) == 0)
        {
            // Same name, valid no-op
            candidate.status = RenameStatus::Ok;
            candidate.statusMessage = QStringLiteral("Unchanged");
        }
        else if (proposedPathsInBatch.contains(normProposed))
        {
            candidate.status = RenameStatus::ConflictBatchDuplicate;
            candidate.statusMessage = QStringLiteral("Duplicate target filename generated in current batch");
        }
        else if (QFile::exists(normProposed))
        {
            candidate.status = RenameStatus::ConflictFileExists;
            candidate.statusMessage = QStringLiteral("Target file already exists on disk");
        }
        else
        {
            candidate.status = RenameStatus::Ok;
            candidate.statusMessage = QStringLiteral("OK");
            proposedPathsInBatch.insert(normProposed);
        }

        candidates.append(candidate);
        currentSeq += rule.sequenceStep;
    }

    return candidates;
}

BatchRenameResult BatchRenamer::executeRename(const QList<RenameCandidate> &candidates)
{
    BatchRenameResult result;
    result.totalCount = candidates.size();

    for (const auto &candidate : candidates)
    {
        if (!candidate.canRename())
        {
            result.failCount++;
            result.failedRenames.append(candidate);
            result.errorMessages.append(QStringLiteral("%1: %2").arg(candidate.originalFileName, candidate.statusMessage));
            continue;
        }

        // If file name unchanged, count as success without filesystem action
        if (candidate.originalFilePath.compare(candidate.proposedFilePath, Qt::CaseInsensitive) == 0)
        {
            result.successCount++;
            result.successfulRenames.append(candidate);
            continue;
        }

        // Rename main photo file
        if (!QFile::rename(candidate.originalFilePath, candidate.proposedFilePath))
        {
            result.failCount++;
            RenameCandidate failed = candidate;
            failed.status = RenameStatus::RenameFailed;
            failed.statusMessage = QStringLiteral("Filesystem rename failed");
            result.failedRenames.append(failed);
            result.errorMessages.append(QStringLiteral("Failed to rename file '%1' to '%2'")
                                            .arg(candidate.originalFilePath, candidate.proposedFilePath));
            continue;
        }

        // Synchronously rename companion XMP sidecar if present
        if (candidate.hasSidecar && !candidate.originalSidecarPath.isEmpty() && !candidate.proposedSidecarPath.isEmpty())
        {
            if (QFile::exists(candidate.originalSidecarPath))
            {
                if (!QFile::rename(candidate.originalSidecarPath, candidate.proposedSidecarPath))
                {
                    qCWarning(lcRenamer) << "Failed to rename companion sidecar from"
                                         << candidate.originalSidecarPath << "to"
                                         << candidate.proposedSidecarPath;
                }
            }
        }

        result.successCount++;
        result.successfulRenames.append(candidate);
    }

    return result;
}

} // namespace PhotoCarta
