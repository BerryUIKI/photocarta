// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "exifdata.h"

#include <QByteArray>
#include <QString>

namespace PhotoCarta
{

class ExifParser
{
public:
    [[nodiscard]] static ExifData parseFile(const QString &filePath);
    [[nodiscard]] static ExifData parseData(const QByteArray &data);

private:
    [[nodiscard]] static bool extractJpegExif(const QByteArray &jpegData, QByteArray &tiffData);
    [[nodiscard]] static bool parseTiffBlock(const QByteArray &tiffData, ExifData &out);

    [[nodiscard]] static quint16 readU16(const uchar *p, bool isLittle);
    [[nodiscard]] static quint32 readU32(const uchar *p, bool isLittle);
    [[nodiscard]] static bool readRational(const uchar *p, bool isLittle, double &val, quint32 &num, quint32 &den);
    [[nodiscard]] static QString readAscii(const QByteArray &data, quint32 offset, quint32 count);

    static QString formatAperture(double fNumber);
    static QString formatShutterSpeed(double exposureTime);
    static QString formatFocalLength(double focalLengthMm);
    static QString buildCameraModel(const QString &make, const QString &model);
};

} // namespace PhotoCarta
