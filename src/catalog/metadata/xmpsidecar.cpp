// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xmpsidecar.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSaveFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace
{
Q_LOGGING_CATEGORY(lcXmp, "photocarta.metadata.xmp")
}

namespace PhotoCarta
{

QString XmpSidecar::sidecarPathForImage(const QString &imageFilePath, bool requireExists)
{
    if (imageFilePath.isEmpty())
    {
        return {};
    }

    const QFileInfo fi(imageFilePath);
    const QString standardSidecar = imageFilePath + QStringLiteral(".xmp");
    if (QFile::exists(standardSidecar))
    {
        return standardSidecar;
    }

    // Secondary check: base name sidecar (e.g. photo.xmp for photo.jpg)
    const QString baseSidecar = fi.dir().filePath(fi.completeBaseName() + QStringLiteral(".xmp"));
    if (QFile::exists(baseSidecar))
    {
        return baseSidecar;
    }

    if (requireExists)
    {
        return {};
    }

    // Default target for creation matches standard digiKam/darktable convention
    return standardSidecar;
}

bool XmpSidecar::hasSidecar(const QString &imageFilePath)
{
    return !sidecarPathForImage(imageFilePath, true).isEmpty();
}

QString XmpSidecar::colorLabelToString(int colorLabel)
{
    switch (colorLabel)
    {
    case 1:
        return QStringLiteral("Red");
    case 2:
        return QStringLiteral("Yellow");
    case 3:
        return QStringLiteral("Green");
    case 4:
        return QStringLiteral("Blue");
    case 5:
        return QStringLiteral("Purple");
    default:
        return {};
    }
}

int XmpSidecar::stringToColorLabel(const QString &labelName)
{
    const QString lower = labelName.trimmed().toLower();
    if (lower == QStringLiteral("red") || lower == QStringLiteral("1"))
    {
        return 1;
    }
    if (lower == QStringLiteral("yellow") || lower == QStringLiteral("2"))
    {
        return 2;
    }
    if (lower == QStringLiteral("green") || lower == QStringLiteral("3"))
    {
        return 3;
    }
    if (lower == QStringLiteral("blue") || lower == QStringLiteral("4"))
    {
        return 4;
    }
    if (lower == QStringLiteral("purple") || lower == QStringLiteral("violet") || lower == QStringLiteral("5"))
    {
        return 5;
    }
    return 0;
}

bool XmpSidecar::readFromFile(const QString &sidecarPath, XmpMetadata &outMetadata)
{
    QFile file(sidecarPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qCDebug(lcXmp) << "Failed to open XMP sidecar for reading:" << sidecarPath << file.errorString();
        return false;
    }

    const QByteArray data = file.readAll();
    return readFromData(data, outMetadata);
}

bool XmpSidecar::readFromData(const QByteArray &data, XmpMetadata &outMetadata)
{
    if (data.isEmpty())
    {
        return false;
    }

    QXmlStreamReader reader(data);
    QString currentContainer; // e.g. "title", "description", "subject", "TagsList"

    while (!reader.atEnd())
    {
        reader.readNext();

        if (reader.isStartElement())
        {
            const QStringRef name = reader.name();

            // 1. Inspect attributes on elements like rdf:Description
            for (const QXmlStreamAttribute &attr : reader.attributes())
            {
                const QStringRef attrName = attr.name();
                const QString val = attr.value().toString().trimmed();

                if (attrName == QStringLiteral("Rating"))
                {
                    outMetadata.rating = qBound(0, val.toInt(), 5);
                }
                else if (attrName == QStringLiteral("Label"))
                {
                    outMetadata.colorLabel = stringToColorLabel(val);
                }
                else if (attrName == QStringLiteral("Urgency") || attrName == QStringLiteral("Pick"))
                {
                    if (val == QStringLiteral("1"))
                    {
                        outMetadata.isFlagged = true;
                    }
                }
                else if (attrName == QStringLiteral("CreateDate") || attrName == QStringLiteral("DateTimeOriginal"))
                {
                    outMetadata.dateTaken = QDateTime::fromString(val, Qt::ISODate);
                }
            }

            // 2. Identify container elements
            if (name == QStringLiteral("title"))
            {
                currentContainer = QStringLiteral("title");
            }
            else if (name == QStringLiteral("description"))
            {
                currentContainer = QStringLiteral("description");
            }
            else if (name == QStringLiteral("subject") || name == QStringLiteral("TagsList"))
            {
                currentContainer = QStringLiteral("tags");
            }
            else if (name == QStringLiteral("Rating"))
            {
                const QString text = reader.readElementText().trimmed();
                outMetadata.rating = qBound(0, text.toInt(), 5);
            }
            else if (name == QStringLiteral("Label"))
            {
                const QString text = reader.readElementText().trimmed();
                outMetadata.colorLabel = stringToColorLabel(text);
            }
            else if (name == QStringLiteral("Urgency") || name == QStringLiteral("Pick"))
            {
                const QString text = reader.readElementText().trimmed();
                if (text == QStringLiteral("1"))
                {
                    outMetadata.isFlagged = true;
                }
            }
            else if (name == QStringLiteral("CreateDate") || name == QStringLiteral("DateTimeOriginal"))
            {
                const QString text = reader.readElementText().trimmed();
                outMetadata.dateTaken = QDateTime::fromString(text, Qt::ISODate);
            }
            else if (name == QStringLiteral("li"))
            {
                // Item inside a container
                const QString text = reader.readElementText().trimmed();
                if (currentContainer == QStringLiteral("title"))
                {
                    if (outMetadata.title.isEmpty() && !text.isEmpty())
                    {
                        outMetadata.title = text;
                    }
                }
                else if (currentContainer == QStringLiteral("description"))
                {
                    if (outMetadata.caption.isEmpty() && !text.isEmpty())
                    {
                        outMetadata.caption = text;
                    }
                }
                else if (currentContainer == QStringLiteral("tags"))
                {
                    if (!text.isEmpty() && !outMetadata.tags.contains(text))
                    {
                        outMetadata.tags.append(text);
                    }
                }
            }
        }
        else if (reader.isEndElement())
        {
            const QStringRef name = reader.name();
            if (name == QStringLiteral("title") || name == QStringLiteral("description") || name == QStringLiteral("subject")
                || name == QStringLiteral("TagsList"))
            {
                currentContainer.clear();
            }
        }
    }

    if (reader.hasError())
    {
        qCDebug(lcXmp) << "XML parsing error in XMP data:" << reader.errorString();
    }

    return !outMetadata.isEmpty();
}

QByteArray XmpSidecar::generateXmpData(const XmpMetadata &metadata)
{
    QByteArray output;
    QBuffer buffer(&output);
    buffer.open(QIODevice::WriteOnly);

    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);

