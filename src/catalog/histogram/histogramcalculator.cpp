// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "histogramcalculator.h"

#include <QFutureWatcher>
#include <QImageReader>
#include <QMutexLocker>
#include <QtConcurrent/QtConcurrent>

#include <algorithm>
#include <cmath>

namespace PhotoCarta
{

QVariantMap HistogramData::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("valid"), valid);
    map.insert(QStringLiteral("filePath"), filePath);
    map.insert(QStringLiteral("maxBinCount"), maxBinCount);
    map.insert(QStringLiteral("meanLuminance"), meanLuminance);
    map.insert(QStringLiteral("clippedShadowsPercent"), clippedShadowsPercent);
    map.insert(QStringLiteral("clippedHighlightsPercent"), clippedHighlightsPercent);
    map.insert(QStringLiteral("totalPixels"), totalPixels);

    QVariantList rList;
    rList.reserve(red.size());
    for (double v : red)
    {
        rList.append(v);
    }

    QVariantList gList;
    gList.reserve(green.size());
    for (double v : green)
    {
        gList.append(v);
    }

    QVariantList bList;
    bList.reserve(blue.size());
    for (double v : blue)
    {
        bList.append(v);
    }

    QVariantList lList;
    lList.reserve(luminance.size());
    for (double v : luminance)
    {
        lList.append(v);
    }

    map.insert(QStringLiteral("red"), rList);
    map.insert(QStringLiteral("green"), gList);
    map.insert(QStringLiteral("blue"), bList);
    map.insert(QStringLiteral("luminance"), lList);

    return map;
}

HistogramCalculator::HistogramCalculator(QObject *parent)
    : QObject(parent)
    , m_cache(64)
{
}

HistogramCalculator::~HistogramCalculator() = default;

HistogramData HistogramCalculator::computeFromImage(const QImage &image, const QString &id)
{
    HistogramData data;
    data.filePath = id;

    if (image.isNull() || image.width() <= 0 || image.height() <= 0)
    {
        return data;
    }

    const QImage converted = (image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32_Premultiplied)
                                 ? image
                                 : image.convertToFormat(QImage::Format_RGB32);

    const int w = converted.width();
    const int h = converted.height();
    const int totalPixels = w * h;

    quint64 rBins[256] = {0};
    quint64 gBins[256] = {0};
    quint64 bBins[256] = {0};
    quint64 lBins[256] = {0};
    quint64 totalLuma = 0;

    for (int y = 0; y < h; ++y)
    {
        const auto *scanLine = reinterpret_cast<const QRgb *>(converted.constScanLine(y));
        for (int x = 0; x < w; ++x)
        {
            const QRgb pixel = scanLine[x];
            const int r = qRed(pixel);
            const int g = qGreen(pixel);
            const int b = qBlue(pixel);
            const int luma = std::clamp(static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b + 0.5), 0, 255);

            rBins[r]++;
            gBins[g]++;
            bBins[b]++;
            lBins[luma]++;
            totalLuma += luma;
        }
    }

    quint64 maxR = 0;
    quint64 maxG = 0;
    quint64 maxB = 0;
    quint64 maxL = 0;

    for (int i = 0; i < 256; ++i)
    {
        if (rBins[i] > maxR) maxR = rBins[i];
        if (gBins[i] > maxG) maxG = gBins[i];
        if (bBins[i] > maxB) maxB = bBins[i];
        if (lBins[i] > maxL) maxL = lBins[i];
    }

    data.valid = true;
    data.totalPixels = totalPixels;
    data.red.resize(256);
    data.green.resize(256);
    data.blue.resize(256);
    data.luminance.resize(256);

    const double normR = maxR > 0 ? (1.0 / static_cast<double>(maxR)) : 0.0;
    const double normG = maxG > 0 ? (1.0 / static_cast<double>(maxG)) : 0.0;
    const double normB = maxB > 0 ? (1.0 / static_cast<double>(maxB)) : 0.0;
    const double normL = maxL > 0 ? (1.0 / static_cast<double>(maxL)) : 0.0;

    for (int i = 0; i < 256; ++i)
    {
        data.red[i] = static_cast<double>(rBins[i]) * normR;
        data.green[i] = static_cast<double>(gBins[i]) * normG;
        data.blue[i] = static_cast<double>(bBins[i]) * normB;
        data.luminance[i] = static_cast<double>(lBins[i]) * normL;
    }

    data.maxBinCount = static_cast<int>(std::max({maxR, maxG, maxB, maxL}));
    data.meanLuminance = totalPixels > 0 ? (static_cast<double>(totalLuma) / (static_cast<double>(totalPixels) * 255.0)) : 0.0;
    data.clippedShadowsPercent = totalPixels > 0 ? (static_cast<double>(lBins[0]) / static_cast<double>(totalPixels) * 100.0) : 0.0;
    data.clippedHighlightsPercent = totalPixels > 0 ? (static_cast<double>(lBins[255]) / static_cast<double>(totalPixels) * 100.0) : 0.0;

    return data;
}

