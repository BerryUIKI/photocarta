// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/metadata/exifparser.h"

#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QTemporaryFile>
#include <QTest>

using namespace PhotoCarta;

class TestExifParser : public QObject
{
    Q_OBJECT

private slots:
    void testEmptyAndCorruptData();
    void testSyntheticTiffExifParsing();
    void testJpegApp1ExifParsing();
    void testFileParsing();
};

void TestExifParser::testEmptyAndCorruptData()
{
    const ExifData empty = ExifParser::parseData(QByteArray());
    QVERIFY(!empty.isValid);

    const ExifData tooShort = ExifParser::parseData(QByteArray("short"));
    QVERIFY(!tooShort.isValid);

    const ExifData invalidJpeg = ExifParser::parseData(QByteArray("\xFF\xD8\xFF\xE0\x00\x10JFIF\0\0\0\0\0\0", 16));
    QVERIFY(!invalidJpeg.isValid);
}

static QByteArray createSyntheticTiffExif()
{
    // Build a little-endian TIFF buffer with IFD0 and Exif Sub-IFD
    QByteArray buf;
    QDataStream out(&buf, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);

    // 1. TIFF Header (8 bytes)
    out.writeRawData("II", 2);  // Little-endian
    out << static_cast<quint16>(42); // Magic
    out << static_cast<quint32>(8);  // IFD0 offset

    // Layout calculation:
    // Offset 8: IFD0 with 3 tags (Make, Model, ExifIFDPointer)
    // Size of IFD0: 2 + 3 * 12 + 4 = 42 bytes. Ends at offset 50.
    // Offset 50: Exif Sub-IFD with 6 tags (ExposureTime, FNumber, ISO, DateTimeOriginal, FocalLength, LensModel)
    // Size of Sub-IFD: 2 + 6 * 12 + 4 = 78 bytes. Ends at offset 128.
    // Offset 128+: String and rational payloads

    const quint32 exifIfdOffset = 50;

    // IFD0: 3 entries
    out << static_cast<quint16>(3);

    // Entry 1: Make (Tag 0x010F, ASCII=2, count=6, offset=128)
    const quint32 makeOffset = 128;
    out << static_cast<quint16>(0x010F) << static_cast<quint16>(2) << static_cast<quint32>(6) << makeOffset;

    // Entry 2: Model (Tag 0x0110, ASCII=2, count=9, offset=134)
    const quint32 modelOffset = 134;
    out << static_cast<quint16>(0x0110) << static_cast<quint16>(2) << static_cast<quint32>(9) << modelOffset;

    // Entry 3: ExifIFDPointer (Tag 0x8769, LONG=4, count=1, value=50)
    out << static_cast<quint16>(0x8769) << static_cast<quint16>(4) << static_cast<quint32>(1) << exifIfdOffset;

    // Next IFD offset: 0
    out << static_cast<quint32>(0);

    // Exif Sub-IFD at offset 50: 6 entries
    out << static_cast<quint16>(6);

    // Entry 1: ExposureTime (Tag 0x829A, RATIONAL=5, count=1, offset=144)
    const quint32 expOffset = 144;
    out << static_cast<quint16>(0x829A) << static_cast<quint16>(5) << static_cast<quint32>(1) << expOffset;

    // Entry 2: FNumber (Tag 0x829D, RATIONAL=5, count=1, offset=152)
    const quint32 fnOffset = 152;
    out << static_cast<quint16>(0x829D) << static_cast<quint16>(5) << static_cast<quint32>(1) << fnOffset;

    // Entry 3: ISO (Tag 0x8827, SHORT=3, count=1, inline value=400)
    out << static_cast<quint16>(0x8827) << static_cast<quint16>(3) << static_cast<quint32>(1) << static_cast<quint32>(400);

    // Entry 4: DateTimeOriginal (Tag 0x9003, ASCII=2, count=20, offset=160)
    const quint32 dateOffset = 160;
    out << static_cast<quint16>(0x9003) << static_cast<quint16>(2) << static_cast<quint32>(20) << dateOffset;

    // Entry 5: FocalLength (Tag 0x920A, RATIONAL=5, count=1, offset=180)
    const quint32 flOffset = 180;
    out << static_cast<quint16>(0x920A) << static_cast<quint16>(5) << static_cast<quint32>(1) << flOffset;

    // Entry 6: LensModel (Tag 0xA434, ASCII=2, count=22, offset=188)
    const quint32 lensOffset = 188;
    out << static_cast<quint16>(0xA434) << static_cast<quint16>(2) << static_cast<quint32>(22) << lensOffset;

    // Next IFD: 0
    out << static_cast<quint32>(0);

    // Pad to offset 128
    while (buf.size() < 128)
    {
        buf.append('\0');
    }

    // Offset 128: Make string "SONY\0" (count=6)
    buf.append("SONY\0\0", 6);

    // Offset 134: Model string "ILCE-7M4\0" (count=9)
    buf.append("ILCE-7M4\0", 9);

    // Pad to offset 144
    while (buf.size() < 144)
    {
        buf.append('\0');
    }

    // Offset 144: ExposureTime rational: 1 / 250
    QDataStream ratOut(&buf, QIODevice::Append);
    ratOut.setByteOrder(QDataStream::LittleEndian);
    ratOut << static_cast<quint32>(1) << static_cast<quint32>(250);

    // Offset 152: FNumber rational: 28 / 10 = 2.8
    ratOut << static_cast<quint32>(28) << static_cast<quint32>(10);

    // Pad to offset 160
    while (buf.size() < 160)
    {
        buf.append('\0');
    }

    // Offset 160: DateTimeOriginal string "2026:08:15 14:30:00\0" (count=20)
    buf.append("2026:08:15 14:30:00\0", 20);

    // Offset 180: FocalLength rational: 50 / 1 = 50mm
    QDataStream flOut(&buf, QIODevice::Append);
    flOut.setByteOrder(QDataStream::LittleEndian);
    flOut << static_cast<quint32>(50) << static_cast<quint32>(1);

    // Pad to offset 188
    while (buf.size() < 188)
    {
        buf.append('\0');
    }

    // Offset 188: LensModel "FE 50mm F1.8 Macro OSS\0" (count=22)
    buf.append("FE 50mm F1.8 Macro OSS\0", 22);

    return buf;
}

