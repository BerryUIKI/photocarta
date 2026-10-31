// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "externaltoolservice.h"

#include <QFileInfo>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QtGlobal>

namespace PhotoCarta
{

QVariantMap ToolLaunchPlan::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("program"), program);
    map.insert(QStringLiteral("arguments"), arguments);
    map.insert(QStringLiteral("filePath"), filePath);
    map.insert(QStringLiteral("fileName"), fileName);
    map.insert(QStringLiteral("status"), static_cast<int>(status));
    map.insert(QStringLiteral("message"), message);
    return map;
}

QStringList ExternalToolService::autodetectedEditorPaths()
{
    QStringList paths;

    const auto appendIfExecutable = [&paths](const QString &candidate) {
        const QString trimmed = candidate.trimmed();
        if (trimmed.isEmpty())
        {
            return;
        }
        const QFileInfo info(trimmed);
        if (!info.isFile() || !info.isExecutable())
        {
            return;
        }
        const QString absolutePath = info.absoluteFilePath();
        if (!paths.contains(absolutePath))
        {
            paths.append(absolutePath);
        }
    };

    // PATH covers Linux packages and Windows/macOS installs that register a shim.
    appendIfExecutable(QStandardPaths::findExecutable(QStringLiteral("showfoto")));
    appendIfExecutable(QStandardPaths::findExecutable(QStringLiteral("digikam")));

#ifdef Q_OS_WIN
    const QStringList programRoots = {
        qEnvironmentVariable("ProgramFiles"),
        qEnvironmentVariable("ProgramFiles(x86)"),
    };
    for (const QString &root : programRoots)
    {
        if (root.isEmpty())
        {
            continue;
        }
        const QString installDir = root + QStringLiteral("/digiKam");
        appendIfExecutable(installDir + QStringLiteral("/showfoto.exe"));
        appendIfExecutable(installDir + QStringLiteral("/digikam.exe"));
    }
#elif defined(Q_OS_MACOS)
    appendIfExecutable(QStringLiteral("/Applications/digiKam.org/digikam.app/Contents/MacOS/digikam"));
    appendIfExecutable(QStringLiteral("/Applications/ShowFoto.app/Contents/MacOS/showfoto"));
    appendIfExecutable(QStringLiteral("/Applications/digiKam.app/Contents/MacOS/digikam"));
#endif

    return paths;
}

QString ExternalToolService::resolveEditorPath(const QString &configuredEditor, ToolLaunchStatus *status)
{
    const auto setStatus = [status](ToolLaunchStatus value) {
        if (status)
        {
            *status = value;
        }
    };

    // An explicit configuration always wins over auto-detection. Falling back
    // silently would hide a typo or a removed installation.
    const QString environmentPath = qEnvironmentVariable(EditorEnvironmentVariable).trimmed();
    const QString explicitPath = !environmentPath.isEmpty() ? environmentPath : configuredEditor.trimmed();

    if (!explicitPath.isEmpty())
    {
        const QFileInfo info(explicitPath);
        if (info.isFile() && info.isExecutable())
        {
            setStatus(ToolLaunchStatus::Ok);
            return info.absoluteFilePath();
        }
        setStatus(ToolLaunchStatus::ToolNotFound);
        return explicitPath;
    }

    const QStringList detected = autodetectedEditorPaths();
    if (!detected.isEmpty())
    {
        setStatus(ToolLaunchStatus::Ok);
        return detected.first();
    }

    setStatus(ToolLaunchStatus::ToolNotConfigured);
    return QString();
}

ToolLaunchPlan ExternalToolService::planFor(const QList<CatalogItem> &items,
                                            const QString &configuredEditor)
{
    ToolLaunchPlan plan;

    if (items.isEmpty())
    {
        plan.status = ToolLaunchStatus::NoSelection;
        plan.message = statusMessage(plan.status, QString());
        return plan;
    }

    if (items.size() > 1)
    {
        plan.status = ToolLaunchStatus::MultipleSelection;
        plan.message = statusMessage(plan.status, QString::number(items.size()));
        return plan;
    }

    const CatalogItem &item = items.first();
    plan.filePath = item.filePath;
    plan.fileName = item.fileName;

    if (item.isDemoFixture || item.filePath.startsWith(QStringLiteral("qrc:")))
    {
        plan.status = ToolLaunchStatus::DemoFixture;
    }
    else if (item.isMissing)
    {
        plan.status = ToolLaunchStatus::CollectionOffline;
    }
    else if (!QFileInfo::exists(item.filePath))
    {
        plan.status = ToolLaunchStatus::SourceFileNotFound;
    }

    if (plan.status != ToolLaunchStatus::Ok)
    {
        plan.message = statusMessage(plan.status, plan.fileName);
        return plan;
    }

    ToolLaunchStatus toolStatus = ToolLaunchStatus::Ok;
    const QString program = resolveEditorPath(configuredEditor, &toolStatus);
    if (toolStatus != ToolLaunchStatus::Ok)
    {
        plan.status = toolStatus;
        plan.message = statusMessage(toolStatus, program);
        return plan;
    }

    plan.program = program;
    plan.arguments = QStringList{QFileInfo(item.filePath).absoluteFilePath()};
    plan.status = ToolLaunchStatus::Ok;
    plan.message = statusMessage(ToolLaunchStatus::Ok, plan.fileName);
    return plan;
}

bool ExternalToolService::launch(const QString &program,
                                 const QStringList &arguments,
                                 QString *errorMessage)
{
    if (program.isEmpty())
    {
        if (errorMessage)
        {
            *errorMessage = statusMessage(ToolLaunchStatus::ToolNotConfigured, QString());
        }
        return false;
    }

    // startDetached takes an argument list, so the photo path is never parsed
    // by a shell and cannot inject commands.
    const bool started = QProcess::startDetached(program, arguments);
    if (!started && errorMessage)
    {
        *errorMessage = statusMessage(ToolLaunchStatus::LaunchFailed, program);
    }
    return started;
}

QString ExternalToolService::statusMessage(ToolLaunchStatus status, const QString &context)
{
    switch (status)
    {
    case ToolLaunchStatus::Ok:
        return QObject::tr("Ready to open %1 in the external editor").arg(context);
    case ToolLaunchStatus::NoSelection:
        return QObject::tr("Select a single photo to open in the external editor.");
    case ToolLaunchStatus::MultipleSelection:
        return QObject::tr("%1 photos are selected; the external editor opens one photo at a time.")
            .arg(context);
    case ToolLaunchStatus::DemoFixture:
        return QObject::tr("%1 is a built-in demo photo and has no file on disk.").arg(context);
    case ToolLaunchStatus::CollectionOffline:
        return QObject::tr("%1 is on a collection that is currently offline; open it once the collection is available again.")
            .arg(context);
    case ToolLaunchStatus::SourceFileNotFound:
        return QObject::tr("%1 could not be found on disk.").arg(context);
    case ToolLaunchStatus::ToolNotConfigured:
        return QObject::tr("No external editor is configured. Set the %1 environment variable to the editor executable, or install digiKam or ShowFoto in a standard location.")
            .arg(QString::fromLatin1(EditorEnvironmentVariable));
    case ToolLaunchStatus::ToolNotFound:
        return QObject::tr("The external editor could not be found or is not executable: %1").arg(context);
    case ToolLaunchStatus::LaunchFailed:
        return QObject::tr("The external editor could not be started: %1").arg(context);
    }
    return QString();
}

} // namespace PhotoCarta
