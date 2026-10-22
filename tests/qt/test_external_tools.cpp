// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogitem.h"
#include "catalog/catalogmodel.h"
#include "catalog/selectionmanager.h"
#include "catalog/sessioncontroller.h"
#include "catalog/tools/externaltoolservice.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

// Behavioral tests for handing a selected photo to an external editor process.
//
// The service layer is tested directly, and the controller is exercised through
// a real model/selection so the refusal paths and the successful handoff match
// what the interface triggers.

class TestExternalTools : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testPlanStatusMatrix();
    void testPlanForReadyItem();
    void testResolvePrecedence();
    void testResolveRejectsInvalidConfiguredPath();
    void testLaunchFailsForMissingProgram();
    void testControllerReportsNoSelection();
    void testControllerReportsMultipleSelection();
    void testControllerRefusesDemoFixture();
    void testControllerOpensSelectedPhoto();

private:
    [[nodiscard]] QString toolStubPath() const;
    QString writePhoto(const QString &name) const;

    QTemporaryDir m_tempDir;
    QString m_stubPath;
};

void TestExternalTools::init()
{
    QVERIFY(m_tempDir.isValid());
    qunsetenv(ExternalToolService::EditorEnvironmentVariable);
    qunsetenv("PHOTOCARTA_TOOL_STUB_OUTPUT");
    m_stubPath = toolStubPath();
}

void TestExternalTools::cleanup()
{
    qunsetenv(ExternalToolService::EditorEnvironmentVariable);
    qunsetenv("PHOTOCARTA_TOOL_STUB_OUTPUT");
}

QString TestExternalTools::toolStubPath() const
{
    const QString base = QCoreApplication::applicationDirPath() + QStringLiteral("/photocarta_tool_stub");
    if (QFileInfo::exists(base))
    {
        return base;
    }
    const QString executable = base + QStringLiteral(".exe");
    if (QFileInfo::exists(executable))
    {
        return executable;
    }
    return QString();
}

QString TestExternalTools::writePhoto(const QString &name) const
{
    const QString path = m_tempDir.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return QString();
    }
    file.write("photo");
    file.close();
    return path;
}

void TestExternalTools::testPlanStatusMatrix()
{
    QVERIFY(!m_stubPath.isEmpty());

    CatalogItem demo;
    demo.id = QStringLiteral("demo");
    demo.filePath = QStringLiteral("qrc:/fixtures/demo.jpg");
    demo.fileName = QStringLiteral("demo.jpg");
    demo.isDemoFixture = true;

    CatalogItem offline;
    offline.id = QStringLiteral("offline");
    offline.filePath = m_tempDir.filePath(QStringLiteral("offline.jpg"));
    offline.fileName = QStringLiteral("offline.jpg");
    offline.isMissing = true;

    CatalogItem absent;
    absent.id = QStringLiteral("absent");
    absent.filePath = m_tempDir.filePath(QStringLiteral("absent.jpg"));
    absent.fileName = QStringLiteral("absent.jpg");

    const QString realPath = writePhoto(QStringLiteral("real.jpg"));
    QVERIFY(!realPath.isEmpty());
    CatalogItem real;
    real.id = QStringLiteral("real");
    real.filePath = realPath;
    real.fileName = QStringLiteral("real.jpg");

    QCOMPARE(ExternalToolService::planFor({}, m_stubPath).status, ToolLaunchStatus::NoSelection);
    QCOMPARE(ExternalToolService::planFor({demo, real}, m_stubPath).status,
             ToolLaunchStatus::MultipleSelection);
    QCOMPARE(ExternalToolService::planFor({demo}, m_stubPath).status, ToolLaunchStatus::DemoFixture);
    QCOMPARE(ExternalToolService::planFor({offline}, m_stubPath).status,
             ToolLaunchStatus::CollectionOffline);
    QCOMPARE(ExternalToolService::planFor({absent}, m_stubPath).status,
             ToolLaunchStatus::SourceFileNotFound);

    // A configured editor that does not exist is reported, not replaced by auto-detection
    const ToolLaunchPlan noTool =
        ExternalToolService::planFor({real}, m_tempDir.filePath(QStringLiteral("missing-editor")));
    QCOMPARE(noTool.status, ToolLaunchStatus::ToolNotFound);
    QVERIFY(!noTool.canLaunch());
    QVERIFY(!noTool.message.isEmpty());

    // Media refusals win over a missing editor so the message names the real cause
    QCOMPARE(ExternalToolService::planFor({demo}, m_tempDir.filePath(QStringLiteral("missing-editor"))).status,
             ToolLaunchStatus::DemoFixture);
}

