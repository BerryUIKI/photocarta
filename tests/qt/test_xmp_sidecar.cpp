// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogitem.h"
#include "catalog/catalogmodel.h"
#include "catalog/ingestion/directoryscanner.h"
#include "catalog/metadata/xmpsidecar.h"
#include "catalog/selectionmanager.h"
#include "catalog/sessioncontroller.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

class TestXmpSidecar : public QObject
{
    Q_OBJECT

private slots:
    void testColorLabelMapping();
    void testSidecarPathResolution();
    void testGenerateAndReadXmpData();
    void testWriteAndReadSidecarFile();
    void testMergeExistingSidecar();
    void testExternalSidecarInteroperability();
    void testDirectoryScannerXmpIntegration();
    void testCatalogModelAndControllerXmpSync();
};

void TestXmpSidecar::testColorLabelMapping()
{
    QCOMPARE(XmpSidecar::colorLabelToString(1), QStringLiteral("Red"));
    QCOMPARE(XmpSidecar::colorLabelToString(2), QStringLiteral("Yellow"));
    QCOMPARE(XmpSidecar::colorLabelToString(3), QStringLiteral("Green"));
    QCOMPARE(XmpSidecar::colorLabelToString(4), QStringLiteral("Blue"));
    QCOMPARE(XmpSidecar::colorLabelToString(5), QStringLiteral("Purple"));
    QCOMPARE(XmpSidecar::colorLabelToString(0), QString());

    QCOMPARE(XmpSidecar::stringToColorLabel(QStringLiteral("Red")), 1);
    QCOMPARE(XmpSidecar::stringToColorLabel(QStringLiteral("red")), 1);
    QCOMPARE(XmpSidecar::stringToColorLabel(QStringLiteral("1")), 1);

    QCOMPARE(XmpSidecar::stringToColorLabel(QStringLiteral("Yellow")), 2);
    QCOMPARE(XmpSidecar::stringToColorLabel(QStringLiteral("Green")), 3);
    QCOMPARE(XmpSidecar::stringToColorLabel(QStringLiteral("Blue")), 4);
    QCOMPARE(XmpSidecar::stringToColorLabel(QStringLiteral("Purple")), 5);
    QCOMPARE(XmpSidecar::stringToColorLabel(QStringLiteral("Violet")), 5);
    QCOMPARE(XmpSidecar::stringToColorLabel(QStringLiteral("None")), 0);
    QCOMPARE(XmpSidecar::stringToColorLabel(QStringLiteral("UnknownColor")), 0);
}

void TestXmpSidecar::testSidecarPathResolution()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString imgPath = tempDir.filePath(QStringLiteral("photo.jpg"));
    QFile f(imgPath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("fake photo data");
    f.close();

    // No sidecar initially
    QVERIFY(!XmpSidecar::hasSidecar(imgPath));
    QVERIFY(XmpSidecar::sidecarPathForImage(imgPath, true).isEmpty());
    QCOMPARE(XmpSidecar::sidecarPathForImage(imgPath, false), imgPath + QStringLiteral(".xmp"));

    // Create standard <file>.<ext>.xmp
    const QString standardSidecar = imgPath + QStringLiteral(".xmp");
    QFile sf(standardSidecar);
    QVERIFY(sf.open(QIODevice::WriteOnly));
    sf.write("<xmp></xmp>");
    sf.close();

    QVERIFY(XmpSidecar::hasSidecar(imgPath));
    QCOMPARE(XmpSidecar::sidecarPathForImage(imgPath, true), standardSidecar);

    // Create base-name sidecar <file>.xmp (Lightroom style)
    const QString imgPath2 = tempDir.filePath(QStringLiteral("lightroom_sample.png"));
    QFile f2(imgPath2);
    QVERIFY(f2.open(QIODevice::WriteOnly));
    f2.write("sample");
    f2.close();

    const QString lrSidecar = tempDir.filePath(QStringLiteral("lightroom_sample.xmp"));
    QFile sf2(lrSidecar);
    QVERIFY(sf2.open(QIODevice::WriteOnly));
    sf2.write("<xmp></xmp>");
    sf2.close();

    QVERIFY(XmpSidecar::hasSidecar(imgPath2));
    QCOMPARE(XmpSidecar::sidecarPathForImage(imgPath2, true), lrSidecar);
}

