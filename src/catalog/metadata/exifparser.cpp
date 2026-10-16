// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "exifparser.h"

#include <cmath>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>

namespace PhotoCarta
{

namespace
{
constexpr quint32 MAX_HEADER_READ_BYTES = 131072; // 128 KB is plenty for standard metadata headers

// Standard TIFF/EXIF tag constants
constexpr quint16 TAG_MAKE = 0x010F;
constexpr quint16 TAG_MODEL = 0x0110;
constexpr quint16 TAG_ORIENTATION = 0x0112;
constexpr quint16 TAG_DATETIME = 0x0132;
constexpr quint16 TAG_EXIF_IFD = 0x8769;
constexpr quint16 TAG_GPS_IFD = 0x8825;

// Exif Sub-IFD tags
constexpr quint16 TAG_EXPOSURE_TIME = 0x829A;
constexpr quint16 TAG_FNUMBER = 0x829D;
constexpr quint16 TAG_ISO = 0x8827;
constexpr quint16 TAG_ISO_REI = 0x8833;
constexpr quint16 TAG_DATETIME_ORIGINAL = 0x9003;
constexpr quint16 TAG_DATETIME_DIGITIZED = 0x9004;
constexpr quint16 TAG_FOCAL_LENGTH = 0x920A;
constexpr quint16 TAG_LENS_MAKE = 0xA433;
constexpr quint16 TAG_LENS_MODEL = 0xA434;
constexpr quint16 TAG_PIXEL_X_DIM = 0xA002;
constexpr quint16 TAG_PIXEL_Y_DIM = 0xA003;

// GPS Sub-IFD tags
constexpr quint16 TAG_GPS_LAT_REF = 0x0001;
constexpr quint16 TAG_GPS_LAT = 0x0002;
constexpr quint16 TAG_GPS_LON_REF = 0x0003;
constexpr quint16 TAG_GPS_LON = 0x0004;
constexpr quint16 TAG_GPS_ALT = 0x0006;
} // namespace

ExifData ExifParser::parseFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }

    const QByteArray header = file.read(MAX_HEADER_READ_BYTES);
    return parseData(header);
}

ExifData ExifParser::parseData(const QByteArray &data)
{
    ExifData out;
    if (data.size() < 12)
    {
        return out;
    }

    QByteArray tiffData;

    // Check for JPEG Start-Of-Image marker (0xFF, 0xD8)
    const auto *p = reinterpret_cast<const uchar *>(data.constData());
    if (p[0] == 0xFF && p[1] == 0xD8)
    {
        if (!extractJpegExif(data, tiffData))
        {
            return out;
        }
    }
    // Check for standard TIFF header (TIFF, DNG, CR2, NEF, ARW, ORF, RW2)
    else if ((data.startsWith("II*\0") || data.startsWith("MM\0*")))
    {
        tiffData = data;
    }
    else
    {
        return out;
    }

    if (parseTiffBlock(tiffData, out))
    {
        out.isValid = true;
    }

    return out;
}

bool ExifParser::extractJpegExif(const QByteArray &jpegData, QByteArray &tiffData)
{
    const int size = jpegData.size();
    const auto *p = reinterpret_cast<const uchar *>(jpegData.constData());
    int i = 2; // Skip SOI (0xFF, 0xD8)

    while (i + 4 <= size)
    {
        if (p[i] != 0xFF)
        {
            ++i;
            continue;
        }

        // Skip extra 0xFF padding bytes
        while (i < size && p[i] == 0xFF)
        {
            ++i;
        }

        if (i >= size)
        {
            break;
        }

        const uchar marker = p[i++];
        if (marker == 0xD8) // SOI
        {
            continue;
        }
        if (marker == 0xD9 || marker == 0xDA) // EOI or Start of Scan (pixel stream begins)
        {
            break;
        }

        if (i + 2 > size)
        {
            break;
        }

        const quint16 length = (static_cast<quint16>(p[i]) << 8) | static_cast<quint16>(p[i + 1]);
        if (length < 2)
        {
            break;
        }

        // APP1 Marker: 0xE1
        if (marker == 0xE1 && (i + length) <= size)
        {
            // Check for "Exif\0\0" header (6 bytes)
            if (length >= 8 && std::memcmp(p + i + 2, "Exif\0\0", 6) == 0)
            {
                tiffData = jpegData.mid(i + 8, length - 8);
                return true;
            }
        }

        i += length;
    }

    return false;
}

