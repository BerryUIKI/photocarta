// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>

namespace PhotoCarta
{

/**
 * @brief Structured container for metadata stored in ISO 16684-1 compliant XMP sidecar files.
 */
struct XmpMetadata
{
    QString title;
    QString caption;
    int rating = 0;      // 0..5 stars
    int colorLabel = 0;  // 0=None, 1=Red, 2=Yellow, 3=Green, 4=Blue, 5=Purple
    bool isFlagged = false;
    QStringList tags;
    QDateTime dateTaken;

    [[nodiscard]] bool isEmpty() const
    {
        return title.isEmpty() && caption.isEmpty() && rating == 0 && colorLabel == 0 && !isFlagged && tags.isEmpty()
               && !dateTaken.isValid();
    }
};

/**
 * @brief Reader and writer for industry-standard XMP sidecar files (.xmp).
 *
 * Implements interoperability with digiKam, Adobe Lightroom, darktable, and Exiv2:
 * - Identifies standard sidecar paths (<file>.<ext>.xmp and <file>.xmp).
 * - Reads and writes XMP ratings (xmp:Rating).
 * - Reads and writes color labels (xmp:Label="Red"|"Yellow"|"Green"|"Blue"|"Purple").
 * - Reads and writes pick/flagged status (photoshop:Urgency="1", xmp:Pick="1").
 * - Reads and writes title (dc:title/rdf:Alt/rdf:li) and caption (dc:description/rdf:Alt/rdf:li).
 * - Reads and writes tags (dc:subject/rdf:Bag/rdf:li and digiKam:TagsList/rdf:Seq/rdf:li).
 * - Uses atomic write operations via QSaveFile to prevent corrupt sidecar files.
 */
class XmpSidecar
{
public:
    /// Returns the sidecar file path for an image.
    /// If requireExists is true, looks for <image>.xmp and <basename>.xmp, returning empty if not found.
    /// If requireExists is false, returns the canonical <image>.xmp path for writing.
    [[nodiscard]] static QString sidecarPathForImage(const QString &imageFilePath, bool requireExists = false);

    /// Checks if an XMP sidecar file exists for the given image.
    [[nodiscard]] static bool hasSidecar(const QString &imageFilePath);

    /// Reads metadata from a sidecar file.
    [[nodiscard]] static bool readFromFile(const QString &sidecarPath, XmpMetadata &outMetadata);

    /// Reads metadata from raw XML data.
    [[nodiscard]] static bool readFromData(const QByteArray &data, XmpMetadata &outMetadata);

    /// Writes metadata to a sidecar file atomically.
    [[nodiscard]] static bool writeToFile(const QString &sidecarPath, const XmpMetadata &metadata);

    /// Generates ISO 16684-1 compliant XMP RDF XML data.
    [[nodiscard]] static QByteArray generateXmpData(const XmpMetadata &metadata);

    /// Color label conversion helpers
    [[nodiscard]] static QString colorLabelToString(int colorLabel);
    [[nodiscard]] static int stringToColorLabel(const QString &labelName);
};

} // namespace PhotoCarta
