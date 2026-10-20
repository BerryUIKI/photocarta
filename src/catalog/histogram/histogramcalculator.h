// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QCache>
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVector>

namespace PhotoCarta
{

struct HistogramData
{
    bool valid = false;
    QString filePath;
    QVector<double> red;       // 256 normalized values (0.0 to 1.0)
    QVector<double> green;     // 256 normalized values
    QVector<double> blue;      // 256 normalized values
    QVector<double> luminance; // 256 normalized values
    int maxBinCount = 0;
    double meanLuminance = 0.0;            // 0.0 to 1.0
    double clippedShadowsPercent = 0.0;    // percentage of pixels with luma == 0
    double clippedHighlightsPercent = 0.0; // percentage of pixels with luma == 255
    int totalPixels = 0;

    [[nodiscard]] QVariantMap toVariantMap() const;
};

class HistogramCalculator : public QObject
{
    Q_OBJECT

public:
    explicit HistogramCalculator(QObject *parent = nullptr);
    ~HistogramCalculator() override;

    [[nodiscard]] static HistogramData computeFromImage(const QImage &image, const QString &id = QString());
    [[nodiscard]] HistogramData computeFromFile(const QString &filePath, int maxDimension = 512);

    Q_INVOKABLE void requestHistogram(const QString &filePath);
    [[nodiscard]] Q_INVOKABLE QVariantMap getCachedHistogram(const QString &filePath) const;
    [[nodiscard]] Q_INVOKABLE bool hasCachedHistogram(const QString &filePath) const;
    Q_INVOKABLE void clearCache();

signals:
    void histogramReady(const QString &filePath, const QVariantMap &data);

private:
    void onCalculationFinished(const QString &filePath, const HistogramData &data);

    mutable QMutex m_cacheMutex;
    QCache<QString, HistogramData> m_cache;
    QString m_currentRequestedPath;
};

} // namespace PhotoCarta
