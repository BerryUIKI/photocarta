// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogitem.h"
#include "catalog/export/imageexporter.h"
#include "catalog/catalogmodel.h"
#include "catalog/catalogfiltermodel.h"
#include "catalog/selectionmanager.h"
#include "catalog/sessioncontroller.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

class TestImageExporter : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testFilenameGeneration();
    void testFilenameCollisionAvoidance();
    void testExportSingleItemConversion();
    void testExportWithDownscaling();
    void testBatchExportAsync();
    void testExportCancellation();
    void testSessionControllerExport();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_sourceImagePath;
};

void TestImageExporter::initTestCase()
{
    qRegisterMetaType<PhotoCarta::ExportResult>("PhotoCarta::ExportResult");
    qRegisterMetaType<PhotoCarta::ExportResult>("ExportResult");
    qRegisterMetaType<PhotoCarta::ExportSettings>("PhotoCarta::ExportSettings");
    qRegisterMetaType<PhotoCarta::ExportSettings>("ExportSettings");

    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    // Create a real source test image (1200 x 800 RGB)
    m_sourceImagePath = m_tempDir->filePath(QStringLiteral("sample_photo.jpg"));
    QImage testImg(1200, 800, QImage::Format_RGB32);
    testImg.fill(Qt::blue);
    QVERIFY(testImg.save(m_sourceImagePath, "JPG", 95));
}

void TestImageExporter::cleanupTestCase()
{
    m_tempDir.reset();
}

void TestImageExporter::testFilenameGeneration()
{
    ExportSettings settings;
    settings.format = QStringLiteral("jpg");
    settings.filenamePrefix = QStringLiteral("web_");
    settings.filenameSuffix = QStringLiteral("_hq");

    const QString outName = ImageExporter::generateOutputFilename(QStringLiteral("Sunset.png"), settings, QString());
    QCOMPARE(outName, QStringLiteral("web_Sunset_hq.jpg"));

    settings.format = QStringLiteral("png");
    const QString outPng = ImageExporter::generateOutputFilename(QStringLiteral("Portrait.jpg"), settings, QString());
    QCOMPARE(outPng, QStringLiteral("web_Portrait_hq.png"));

    settings.format = QStringLiteral("original");
    const QString outOrig = ImageExporter::generateOutputFilename(QStringLiteral("Landscape.RAW"), settings, QString());
    QCOMPARE(outOrig, QStringLiteral("web_Landscape_hq.raw"));
}