void TestXmpSidecar::testGenerateAndReadXmpData()
{
    XmpMetadata input;
    input.title = QStringLiteral("Sunset Over Glacial Alpine Peaks");
    input.caption = QStringLiteral("Captured during sunset with neutral density filter.");
    input.rating = 5;
    input.colorLabel = 3; // Green
    input.isFlagged = true;
    input.tags = {QStringLiteral("Mountains"), QStringLiteral("Landscape"), QStringLiteral("Sunset")};
    input.dateTaken = QDateTime::fromString(QStringLiteral("2026-10-18T18:45:00Z"), Qt::ISODate);

    const QByteArray xml = XmpSidecar::generateXmpData(input);
    QVERIFY(!xml.isEmpty());
    QVERIFY(xml.contains("x:xmpmeta"));
    QVERIFY(xml.contains("rdf:RDF"));
    QVERIFY(xml.contains("xmp:Rating=\"5\""));
    QVERIFY(xml.contains("xmp:Label=\"Green\""));
    QVERIFY(xml.contains("photoshop:Urgency=\"1\""));
    QVERIFY(xml.contains("Sunset Over Glacial Alpine Peaks"));
    QVERIFY(xml.contains("Captured during sunset"));
    QVERIFY(xml.contains("<rdf:li>Mountains</rdf:li>"));

    XmpMetadata parsed;
    QVERIFY(XmpSidecar::readFromData(xml, parsed));
    QCOMPARE(parsed.title, input.title);
    QCOMPARE(parsed.caption, input.caption);
    QCOMPARE(parsed.rating, 5);
    QCOMPARE(parsed.colorLabel, 3);
    QCOMPARE(parsed.isFlagged, true);
    QCOMPARE(parsed.tags.size(), 3);
    QVERIFY(parsed.tags.contains(QStringLiteral("Mountains")));
    QVERIFY(parsed.tags.contains(QStringLiteral("Landscape")));
    QVERIFY(parsed.tags.contains(QStringLiteral("Sunset")));
    QCOMPARE(parsed.dateTaken.toUTC(), input.dateTaken.toUTC());
}

void TestXmpSidecar::testWriteAndReadSidecarFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sidecarPath = tempDir.filePath(QStringLiteral("test.jpg.xmp"));

    XmpMetadata meta;
    meta.title = QStringLiteral("Coastal Sea Cliff Arch");
    meta.caption = QStringLiteral("Dramatic high tide waves crashing against sea arches.");
    meta.rating = 4;
    meta.colorLabel = 4; // Blue
    meta.isFlagged = false;
    meta.tags = {QStringLiteral("Ocean"), QStringLiteral("Coast")};

    QVERIFY(XmpSidecar::writeToFile(sidecarPath, meta));
    QVERIFY(QFile::exists(sidecarPath));

    XmpMetadata reloaded;
    QVERIFY(XmpSidecar::readFromFile(sidecarPath, reloaded));
    QCOMPARE(reloaded.title, meta.title);
    QCOMPARE(reloaded.caption, meta.caption);
    QCOMPARE(reloaded.rating, 4);
    QCOMPARE(reloaded.colorLabel, 4);
    QCOMPARE(reloaded.isFlagged, false);
    QCOMPARE(reloaded.tags.size(), 2);
}

void TestXmpSidecar::testMergeExistingSidecar()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sidecarPath = tempDir.filePath(QStringLiteral("merge_test.jpg.xmp"));

    // Step 1: Write initial metadata (title, rating)
    XmpMetadata initial;
    initial.title = QStringLiteral("Original Preserved Title");
    initial.rating = 3;
    initial.tags = {QStringLiteral("Architecture")};
    QVERIFY(XmpSidecar::writeToFile(sidecarPath, initial));

    // Step 2: Write update modifying color label and caption (with title empty)
    XmpMetadata update;
    update.caption = QStringLiteral("Newly added description notes.");
    update.colorLabel = 1; // Red
    QVERIFY(XmpSidecar::writeToFile(sidecarPath, update));

    // Step 3: Verify that original title and tags were merged and preserved
    XmpMetadata merged;
    QVERIFY(XmpSidecar::readFromFile(sidecarPath, merged));
    QCOMPARE(merged.title, QStringLiteral("Original Preserved Title"));
    QCOMPARE(merged.rating, 3);
    QCOMPARE(merged.colorLabel, 1);
    QCOMPARE(merged.caption, QStringLiteral("Newly added description notes."));
    QCOMPARE(merged.tags, QStringList{QStringLiteral("Architecture")});
}

