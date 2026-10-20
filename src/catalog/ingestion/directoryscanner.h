// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "catalogitem.h"

#include <QAtomicInt>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

namespace PhotoCarta
{

class DirectoryScanner : public QObject
{
    Q_OBJECT

public:
    explicit DirectoryScanner(QObject *parent = nullptr);
    ~DirectoryScanner() override;

    [[nodiscard]] static const QSet<QString> &supportedExtensions();
    [[nodiscard]] static bool isSupportedImageFile(const QString &filePath);
    [[nodiscard]] static CatalogItem inspectFile(const QString &filePath);

    [[nodiscard]] bool isScanning() const;

public slots:
    void startScan(const QString &directoryPath, bool recursive = true);
    void cancel();

    // Synchronous execution for tests or background jobs
    [[nodiscard]] QList<CatalogItem> scanSynchronously(const QString &directoryPath, bool recursive = true);

signals:
    void scanStarted(const QString &directoryPath);
    void batchDiscovered(const QList<CatalogItem> &items);
    void progress(int count, const QString &currentFile);
    void scanFinished(int totalFound, qint64 elapsedMs);
    void scanCanceled();

private:
    void doScan(const QString &directoryPath, bool recursive);

    QAtomicInt m_isScanning{0};
    QAtomicInt m_cancelRequested{0};
};

} // namespace PhotoCarta
