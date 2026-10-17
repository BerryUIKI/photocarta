// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogmodel.h"
#include "catalog/histogram/histogramcalculator.h"
#include "catalog/selectionmanager.h"
#include "catalog/sessioncontroller.h"

#include <QDir>
#include <QImage>
#include <QPainter>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

class TestHistogramCalculator : public QObject
{
    Q_OBJECT

private slots:
    void testPureBlackImage();
    void testPureWhiteImage();
    void testColorSegregation();
    void testLinearGradient();
    void testFileComputationAndCache();
    void testSessionControllerHistogramIntegration();
};

void TestHistogramCalculator::testPureBlackImage()
{
    QImage blackImg(16, 16, QImage::Format_RGB32);
    blackImg.fill(Qt::black);

    const HistogramData data = HistogramCalculator::computeFromImage(blackImg, QStringLiteral("black"));
    QVERIFY(data.valid);
    QCOMPARE(data.totalPixels, 256);
    QCOMPARE(data.meanLuminance, 0.0);
    QCOMPARE(data.clippedShadowsPercent, 100.0);
    QCOMPARE(data.clippedHighlightsPercent, 0.0);

    QCOMPARE(data.red.size(), 256);
    QCOMPARE(data.green.size(), 256);
    QCOMPARE(data.blue.size(), 256);
    QCOMPARE(data.luminance.size(), 256);

    QCOMPARE(data.red.at(0), 1.0);
    QCOMPARE(data.green.at(0), 1.0);
    QCOMPARE(data.blue.at(0), 1.0);
    QCOMPARE(data.luminance.at(0), 1.0);

    for (int i = 1; i < 256; ++i)
    {
        QCOMPARE(data.red.at(i), 0.0);
        QCOMPARE(data.green.at(i), 0.0);
        QCOMPARE(data.blue.at(i), 0.0);
        QCOMPARE(data.luminance.at(i), 0.0);
    }

    const QVariantMap map = data.toVariantMap();
    QCOMPARE(map.value(QStringLiteral("valid")).toBool(), true);
    QCOMPARE(map.value(QStringLiteral("clippedShadowsPercent")).toDouble(), 100.0);
}

void TestHistogramCalculator::testPureWhiteImage()
{
    QImage whiteImg(16, 16, QImage::Format_RGB32);
    whiteImg.fill(Qt::white);

    const HistogramData data = HistogramCalculator::computeFromImage(whiteImg, QStringLiteral("white"));
    QVERIFY(data.valid);
    QCOMPARE(data.totalPixels, 256);
    QCOMPARE(data.meanLuminance, 1.0);
    QCOMPARE(data.clippedShadowsPercent, 0.0);
    QCOMPARE(data.clippedHighlightsPercent, 100.0);

    QCOMPARE(data.red.at(255), 1.0);
    QCOMPARE(data.green.at(255), 1.0);
    QCOMPARE(data.blue.at(255), 1.0);
    QCOMPARE(data.luminance.at(255), 1.0);

    for (int i = 0; i < 255; ++i)
    {
        QCOMPARE(data.red.at(i), 0.0);
        QCOMPARE(data.green.at(i), 0.0);
        QCOMPARE(data.blue.at(i), 0.0);
        QCOMPARE(data.luminance.at(i), 0.0);
    }
}

void TestHistogramCalculator::testColorSegregation()
{
    // Pure Red image
    QImage redImg(8, 8, QImage::Format_RGB32);
    redImg.fill(QColor(255, 0, 0));

    const HistogramData data = HistogramCalculator::computeFromImage(redImg, QStringLiteral("red"));
    QVERIFY(data.valid);

    // Red bin 255 is peak
    QCOMPARE(data.red.at(255), 1.0);
    QCOMPARE(data.red.at(0), 0.0);

    // Green and Blue are at bin 0
    QCOMPARE(data.green.at(0), 1.0);
    QCOMPARE(data.green.at(255), 0.0);
    QCOMPARE(data.blue.at(0), 1.0);
    QCOMPARE(data.blue.at(255), 0.0);

    // Luma = 0.299 * 255 = ~76
    const int expectedLuma = std::clamp(static_cast<int>(0.299 * 255 + 0.5), 0, 255);
    QCOMPARE(data.luminance.at(expectedLuma), 1.0);
}