void TestXmpSidecar::testExternalSidecarInteroperability()
{
    // Adobe Lightroom style XMP sidecar
    const QByteArray lrXml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">\n"
        " <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
        "  <rdf:Description rdf:about=\"\"\n"
        "    xmlns:xmp=\"http://ns.adobe.com/xap/1.0/\"\n"
        "    xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n"
        "    xmlns:photoshop=\"http://ns.adobe.com/photoshop/1.0/\"\n"
        "    xmp:Rating=\"4\"\n"
        "    xmp:Label=\"Yellow\"\n"
        "    xmp:Pick=\"1\">\n"
        "   <dc:title><rdf:Alt><rdf:li xml:lang=\"x-default\">Lightroom Title</rdf:li></rdf:Alt></dc:title>\n"
        "   <dc:description><rdf:Alt><rdf:li xml:lang=\"x-default\">Lightroom Caption</rdf:li></rdf:Alt></dc:description>\n"
        "   <dc:subject><rdf:Bag><rdf:li>Portrait</rdf:li><rdf:li>Studio</rdf:li></rdf:Bag></dc:subject>\n"
        "  </rdf:Description>\n"
        " </rdf:RDF>\n"
        "</x:xmpmeta>\n";

    XmpMetadata lrMeta;
    QVERIFY(XmpSidecar::readFromData(lrXml, lrMeta));
    QCOMPARE(lrMeta.title, QStringLiteral("Lightroom Title"));
    QCOMPARE(lrMeta.caption, QStringLiteral("Lightroom Caption"));
    QCOMPARE(lrMeta.rating, 4);
    QCOMPARE(lrMeta.colorLabel, 2); // Yellow
    QCOMPARE(lrMeta.isFlagged, true);
    QCOMPARE(lrMeta.tags, QStringList({QStringLiteral("Portrait"), QStringLiteral("Studio")}));

    // digiKam style XMP sidecar with digiKam:TagsList and photoshop:Urgency
    const QByteArray dkXml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">\n"
        " <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
        "  <rdf:Description rdf:about=\"\"\n"
        "    xmlns:xmp=\"http://ns.adobe.com/xap/1.0/\"\n"
        "    xmlns:photoshop=\"http://ns.adobe.com/photoshop/1.0/\"\n"
        "    xmlns:digiKam=\"http://www.digikam.org/ns/1.0/\"\n"
        "    xmp:Rating=\"5\"\n"
        "    xmp:Label=\"Purple\"\n"
        "    photoshop:Urgency=\"1\">\n"
        "   <digiKam:TagsList><rdf:Seq><rdf:li>Macro</rdf:li><rdf:li>Flora</rdf:li></rdf:Seq></digiKam:TagsList>\n"
        "  </rdf:Description>\n"
        " </rdf:RDF>\n"
        "</x:xmpmeta>\n";

    XmpMetadata dkMeta;
    QVERIFY(XmpSidecar::readFromData(dkXml, dkMeta));
    QCOMPARE(dkMeta.rating, 5);
    QCOMPARE(dkMeta.colorLabel, 5); // Purple
    QCOMPARE(dkMeta.isFlagged, true);
    QCOMPARE(dkMeta.tags, QStringList({QStringLiteral("Macro"), QStringLiteral("Flora")}));
}