    writer.writeStartDocument(QStringLiteral("1.0"), true);

    // <x:xmpmeta>
    writer.writeStartElement(QStringLiteral("x:xmpmeta"));
    writer.writeNamespace(QStringLiteral("adobe:ns:meta/"), QStringLiteral("x"));
    writer.writeAttribute(QStringLiteral("x:xmptk"), QStringLiteral("PhotoCarta XMP Core 1.0"));

    // <rdf:RDF>
    writer.writeStartElement(QStringLiteral("rdf:RDF"));
    writer.writeNamespace(QStringLiteral("http://www.w3.org/1999/02/22-rdf-syntax-ns#"), QStringLiteral("rdf"));

    // <rdf:Description>
    writer.writeStartElement(QStringLiteral("rdf:Description"));
    writer.writeAttribute(QStringLiteral("rdf:about"), QString());
    writer.writeNamespace(QStringLiteral("http://ns.adobe.com/xap/1.0/"), QStringLiteral("xmp"));
    writer.writeNamespace(QStringLiteral("http://ns.adobe.com/photoshop/1.0/"), QStringLiteral("photoshop"));
    writer.writeNamespace(QStringLiteral("http://purl.org/dc/elements/1.1/"), QStringLiteral("dc"));
    writer.writeNamespace(QStringLiteral("http://www.digikam.org/ns/1.0/"), QStringLiteral("digiKam"));

    if (metadata.rating > 0)
    {
        writer.writeAttribute(QStringLiteral("xmp:Rating"), QString::number(metadata.rating));
    }
    if (metadata.colorLabel > 0)
    {
        writer.writeAttribute(QStringLiteral("xmp:Label"), colorLabelToString(metadata.colorLabel));
    }
    if (metadata.isFlagged)
    {
        writer.writeAttribute(QStringLiteral("photoshop:Urgency"), QStringLiteral("1"));
    }
    if (metadata.dateTaken.isValid())
    {
        writer.writeAttribute(QStringLiteral("xmp:CreateDate"), metadata.dateTaken.toString(Qt::ISODate));
    }