quint16 ExifParser::readU16(const uchar *p, bool isLittle)
{
    if (isLittle)
    {
        return static_cast<quint16>(p[0]) | (static_cast<quint16>(p[1]) << 8);
    }
    return (static_cast<quint16>(p[0]) << 8) | static_cast<quint16>(p[1]);
}

quint32 ExifParser::readU32(const uchar *p, bool isLittle)
{
    if (isLittle)
    {
        return static_cast<quint32>(p[0]) |
               (static_cast<quint32>(p[1]) << 8) |
               (static_cast<quint32>(p[2]) << 16) |
               (static_cast<quint32>(p[3]) << 24);
    }
    return (static_cast<quint32>(p[0]) << 24) |
           (static_cast<quint32>(p[1]) << 16) |
           (static_cast<quint32>(p[2]) << 8) |
           static_cast<quint32>(p[3]);
}

bool ExifParser::readRational(const uchar *p, bool isLittle, double &val, quint32 &num, quint32 &den)
{
    num = readU32(p, isLittle);
    den = readU32(p + 4, isLittle);
    if (den == 0)
    {
        val = 0.0;
        return false;
    }
    val = static_cast<double>(num) / static_cast<double>(den);
    return true;
}

QString ExifParser::readAscii(const QByteArray &data, quint32 offset, quint32 count)
{
    if (offset >= static_cast<quint32>(data.size()) || count == 0)
    {
        return {};
    }
    const quint32 safeLen = qMin(count, static_cast<quint32>(data.size()) - offset);
    QByteArray bytes = data.mid(static_cast<int>(offset), static_cast<int>(safeLen));
    // Remove null terminators
    const int nulPos = bytes.indexOf('\0');
    if (nulPos >= 0)
    {
        bytes.truncate(nulPos);
    }
    return QString::fromUtf8(bytes).trimmed();
}

