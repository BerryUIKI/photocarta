// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "catalogitem.h"

#include <QFutureWatcher>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <atomic>
#include <memory>

namespace PhotoCarta
{

/**
 * @brief Configuration settings for batch photo export.
 */
struct ExportSettings
{
    QString outputDirectory;
    QString format = QStringLiteral("original"); // "original", "jpg", "png", "webp"
    int quality = 90;                            // 1-100 (for jpg/webp)
    int maxDimension = 0;                        // 0 = no resizing; > 0 = scale aspect ratio so max(w,h) <= maxDimension
    bool preserveMetadata = true;
    QString filenamePrefix;
    QString filenameSuffix;
    bool overwriteExisting = false;
};

/**
 * @brief Summary of results from an export operation.
 */
struct ExportResult
{
    int totalCount = 0;
    int successCount = 0;
    int failCount = 0;
    QStringList exportedFiles;
    QStringList failedFiles;
    QStringList errorMessages;
};

/**
 * @brief High-performance asynchronous photo exporter service.
 *
 * Supports batch format conversion, resolution downscaling, lossless original
 * copying, and collision avoidance on the filesystem. Runs off the GUI thread
 * using QtConcurrent.
 */
class ImageExporter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isExporting READ isExporting NOTIFY exportStateChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int total READ total NOTIFY progressChanged)

public:
    explicit ImageExporter(QObject *parent = nullptr);
    ~ImageExporter() override;

    [[nodiscard]] bool isExporting() const;
    [[nodiscard]] int progress() const;
    [[nodiscard]] int total() const;

    /// Generates the output filename for an item given the settings and collision rules.
    [[nodiscard]] static QString generateOutputFilename(const QString &originalFileName,
                                                        const ExportSettings &settings,
                                                        const QString &targetDirectory);

    /// Synchronously exports a single item (safe for unit testing or worker execution).
    [[nodiscard]] static bool exportSingleItem(const CatalogItem &item,
                                               const ExportSettings &settings,
                                               QString *outExportedPath = nullptr,
                                               QString *outError = nullptr);

    /// Starts an asynchronous batch export in a background thread.
    bool startExport(const QList<CatalogItem> &items, const ExportSettings &settings);

    /// Requests cancellation of any ongoing export operation.
    void cancel();

signals:
    void exportStarted(int total);
    void itemExported(int index, const QString &sourcePath, const QString &outputPath);
    void itemFailed(int index, const QString &sourcePath, const QString &errorMessage);
    void progressChanged(int completed, int total);
    void exportStateChanged();
    void exportFinished(const PhotoCarta::ExportResult &result);
    void exportCanceled();

private:
    std::shared_ptr<std::atomic<bool>> m_cancelFlag;
    std::atomic<bool> m_isExporting{false};
    std::atomic<int> m_completed{0};
    int m_total = 0;
    QFutureWatcher<ExportResult> *m_watcher = nullptr;
};

} // namespace PhotoCarta

Q_DECLARE_METATYPE(PhotoCarta::ExportSettings)
Q_DECLARE_METATYPE(PhotoCarta::ExportResult)