    // <dc:title>
    if (!metadata.title.isEmpty())
    {
        writer.writeStartElement(QStringLiteral("dc:title"));
        writer.writeStartElement(QStringLiteral("rdf:Alt"));
        writer.writeStartElement(QStringLiteral("rdf:li"));
        writer.writeAttribute(QStringLiteral("xml:lang"), QStringLiteral("x-default"));
        writer.writeCharacters(metadata.title);
        writer.writeEndElement(); // rdf:li
        writer.writeEndElement(); // rdf:Alt
        writer.writeEndElement(); // dc:title
    }

    // <dc:description>
    if (!metadata.caption.isEmpty())
    {
        writer.writeStartElement(QStringLiteral("dc:description"));
        writer.writeStartElement(QStringLiteral("rdf:Alt"));
        writer.writeStartElement(QStringLiteral("rdf:li"));
        writer.writeAttribute(QStringLiteral("xml:lang"), QStringLiteral("x-default"));
        writer.writeCharacters(metadata.caption);
        writer.writeEndElement(); // rdf:li
        writer.writeEndElement(); // rdf:Alt
        writer.writeEndElement(); // dc:description
    }

    // <dc:subject> (Standard Dublin Core tags)
    if (!metadata.tags.isEmpty())
    {
        writer.writeStartElement(QStringLiteral("dc:subject"));
        writer.writeStartElement(QStringLiteral("rdf:Bag"));
        for (const QString &tag : metadata.tags)
        {
            writer.writeTextElement(QStringLiteral("rdf:li"), tag);
        }
        writer.writeEndElement(); // rdf:Bag
        writer.writeEndElement(); // dc:subject

        // <digiKam:TagsList> (digiKam tags hierarchy / list)
        writer.writeStartElement(QStringLiteral("digiKam:TagsList"));
        writer.writeStartElement(QStringLiteral("rdf:Seq"));
        for (const QString &tag : metadata.tags)
        {
            writer.writeTextElement(QStringLiteral("rdf:li"), tag);
        }
        writer.writeEndElement(); // rdf:Seq
        writer.writeEndElement(); // digiKam:TagsList
    }

    writer.writeEndElement(); // rdf:Description
    writer.writeEndElement(); // rdf:RDF
    writer.writeEndElement(); // x:xmpmeta
    writer.writeEndDocument();

    return output;
}

bool XmpSidecar::writeToFile(const QString &sidecarPath, const XmpMetadata &metadata)
{
    if (sidecarPath.isEmpty())
    {
        return false;
    }

    XmpMetadata merged = metadata;
    if (QFile::exists(sidecarPath))
    {
        XmpMetadata existing;
        if (readFromFile(sidecarPath, existing))
        {
            if (merged.title.isEmpty() && !existing.title.isEmpty())
            {
                merged.title = existing.title;
            }
            if (merged.caption.isEmpty() && !existing.caption.isEmpty())
            {
                merged.caption = existing.caption;
            }
            if (merged.rating == 0 && existing.rating > 0)
            {
                merged.rating = existing.rating;
            }
            if (merged.colorLabel == 0 && existing.colorLabel > 0)
            {
                merged.colorLabel = existing.colorLabel;
            }
            if (!merged.dateTaken.isValid() && existing.dateTaken.isValid())
            {
                merged.dateTaken = existing.dateTaken;
            }
            if (merged.tags.isEmpty() && !existing.tags.isEmpty())
            {
                merged.tags = existing.tags;
            }
        }
    }

    // Atomic write using QSaveFile
    QSaveFile saveFile(sidecarPath);
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qCDebug(lcXmp) << "Failed to open XMP sidecar for writing:" << sidecarPath << saveFile.errorString();
        return false;
    }

    const QByteArray xmlData = generateXmpData(merged);
    if (saveFile.write(xmlData) == -1)
    {
        qCDebug(lcXmp) << "Failed to write XML data to:" << sidecarPath << saveFile.errorString();
        saveFile.cancelWriting();
        return false;
    }

    if (!saveFile.commit())
    {
        qCDebug(lcXmp) << "Failed to commit XMP sidecar file:" << sidecarPath << saveFile.errorString();
        return false;
    }

    return true;
}

} // namespace PhotoCarta