bool ExifParser::parseTiffBlock(const QByteArray &tiffData, ExifData &out)
{
    if (tiffData.size() < 8)
    {
        return false;
    }

    const auto *raw = reinterpret_cast<const uchar *>(tiffData.constData());
    const bool isLittle = (raw[0] == 'I' && raw[1] == 'I');
    const bool isBig = (raw[0] == 'M' && raw[1] == 'M');
    if (!isLittle && !isBig)
    {
        return false;
    }

    const quint16 magic = readU16(raw + 2, isLittle);
    if (magic != 42 && magic != 0x4f52) // 42 is standard TIFF; 0x4f52 is Olympus ORF
    {
        return false;
    }

    const quint32 ifd0Offset = readU32(raw + 4, isLittle);
    if (ifd0Offset == 0 || ifd0Offset + 2 > static_cast<quint32>(tiffData.size()))
    {
        return false;
    }

    quint32 exifIfdOffset = 0;
    quint32 gpsIfdOffset = 0;
    QString dateStr;
    QString origDateStr;

    auto parseIfd = [&](quint32 offset, auto &&handler)
    {
        if (offset + 2 > static_cast<quint32>(tiffData.size()))
        {
            return;
        }
        const quint16 numEntries = readU16(raw + offset, isLittle);
        quint32 cur = offset + 2;

        for (quint16 i = 0; i < numEntries; ++i)
        {
            if (cur + 12 > static_cast<quint32>(tiffData.size()))
            {
                break;
            }
            const quint16 tag = readU16(raw + cur, isLittle);
            const quint16 type = readU16(raw + cur + 2, isLittle);
            const quint32 count = readU32(raw + cur + 4, isLittle);

            // Determine whether the value is stored inline or at an offset
            quint32 valueOffset = cur + 8;
            quint32 dataSize = 0;
            switch (type)
            {
            case 1: // BYTE
            case 2: // ASCII
            case 7: // UNDEFINED
                dataSize = count;
                break;
            case 3: // SHORT
                dataSize = count * 2;
                break;
            case 4: // LONG
            case 9: // SLONG
                dataSize = count * 4;
                break;
            case 5:  // RATIONAL
            case 10: // SRATIONAL
                dataSize = count * 8;
                break;
            default:
                break;
            }

            if (dataSize > 4)
            {
                valueOffset = readU32(raw + cur + 8, isLittle);
            }

            handler(tag, type, count, valueOffset);
            cur += 12;
        }
    };

    // Parse IFD0
    parseIfd(ifd0Offset, [&](quint16 tag, quint16 type, quint32 count, quint32 offset) {
        Q_UNUSED(type);
        if (tag == TAG_MAKE)
        {
            out.make = readAscii(tiffData, offset, count);
        }
        else if (tag == TAG_MODEL)
        {
            out.model = readAscii(tiffData, offset, count);
        }
        else if (tag == TAG_ORIENTATION && offset + 2 <= static_cast<quint32>(tiffData.size()))
        {
            out.orientation = readU16(raw + offset, isLittle);
        }
        else if (tag == TAG_DATETIME)
        {
            dateStr = readAscii(tiffData, offset, count);
        }
        else if (tag == TAG_EXIF_IFD && offset + 4 <= static_cast<quint32>(tiffData.size()))
        {
            exifIfdOffset = (count == 1) ? readU32(raw + offset, isLittle) : offset;
        }
        else if (tag == TAG_GPS_IFD && offset + 4 <= static_cast<quint32>(tiffData.size()))
        {
            gpsIfdOffset = (count == 1) ? readU32(raw + offset, isLittle) : offset;
        }
    });

    // Parse EXIF Sub-IFD
    if (exifIfdOffset > 0)
    {
        parseIfd(exifIfdOffset, [&](quint16 tag, quint16 type, quint32 count, quint32 offset) {
            Q_UNUSED(type);
            if (tag == TAG_DATETIME_ORIGINAL)
            {
                origDateStr = readAscii(tiffData, offset, count);
            }
            else if (tag == TAG_EXPOSURE_TIME && offset + 8 <= static_cast<quint32>(tiffData.size()))
            {
                double expSec = 0.0;
                quint32 n = 0;
                quint32 d = 0;
                if (readRational(raw + offset, isLittle, expSec, n, d))
                {
                    out.shutterSpeed = formatShutterSpeed(expSec);
                }
            }
            else if (tag == TAG_FNUMBER && offset + 8 <= static_cast<quint32>(tiffData.size()))
            {
                double fVal = 0.0;
                quint32 n = 0;
                quint32 d = 0;
                if (readRational(raw + offset, isLittle, fVal, n, d))
                {
                    out.aperture = formatAperture(fVal);
                }
            }
            else if ((tag == TAG_ISO || tag == TAG_ISO_REI) && offset + 2 <= static_cast<quint32>(tiffData.size()))
            {
                if (out.iso == 0)
                {
                    out.iso = readU16(raw + offset, isLittle);
                }
            }
            else if (tag == TAG_FOCAL_LENGTH && offset + 8 <= static_cast<quint32>(tiffData.size()))
            {
                double fl = 0.0;
                quint32 n = 0;
                quint32 d = 0;
                if (readRational(raw + offset, isLittle, fl, n, d))
                {
                    out.focalLength = formatFocalLength(fl);
                }
            }
            else if (tag == TAG_LENS_MAKE)
            {
                out.lensMake = readAscii(tiffData, offset, count);
            }
            else if (tag == TAG_LENS_MODEL)
            {
                out.lensModel = readAscii(tiffData, offset, count);
            }
            else if (tag == TAG_PIXEL_X_DIM && offset + 4 <= static_cast<quint32>(tiffData.size()))
            {
                out.dimensions.setWidth(static_cast<int>(readU32(raw + offset, isLittle)));
            }
            else if (tag == TAG_PIXEL_Y_DIM && offset + 4 <= static_cast<quint32>(tiffData.size()))
            {
                out.dimensions.setHeight(static_cast<int>(readU32(raw + offset, isLittle)));
            }
        });
    }

    // Parse GPS Sub-IFD
    if (gpsIfdOffset > 0)
    {
        QString latRef;
        QString lonRef;
        double latVals[3] = {0, 0, 0};
        double lonVals[3] = {0, 0, 0};
        bool hasLat = false;
        bool hasLon = false;

        parseIfd(gpsIfdOffset, [&](quint16 tag, quint16 type, quint32 count, quint32 offset) {
            Q_UNUSED(type);
            if (tag == TAG_GPS_LAT_REF)
            {
                latRef = readAscii(tiffData, offset, count);
            }
            else if (tag == TAG_GPS_LON_REF)
            {
                lonRef = readAscii(tiffData, offset, count);
            }
            else if (tag == TAG_GPS_LAT && count >= 3 && offset + 24 <= static_cast<quint32>(tiffData.size()))
            {
                quint32 n = 0;
                quint32 d = 0;
                for (int k = 0; k < 3; ++k)
                {
                    (void)readRational(raw + offset + k * 8, isLittle, latVals[k], n, d);
                }
                hasLat = true;
            }
            else if (tag == TAG_GPS_LON && count >= 3 && offset + 24 <= static_cast<quint32>(tiffData.size()))
            {
                quint32 n = 0;
                quint32 d = 0;
                for (int k = 0; k < 3; ++k)
                {
                    (void)readRational(raw + offset + k * 8, isLittle, lonVals[k], n, d);
                }
                hasLon = true;
            }
        });

        if (hasLat && hasLon)
        {
            out.latitude = latVals[0] + (latVals[1] / 60.0) + (latVals[2] / 3600.0);
            if (latRef.compare(QStringLiteral("S"), Qt::CaseInsensitive) == 0)
            {
                out.latitude = -out.latitude;
            }

            out.longitude = lonVals[0] + (lonVals[1] / 60.0) + (lonVals[2] / 3600.0);
            if (lonRef.compare(QStringLiteral("W"), Qt::CaseInsensitive) == 0)
            {
                out.longitude = -out.longitude;
            }

            out.hasGps = true;
        }
    }

    // Format final fields
    out.cameraModel = buildCameraModel(out.make, out.model);

    const QString dateToUse = !origDateStr.isEmpty() ? origDateStr : dateStr;
    if (!dateToUse.isEmpty())
    {
        out.dateTaken = QDateTime::fromString(dateToUse, QStringLiteral("yyyy:MM:dd HH:mm:ss"));
    }

    return !out.cameraModel.isEmpty() || !out.lensModel.isEmpty() ||
           !out.aperture.isEmpty() || !out.shutterSpeed.isEmpty() ||
           out.iso > 0 || out.dateTaken.isValid();
}

