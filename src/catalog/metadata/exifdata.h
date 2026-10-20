// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QSize>
#include <QString>

namespace PhotoCarta
{

struct ExifData
{
    bool isValid = false;

    QString make;
    QString model;
    QString cameraModel;
    QString lensMake;
    QString lensModel;
    QString focalLength;
    QString aperture;
    QString shutterSpeed;
    int iso = 0;
    int orientation = 1;

    QDateTime dateTaken;
    QDateTime dateDigitized;
    QSize dimensions;

    bool hasGps = false;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
};

} // namespace PhotoCarta