void TestImageExporter::testFilenameCollisionAvoidance()
{
    const QString targetDir = m_tempDir->filePath(QStringLiteral("collision_test"));
    QDir().mkpath(targetDir);

    // Create existing file
    QFile f(targetDir + QStringLiteral("/IMG_0001.jpg"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("existing");
    f.close();

    ExportSettings settings;
    settings.format = QStringLiteral("jpg");
    settings.overwriteExisting = false;

    const QString nonColliding = ImageExporter::generateOutputFilename(QStringLiteral("IMG_0001.jpg"), settings, targetDir);
    QCOMPARE(nonColliding, QStringLiteral("IMG_0001 (1).jpg"));

    // If overwrite is true, it should return original name
    settings.overwriteExisting = true;
    const QString overwriteName = ImageExporter::generateOutputFilename(QStringLiteral("IMG_0001.jpg"), settings, targetDir);
    QCOMPARE(overwriteName, QStringLiteral("IMG_0001.jpg"));
}

void TestImageExporter::testExportSingleItemConversion()
{
    CatalogItem item;
    item.id = QStringLiteral("test-01");
    item.filePath = m_sourceImagePath;
    item.fileName = QStringLiteral("sample_photo.jpg");

    const QString outDir = m_tempDir->filePath(QStringLiteral("export_single"));

    ExportSettings settings;
    settings.outputDirectory = outDir;
    settings.format = QStringLiteral("png");
    settings.quality = 90;

    QString exportedPath;
    QString errorMsg;
    const bool ok = ImageExporter::exportSingleItem(item, settings, &exportedPath, &errorMsg);
    QVERIFY2(ok, qPrintable(errorMsg));
    QVERIFY(QFile::exists(exportedPath));
    QVERIFY(exportedPath.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive));

    QImage loaded(exportedPath);
    QVERIFY(!loaded.isNull());
    QCOMPARE(loaded.width(), 1200);
    QCOMPARE(loaded.height(), 800);
}

void TestImageExporter::testExportWithDownscaling()
{
    CatalogItem item;
    item.id = QStringLiteral("test-scale");
    item.filePath = m_sourceImagePath;
    item.fileName = QStringLiteral("sample_photo.jpg");

    const QString outDir = m_tempDir->filePath(QStringLiteral("export_scale"));

    ExportSettings settings;
    settings.outputDirectory = outDir;
    settings.format = QStringLiteral("jpg");
    settings.maxDimension = 600; // max width or height 600
    settings.quality = 85;

    QString exportedPath;
    QString errorMsg;
    const bool ok = ImageExporter::exportSingleItem(item, settings, &exportedPath, &errorMsg);
    QVERIFY2(ok, qPrintable(errorMsg));
    QVERIFY(QFile::exists(exportedPath));

    QImage loaded(exportedPath);
    QVERIFY(!loaded.isNull());
    QCOMPARE(loaded.width(), 600);
    QCOMPARE(loaded.height(), 400); // 1200x800 scaled by 0.5
}

void TestImageExporter::testBatchExportAsync()
{
    QList<CatalogItem> items;
    for (int i = 0; i < 3; ++i)
    {
        CatalogItem it;
        it.id = QString::number(i + 1);
        it.filePath = m_sourceImagePath;
        it.fileName = QStringLiteral("photo_%1.jpg").arg(i + 1);
        items.append(it);
    }

    const QString outDir = m_tempDir->filePath(QStringLiteral("export_batch"));

    ExportSettings settings;
    settings.outputDirectory = outDir;
    settings.format = QStringLiteral("jpg");
    settings.maxDimension = 300;

    ImageExporter exporter;
    QSignalSpy spyStarted(&exporter, &ImageExporter::exportStarted);
    QSignalSpy spyProgress(&exporter, &ImageExporter::progressChanged);
    QSignalSpy spyFinished(&exporter, &ImageExporter::exportFinished);

    QVERIFY(exporter.startExport(items, settings));
    QVERIFY(exporter.isExporting());
    QCOMPARE(spyStarted.count(), 1);

    QVERIFY(spyFinished.wait(5000));
    QCOMPARE(spyFinished.count(), 1);
    QVERIFY(!exporter.isExporting());
    QCOMPARE(exporter.progress(), 3);
    QCOMPARE(exporter.total(), 3);

    const auto arguments = spyFinished.takeFirst();
    const ExportResult result = arguments.at(0).value<ExportResult>();
    QCOMPARE(result.totalCount, 3);
    QCOMPARE(result.successCount, 3);
    QCOMPARE(result.failCount, 0);
    QCOMPARE(result.exportedFiles.size(), 3);
}

void TestImageExporter::testExportCancellation()
{
    QList<CatalogItem> items;
    for (int i = 0; i < 10; ++i)
    {
        CatalogItem it;
        it.id = QString::number(i + 1);
        it.filePath = m_sourceImagePath;
        it.fileName = QStringLiteral("cancel_photo_%1.jpg").arg(i + 1);
        items.append(it);
    }

    const QString outDir = m_tempDir->filePath(QStringLiteral("export_cancel"));
    ExportSettings settings;
    settings.outputDirectory = outDir;

    ImageExporter exporter;
    QSignalSpy spyFinished(&exporter, &ImageExporter::exportFinished);
    QSignalSpy spyCanceled(&exporter, &ImageExporter::exportCanceled);

    QVERIFY(exporter.startExport(items, settings));
    exporter.cancel();

    // Either exportCanceled or exportFinished with early stop
    QTRY_VERIFY_WITH_TIMEOUT(!exporter.isExporting(), 3000);
    QVERIFY(spyCanceled.count() > 0 || spyFinished.count() > 0);
}

void TestImageExporter::testSessionControllerExport()
{
    CatalogModel model;
    CatalogFilterModel filterModel;
    filterModel.setSourceModel(&model);
    SelectionManager selManager(&filterModel);
    SessionController controller(&model, &filterModel, &selManager);

    // Initial state
    QCOMPARE(controller.isExporting(), false);
    QCOMPARE(controller.exportDialogOpen(), false);

    controller.openExportDialog();
    QCOMPARE(controller.exportDialogOpen(), true);
    controller.closeExportDialog();
    QCOMPARE(controller.exportDialogOpen(), false);

    // Populate model with test items
    CatalogItem it1;
    it1.id = QStringLiteral("item-01");
    it1.filePath = m_sourceImagePath;
    it1.fileName = QStringLiteral("ctrl_test_1.jpg");

    CatalogItem it2;
    it2.id = QStringLiteral("item-02");
    it2.filePath = m_sourceImagePath;
    it2.fileName = QStringLiteral("ctrl_test_2.jpg");

    model.addItems({it1, it2});
    selManager.selectAll();
    QCOMPARE(selManager.selectedCount(), 2);

    const QString outDir = m_tempDir->filePath(QStringLiteral("ctrl_export"));
    QVariantMap opts;
    opts.insert(QStringLiteral("outputDirectory"), outDir);
    opts.insert(QStringLiteral("format"), QStringLiteral("jpg"));
    opts.insert(QStringLiteral("maxDimension"), 400);

    QSignalSpy spyProgress(&controller, &SessionController::exportProgressChanged);
    QSignalSpy spyCompleted(&controller, &SessionController::exportCompleted);

    controller.startExportSelected(opts);
    QVERIFY(controller.isExporting() || spyCompleted.count() > 0);

    QVERIFY(spyCompleted.wait(5000));
    QCOMPARE(spyCompleted.count(), 1);
    QCOMPARE(controller.isExporting(), false);
}

QTEST_MAIN(TestImageExporter)
#include "test_image_exporter.moc"