void TestExternalTools::testPlanForReadyItem()
{
    QVERIFY(!m_stubPath.isEmpty());

    const QString realPath = writePhoto(QStringLiteral("ready.jpg"));
    QVERIFY(!realPath.isEmpty());

    CatalogItem item;
    item.id = QStringLiteral("ready");
    item.filePath = realPath;
    item.fileName = QStringLiteral("ready.jpg");

    const ToolLaunchPlan plan = ExternalToolService::planFor({item}, m_stubPath);
    QCOMPARE(plan.status, ToolLaunchStatus::Ok);
    QVERIFY(plan.canLaunch());
    QCOMPARE(plan.program, QFileInfo(m_stubPath).absoluteFilePath());
    QCOMPARE(plan.arguments, QStringList{QFileInfo(realPath).absoluteFilePath()});
    QCOMPARE(plan.fileName, QStringLiteral("ready.jpg"));

    const QVariantMap map = plan.toVariantMap();
    QCOMPARE(map.value(QStringLiteral("filePath")).toString(), realPath);
    QCOMPARE(map.value(QStringLiteral("status")).toInt(), static_cast<int>(ToolLaunchStatus::Ok));
}

void TestExternalTools::testResolvePrecedence()
{
    QVERIFY(!m_stubPath.isEmpty());

    // The environment variable overrides the saved setting.
    const QString otherExecutable = QCoreApplication::applicationFilePath();
    QVERIFY(QFileInfo(otherExecutable).isExecutable());

    qputenv(ExternalToolService::EditorEnvironmentVariable, m_stubPath.toUtf8());
    ToolLaunchStatus status = ToolLaunchStatus::LaunchFailed;
    QCOMPARE(ExternalToolService::resolveEditorPath(otherExecutable, &status),
             QFileInfo(m_stubPath).absoluteFilePath());
    QCOMPARE(status, ToolLaunchStatus::Ok);

    // Without the override the saved setting is used.
    qunsetenv(ExternalToolService::EditorEnvironmentVariable);
    QCOMPARE(ExternalToolService::resolveEditorPath(m_stubPath, &status),
             QFileInfo(m_stubPath).absoluteFilePath());
    QCOMPARE(status, ToolLaunchStatus::Ok);

    // With nothing configured, auto-detection either finds an installed editor
    // or reports that none is configured; both outcomes must be consistent.
    const QString detected = ExternalToolService::resolveEditorPath(QString(), &status);
    if (status == ToolLaunchStatus::Ok)
    {
        QVERIFY(QFileInfo(detected).isFile());
        QVERIFY(QFileInfo(detected).isExecutable());
    }
    else
    {
        QCOMPARE(status, ToolLaunchStatus::ToolNotConfigured);
        QVERIFY(detected.isEmpty());
    }
}

void TestExternalTools::testResolveRejectsInvalidConfiguredPath()
{
    const QString missing = m_tempDir.filePath(QStringLiteral("no-such-editor"));

    ToolLaunchStatus status = ToolLaunchStatus::Ok;
    QCOMPARE(ExternalToolService::resolveEditorPath(missing, &status), missing);
    QCOMPARE(status, ToolLaunchStatus::ToolNotFound);

    // A directory is not an executable editor.
    status = ToolLaunchStatus::Ok;
    QCOMPARE(ExternalToolService::resolveEditorPath(m_tempDir.path(), &status), m_tempDir.path());
    QCOMPARE(status, ToolLaunchStatus::ToolNotFound);
}

void TestExternalTools::testLaunchFailsForMissingProgram()
{
    QString error;
    QVERIFY(!ExternalToolService::launch(m_tempDir.filePath(QStringLiteral("missing-tool")), {}, &error));
    QVERIFY(!error.isEmpty());
}