void TestXmpSidecar::testDirectoryScannerXmpIntegration()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Create a real JPEG image file
    const QString imgPath = tempDir.filePath(QStringLiteral("nature_macro.jpg"));
    QImage dummy(64, 64, QImage::Format_RGB32);
    dummy.fill(Qt::darkGreen);
    QVERIFY(dummy.save(imgPath, "JPG"));

    // Write companion .xmp sidecar
    XmpMetadata sidecar;
    sidecar.title = QStringLiteral("Dewdrops on Fern Leaf");
    sidecar.caption = QStringLiteral("Early morning botanical macro capture.");
    sidecar.rating = 5;
    sidecar.colorLabel = 3; // Green
    sidecar.isFlagged = true;
    sidecar.tags = {QStringLiteral("Nature"), QStringLiteral("Macro"), QStringLiteral("Botanical")};

    const QString sidecarPath = XmpSidecar::sidecarPathForImage(imgPath, false);
    QVERIFY(XmpSidecar::writeToFile(sidecarPath, sidecar));

    // Inspect file via DirectoryScanner
    const CatalogItem item = DirectoryScanner::inspectFile(imgPath);
    QCOMPARE(item.title, QStringLiteral("Dewdrops on Fern Leaf"));
    QCOMPARE(item.caption, QStringLiteral("Early morning botanical macro capture."));
    QCOMPARE(item.rating, 5);
    QCOMPARE(item.colorLabel, 3);
    QCOMPARE(item.isFlagged, true);
    QCOMPARE(item.tags.size(), 3);
    QVERIFY(item.tags.contains(QStringLiteral("Botanical")));
}

void TestXmpSidecar::testCatalogModelAndControllerXmpSync()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Create 2 test image files
    const QString imgPath1 = tempDir.filePath(QStringLiteral("photo1.jpg"));
    const QString imgPath2 = tempDir.filePath(QStringLiteral("photo2.jpg"));
    QImage img(32, 32, QImage::Format_RGB32);
    img.fill(Qt::blue);
    QVERIFY(img.save(imgPath1, "JPG"));
    QVERIFY(img.save(imgPath2, "JPG"));

    CatalogModel model;
    CatalogFilterModel filterModel;
    filterModel.setSourceModel(&model);
    SelectionManager selManager(&filterModel);
    SessionController controller(&model, &filterModel, &selManager);

    // Add images to model
    model.addFilePaths({imgPath1, imgPath2});
    QCOMPARE(model.rowCount(), 2);

    // Enable XMP sync in controller
    controller.setXmpSyncEnabled(true);
    QVERIFY(controller.xmpSyncEnabled());
    QVERIFY(model.xmpSyncEnabled());

    // Update item 0 title and color label
    model.updateItemTitle(0, QStringLiteral("Synchronized Title"));
    model.updateItemColorLabel(0, 1); // Red
    model.updateItemRating(0, 4);

    // Check that sidecar was written automatically
    const QString sidecar1 = XmpSidecar::sidecarPathForImage(imgPath1, true);
    QVERIFY(!sidecar1.isEmpty());
    QVERIFY(QFile::exists(sidecar1));

    XmpMetadata reloaded1;
    QVERIFY(XmpSidecar::readFromFile(sidecar1, reloaded1));
    QCOMPARE(reloaded1.title, QStringLiteral("Synchronized Title"));
    QCOMPARE(reloaded1.colorLabel, 1);
    QCOMPARE(reloaded1.rating, 4);

    // Test manual export for selected items
    selManager.select(1);
    controller.setTitleForSelected(QStringLiteral("Photo 2 Title"));
    controller.setColorLabelForSelected(4); // Blue
    controller.exportXmpSidecarsForSelected();

    const QString sidecar2 = XmpSidecar::sidecarPathForImage(imgPath2, true);
    QVERIFY(!sidecar2.isEmpty());
    XmpMetadata reloaded2;
    QVERIFY(XmpSidecar::readFromFile(sidecar2, reloaded2));
    QCOMPARE(reloaded2.title, QStringLiteral("Photo 2 Title"));
    QCOMPARE(reloaded2.colorLabel, 4);

    // Test external sidecar modification and import
    reloaded2.caption = QStringLiteral("Externally modified notes in XMP sidecar");
    reloaded2.rating = 5;
    QVERIFY(XmpSidecar::writeToFile(sidecar2, reloaded2));

    controller.importXmpSidecarsForSelected();
    QVERIFY(model.itemAt(1) != nullptr);
    QCOMPARE(model.itemAt(1)->caption, QStringLiteral("Externally modified notes in XMP sidecar"));
    QCOMPARE(model.itemAt(1)->rating, 5);
}

QTEST_MAIN(TestXmpSidecar)
#include "test_xmp_sidecar.moc"
