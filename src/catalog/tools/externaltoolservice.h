// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "catalogitem.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace PhotoCarta
{

/**
 * @brief Result of planning a handoff to an external tool.
 */
enum class ToolLaunchStatus
{
    Ok = 0,
    NoSelection,
    MultipleSelection,
    DemoFixture,
    CollectionOffline,
    SourceFileNotFound,
    ToolNotConfigured,
    ToolNotFound,
    LaunchFailed
};

/**
 * @brief Resolved command for opening one catalog item in an external tool.
 */
struct ToolLaunchPlan
{
    QString program;
    QStringList arguments;
    QString filePath;
    QString fileName;
    ToolLaunchStatus status = ToolLaunchStatus::Ok;
    QString message;

    [[nodiscard]] bool canLaunch() const { return status == ToolLaunchStatus::Ok; }
    [[nodiscard]] QVariantMap toVariantMap() const;
};

/**
 * @brief Hands selected photos to an external editor process.
 *
 * The retained digiKam tools are not part of this build, so the first
 * integration step launches an installed editor as a separate process. The
 * service validates the item exactly like the destructive file operations do
 * (demo fixtures, offline collections, and missing files are refused) and
 * reports every unsupported case instead of failing silently.
 *
 * The editor is resolved from the PHOTOCARTA_EXTERNAL_EDITOR environment
 * variable, then the saved setting, then common installation locations. A
 * configured path that does not exist is reported as an error rather than
 * silently replaced by an auto-detected tool.
 */
class ExternalToolService
{
public:
    /// Environment variable that overrides the saved editor path
    static constexpr char EditorEnvironmentVariable[] = "PHOTOCARTA_EXTERNAL_EDITOR";

    /// Editor locations discovered in PATH and standard install directories
    [[nodiscard]] static QStringList autodetectedEditorPaths();

    /// Resolves the editor executable; reports why resolution failed through status
    [[nodiscard]] static QString resolveEditorPath(const QString &configuredEditor,
                                                   ToolLaunchStatus *status = nullptr);

    /// Validates the items and builds the command for opening the single selected item
    [[nodiscard]] static ToolLaunchPlan planFor(const QList<CatalogItem> &items,
                                                const QString &configuredEditor);

    /// Starts the editor detached; the photo path is passed as an argument, never through a shell
    [[nodiscard]] static bool launch(const QString &program,
                                     const QStringList &arguments,
                                     QString *errorMessage = nullptr);

    /// Human-readable description of a launch status
    [[nodiscard]] static QString statusMessage(ToolLaunchStatus status, const QString &context);
};

} // namespace PhotoCarta