void TestExifParser::testSyntheticTiffExifParsing()
{
    const QByteArray tiff = createSyntheticTiffExif();
    const ExifData exif = ExifParser::parseData(tiff);

    QVERIFY(exif.isValid);
    QCOMPARE(exif.cameraModel, QStringLiteral("Sony ILCE-7M4"));
    QCOMPARE(exif.lensModel, QStringLiteral("FE 50mm F1.8 Macro OSS"));
    QCOMPARE(exif.aperture, QStringLiteral("f/2.8"));
    QCOMPARE(exif.shutterSpeed, QStringLiteral("1/250s"));
    QCOMPARE(exif.iso, 400);
    QCOMPARE(exif.focalLength, QStringLiteral("50 mm"));
    QCOMPARE(exif.dateTaken, QDateTime::fromString(QStringLiteral("2026:08:15 14:30:00"), QStringLiteral("yyyy:MM:dd HH:mm:ss")));
}

void TestExifParser::testJpegApp1ExifParsing()
{
    const QByteArray tiff = createSyntheticTiffExif();

    // Wrap in standard JPEG SOI + APP1 marker
    QByteArray jpeg;
    jpeg.append("\xFF\xD8", 2); // SOI
    jpeg.append("\xFF\xE1", 2); // APP1 marker

    const quint16 app1Length = static_cast<quint16>(tiff.size() + 8);
    jpeg.append(static_cast<char>((app1Length >> 8) & 0xFF));
    jpeg.append(static_cast<char>(app1Length & 0xFF));
    jpeg.append("Exif\0\0", 6);
    jpeg.append(tiff);
    jpeg.append("\xFF\xD9", 2); // EOI

    const ExifData exif = ExifParser::parseData(jpeg);

    QVERIFY(exif.isValid);
    QCOMPARE(exif.cameraModel, QStringLiteral("Sony ILCE-7M4"));
    QCOMPARE(exif.lensModel, QStringLiteral("FE 50mm F1.8 Macro OSS"));
    QCOMPARE(exif.aperture, QStringLiteral("f/2.8"));
    QCOMPARE(exif.shutterSpeed, QStringLiteral("1/250s"));
    QCOMPARE(exif.iso, 400);
    QCOMPARE(exif.focalLength, QStringLiteral("50 mm"));
}

void TestExifParser::testFileParsing()
{
    const QByteArray tiff = createSyntheticTiffExif();
    QByteArray jpeg;
    jpeg.append("\xFF\xD8\xFF\xE1", 4);
    const quint16 len = static_cast<quint16>(tiff.size() + 8);
    jpeg.append(static_cast<char>((len >> 8) & 0xFF));
    jpeg.append(static_cast<char>(len & 0xFF));
    jpeg.append("Exif\0\0", 6);
    jpeg.append(tiff);
    jpeg.append("\xFF\xD9", 2);

    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QStringLiteral("XXXXXX.jpg"));
    QVERIFY(tempFile.open());
    tempFile.write(jpeg);
    tempFile.flush();

    const ExifData exif = ExifParser::parseFile(tempFile.fileName());
    QVERIFY(exif.isValid);
    QCOMPARE(exif.cameraModel, QStringLiteral("Sony ILCE-7M4"));
    QCOMPARE(exif.iso, 400);
}

QTEST_MAIN(TestExifParser)
#include "test_exif_parser.moc"