void TestExternalTools::testControllerReportsNoSelection()
{
    CatalogModel model;
    CatalogFilterModel filterModel;
    filterModel.setSourceModel(&model);
    SelectionManager selection(&filterModel);
    SessionController controller(&model, &filterModel, &selection);

    controller.setExternalEditorPath(m_stubPath);
    controller.openSelectedInEditor();

    QVERIFY(controller.hasError());
    QVERIFY(controller.errorMessage().contains(QStringLiteral("Select a single photo")));
}

void TestExternalTools::testControllerReportsMultipleSelection()
{
    QVERIFY(!m_stubPath.isEmpty());

    CatalogModel model;
    QList<CatalogItem> items;
    for (int i = 0; i < 2; ++i)
    {
        const QString path = writePhoto(QStringLiteral("multi-%1.jpg").arg(i));
        QVERIFY(!path.isEmpty());
        CatalogItem item;
        item.id = QStringLiteral("multi-%1").arg(i);
        item.filePath = path;
        item.fileName = QFileInfo(path).fileName();
        items.append(item);
    }
    model.addItems(items);

    CatalogFilterModel filterModel;
    filterModel.setSourceModel(&model);
    SelectionManager selection(&filterModel);
    QCOMPARE(filterModel.rowCount(), 2);
    selection.selectAll();
    QCOMPARE(selection.selectedCount(), 2);

    SessionController controller(&model, &filterModel, &selection);
    controller.setExternalEditorPath(m_stubPath);
    controller.openSelectedInEditor();

    QVERIFY(controller.hasError());
    QVERIFY(controller.errorMessage().contains(QStringLiteral("2 photos are selected")));
}

void TestExternalTools::testControllerRefusesDemoFixture()
{
    QVERIFY(!m_stubPath.isEmpty());

    CatalogItem demo;
    demo.id = QStringLiteral("demo");
    demo.filePath = QStringLiteral("qrc:/fixtures/demo.jpg");
    demo.fileName = QStringLiteral("demo.jpg");
    demo.isDemoFixture = true;

    CatalogModel model;
    model.addItems({demo});

    CatalogFilterModel filterModel;
    filterModel.setSourceModel(&model);
    SelectionManager selection(&filterModel);
    selection.select(0);

    SessionController controller(&model, &filterModel, &selection);
    controller.setExternalEditorPath(m_stubPath);
    controller.openSelectedInEditor();

    QVERIFY(controller.hasError());
    QVERIFY(controller.errorMessage().contains(QStringLiteral("demo photo")));
}

void TestExternalTools::testControllerOpensSelectedPhoto()
{
    QVERIFY(!m_stubPath.isEmpty());

    const QString photoPath = writePhoto(QStringLiteral("open-me.jpg"));
    QVERIFY(!photoPath.isEmpty());

    CatalogItem item;
    item.id = QStringLiteral("open-me");
    item.filePath = photoPath;
    item.fileName = QStringLiteral("open-me.jpg");

    CatalogModel model;
    model.addItems({item});

    CatalogFilterModel filterModel;
    filterModel.setSourceModel(&model);
    SelectionManager selection(&filterModel);
    selection.select(0);

    const QString outputPath = m_tempDir.filePath(QStringLiteral("stub-arguments.txt"));
    qputenv("PHOTOCARTA_TOOL_STUB_OUTPUT", outputPath.toUtf8());

    SessionController controller(&model, &filterModel, &selection);
    controller.setExternalEditorPath(m_stubPath);
    controller.openSelectedInEditor();

    QVERIFY2(!controller.hasError(), qPrintable(controller.errorMessage()));
    QVERIFY(controller.statusMessage().contains(QStringLiteral("Opened open-me.jpg")));

    const auto readOutput = [&outputPath]() {
        QFile output(outputPath);
        if (!output.open(QIODevice::ReadOnly))
        {
            return QString();
        }
        return QString::fromUtf8(output.readAll());
    };
    QTRY_COMPARE_WITH_TIMEOUT(readOutput(), QFileInfo(photoPath).absoluteFilePath(), 5000);
}

QTEST_MAIN(TestExternalTools)
#include "test_external_tools.moc"