HistogramData HistogramCalculator::computeFromFile(const QString &filePath, int maxDimension)
{
    if (filePath.isEmpty())
    {
        return {};
    }

    QString cleanPath = filePath;
    if (cleanPath.startsWith(QLatin1String("file:///")))
    {
        cleanPath = cleanPath.mid(8);
#ifdef Q_OS_WIN
        if (cleanPath.startsWith(QLatin1Char('/')) && cleanPath.length() >= 3 && cleanPath.at(2) == QLatin1Char(':'))
        {
            cleanPath = cleanPath.mid(1);
        }
#endif
    }
    else if (cleanPath.startsWith(QLatin1String("file://")))
    {
        cleanPath = cleanPath.mid(7);
    }

    // Handle Qt Resource files (demo assets)
    if (cleanPath.startsWith(QLatin1String("qrc:/")))
    {
        cleanPath = cleanPath.mid(3); // becomes ":/assets/..."
    }

    if (cleanPath.startsWith(QLatin1String(":/")))
    {
        QImage resImg(cleanPath);
        if (!resImg.isNull())
        {
            if (maxDimension > 0 && (resImg.width() > maxDimension || resImg.height() > maxDimension))
            {
                resImg = resImg.scaled(maxDimension, maxDimension, Qt::KeepAspectRatio, Qt::FastTransformation);
            }
            return computeFromImage(resImg, filePath);
        }
    }

    QImageReader reader(cleanPath);
    if (reader.canRead())
    {
        const QSize origSize = reader.size();
        if (maxDimension > 0 && origSize.isValid() && (origSize.width() > maxDimension || origSize.height() > maxDimension))
        {
            const QSize target = origSize.scaled(maxDimension, maxDimension, Qt::KeepAspectRatio);
            reader.setScaledSize(target);
        }

        const QImage img = reader.read();
        if (!img.isNull())
        {
            return computeFromImage(img, filePath);
        }
    }

    // Fallback: direct QImage load
    QImage directImg(cleanPath);
    if (!directImg.isNull())
    {
        if (maxDimension > 0 && (directImg.width() > maxDimension || directImg.height() > maxDimension))
        {
            directImg = directImg.scaled(maxDimension, maxDimension, Qt::KeepAspectRatio, Qt::FastTransformation);
        }
        return computeFromImage(directImg, filePath);
    }

    return {};
}

void HistogramCalculator::requestHistogram(const QString &filePath)
{
    if (filePath.isEmpty())
    {
        return;
    }

    {
        QMutexLocker locker(&m_cacheMutex);
        if (m_cache.contains(filePath))
        {
            const HistogramData *cached = m_cache.object(filePath);
            if (cached)
            {
                emit histogramReady(filePath, cached->toVariantMap());
                return;
            }
        }
    }

    m_currentRequestedPath = filePath;

    auto *watcher = new QFutureWatcher<HistogramData>(this);
    connect(watcher, &QFutureWatcher<HistogramData>::finished, this, [this, watcher, filePath]() {
        const HistogramData data = watcher->result();
        watcher->deleteLater();
        onCalculationFinished(filePath, data);
    });

    QFuture<HistogramData> future = QtConcurrent::run([filePath]() {
        HistogramCalculator worker;
        return worker.computeFromFile(filePath);
    });

    watcher->setFuture(future);
}

void HistogramCalculator::onCalculationFinished(const QString &filePath, const HistogramData &data)
{
    {
        QMutexLocker locker(&m_cacheMutex);
        m_cache.insert(filePath, new HistogramData(data));
    }

    if (filePath == m_currentRequestedPath || m_currentRequestedPath.isEmpty())
    {
        emit histogramReady(filePath, data.toVariantMap());
    }
}

QVariantMap HistogramCalculator::getCachedHistogram(const QString &filePath) const
{
    QMutexLocker locker(&m_cacheMutex);
    if (m_cache.contains(filePath))
    {
        const HistogramData *cached = m_cache.object(filePath);
        if (cached)
        {
            return cached->toVariantMap();
        }
    }
    return {};
}

bool HistogramCalculator::hasCachedHistogram(const QString &filePath) const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.contains(filePath);
}

void HistogramCalculator::clearCache()
{
    QMutexLocker locker(&m_cacheMutex);
    m_cache.clear();
}

} // namespace PhotoCarta