QString ExifParser::formatAperture(double fNumber)
{
    if (fNumber <= 0.0)
    {
        return {};
    }
    // E.g. f/2.8, f/1.8, f/4, f/5.6
    if (std::abs(fNumber - std::round(fNumber)) < 0.05)
    {
        return QString::asprintf("f/%d", static_cast<int>(std::round(fNumber)));
    }
    return QString::asprintf("f/%.1f", fNumber);
}

QString ExifParser::formatShutterSpeed(double exposureTime)
{
    if (exposureTime <= 0.0)
    {
        return {};
    }

    if (exposureTime < 1.0)
    {
        const int denominator = static_cast<int>(std::round(1.0 / exposureTime));
        return QString::asprintf("1/%ds", denominator);
    }

    if (std::abs(exposureTime - std::round(exposureTime)) < 0.05)
    {
        return QString::asprintf("%ds", static_cast<int>(std::round(exposureTime)));
    }
    return QString::asprintf("%.1fs", exposureTime);
}

QString ExifParser::formatFocalLength(double focalLengthMm)
{
    if (focalLengthMm <= 0.0)
    {
        return {};
    }
    if (std::abs(focalLengthMm - std::round(focalLengthMm)) < 0.05)
    {
        return QString::asprintf("%d mm", static_cast<int>(std::round(focalLengthMm)));
    }
    return QString::asprintf("%.1f mm", focalLengthMm);
}

QString ExifParser::buildCameraModel(const QString &make, const QString &model)
{
    if (model.isEmpty())
    {
        return make;
    }
    if (make.isEmpty())
    {
        return model;
    }

    // Clean common vendor naming quirks: "SONY" -> "Sony", "NIKON CORPORATION" -> "Nikon"
    QString cleanMake = make;
    if (cleanMake.startsWith(QStringLiteral("NIKON"), Qt::CaseInsensitive))
    {
        cleanMake = QStringLiteral("Nikon");
    }
    else if (cleanMake.startsWith(QStringLiteral("SONY"), Qt::CaseInsensitive))
    {
        cleanMake = QStringLiteral("Sony");
    }
    else if (cleanMake.startsWith(QStringLiteral("CANON"), Qt::CaseInsensitive))
    {
        cleanMake = QStringLiteral("Canon");
    }
    else if (cleanMake.startsWith(QStringLiteral("FUJIFILM"), Qt::CaseInsensitive))
    {
        cleanMake = QStringLiteral("Fujifilm");
    }
    else if (cleanMake.startsWith(QStringLiteral("PANASONIC"), Qt::CaseInsensitive))
    {
        cleanMake = QStringLiteral("Panasonic");
    }

    if (model.startsWith(cleanMake, Qt::CaseInsensitive))
    {
        return model;
    }

    return cleanMake + QStringLiteral(" ") + model;
}

} // namespace PhotoCarta