void TestHistogramCalculator::testLinearGradient()
{
    // 256x1 grayscale gradient with exactly 1 pixel per value (0 to 255)
    QImage gradImg(256, 1, QImage::Format_RGB32);
    for (int x = 0; x < 256; ++x)
    {
        gradImg.setPixelColor(x, 0, QColor(x, x, x));
    }

    const HistogramData data = HistogramCalculator::computeFromImage(gradImg, QStringLiteral("gradient"));
    QVERIFY(data.valid);
    QCOMPARE(data.totalPixels, 256);

    for (int i = 0; i < 256; ++i)
    {
        QCOMPARE(data.luminance.at(i), 1.0);
        QCOMPARE(data.red.at(i), 1.0);
        QCOMPARE(data.green.at(i), 1.0);
        QCOMPARE(data.blue.at(i), 1.0);
    }

    // Mean should be around 0.5 (127.5 / 255 = 0.5)
    QVERIFY(std::abs(data.meanLuminance - 0.5) < 0.01);
    // 1 pixel out of 256 = 100 / 256 ~ 0.39%
    QVERIFY(std::abs(data.clippedShadowsPercent - (100.0 / 256.0)) < 0.01);
    QVERIFY(std::abs(data.clippedHighlightsPercent - (100.0 / 256.0)) < 0.01);
}

void TestHistogramCalculator::testFileComputationAndCache()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString testPath = tempDir.filePath(QStringLiteral("test_histogram.png"));
    QImage testImg(64, 64, QImage::Format_RGB32);
    testImg.fill(QColor(100, 150, 200));
    QVERIFY(testImg.save(testPath));

    HistogramCalculator calc;
    QCOMPARE(calc.hasCachedHistogram(testPath), false);

    // Synchronous file calculation
    const HistogramData data = calc.computeFromFile(testPath, 512);
    QVERIFY(data.valid);
    QCOMPARE(data.totalPixels, 64 * 64);

    // Request histogram (tests caching & async watcher)
    QSignalSpy readySpy(&calc, &HistogramCalculator::histogramReady);
    calc.requestHistogram(testPath);

    QVERIFY(readySpy.wait(3000));
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(calc.hasCachedHistogram(testPath), true);

    const QVariantMap cached = calc.getCachedHistogram(testPath);
    QCOMPARE(cached.value(QStringLiteral("valid")).toBool(), true);

    calc.clearCache();
    QCOMPARE(calc.hasCachedHistogram(testPath), false);
}

void TestHistogramCalculator::testSessionControllerHistogramIntegration()
{
    CatalogModel model;
    CatalogFilterModel filter;
    filter.setSourceModel(&model);
    SelectionManager selection(&filter);
    SessionController controller(&model, &filter, &selection);

    QCOMPARE(controller.histogramChannel(), QStringLiteral("rgb"));
    QCOMPARE(controller.histogramLogScale(), false);
    QCOMPARE(controller.histogramVisible(), true);

    // Toggle channel
    QSignalSpy chSpy(&controller, &SessionController::histogramChannelChanged);
    controller.setHistogramChannel(QStringLiteral("luma"));
    QCOMPARE(controller.histogramChannel(), QStringLiteral("luma"));
    QCOMPARE(chSpy.count(), 1);

    // Toggle log scale
    QSignalSpy logSpy(&controller, &SessionController::histogramLogScaleChanged);
    controller.toggleHistogramLogScale();
    QCOMPARE(controller.histogramLogScale(), true);
    QCOMPARE(logSpy.count(), 1);

    // Toggle visibility
    QSignalSpy visSpy(&controller, &SessionController::histogramVisibleChanged);
    controller.toggleHistogramVisible();
    QCOMPARE(controller.histogramVisible(), false);
    QCOMPARE(visSpy.count(), 1);

    // Load demo fixtures and verify histogram calculation
    model.loadDemoFixtures();
    QVERIFY(model.count() > 0);

    QSignalSpy histSpy(&controller, &SessionController::currentHistogramChanged);
    selection.select(0);

    // Wait for async calculation if not yet ready
    if (!controller.currentHistogram().value(QStringLiteral("valid")).toBool())
    {
        histSpy.wait(3000);
    }

    const QVariantMap hist = controller.currentHistogram();
    QVERIFY(hist.value(QStringLiteral("valid")).toBool());
    QVERIFY(hist.value(QStringLiteral("red")).toList().size() == 256);
}

QTEST_MAIN(TestHistogramCalculator)
#include "test_histogram_calculator.moc"
