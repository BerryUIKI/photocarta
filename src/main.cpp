// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogmodel.h"
#include "catalog/selectionmanager.h"
#include "catalog/sessioncontroller.h"
#include "catalog/thememanager.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

using namespace PhotoCarta;

int main(int argc, char *argv[])
{
    // Use QApplication (Qt Widgets) to preserve compatibility for later digiKam tool windows
    QApplication app(argc, argv);
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    app.setApplicationName(QStringLiteral("PhotoCarta"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setOrganizationName(QStringLiteral("PhotoCarta"));
    app.setOrganizationDomain(QStringLiteral("photocarta.org"));
    app.setWindowIcon(QIcon(QStringLiteral(":/assets/icons/app_icon.svg")));

    // Command-line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("PhotoCarta Desktop Photo Management"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption screenshotOption(
        QStringList() << QStringLiteral("s") << QStringLiteral("screenshot"),
        QStringLiteral("Capture window screenshot and save to <path>, then exit."),
        QStringLiteral("path"));
    parser.addOption(screenshotOption);

    QCommandLineOption smokeTestOption(
        QStringLiteral("smoke-test"),
        QStringLiteral("Perform automated startup smoke test and exit."));
    parser.addOption(smokeTestOption);

    QCommandLineOption themeOption(
        QStringLiteral("theme"),
        QStringLiteral("Set initial theme ('dark' or 'light')."),
        QStringLiteral("theme"),
        QStringLiteral("dark"));
    parser.addOption(themeOption);

    QCommandLineOption previewOption(
        QStringLiteral("preview"),
        QStringLiteral("Launch directly into single-image preview mode."));
    parser.addOption(previewOption);

    QCommandLineOption widthOption(
        QStringList() << QStringLiteral("width"),
        QStringLiteral("Set initial window width in pixels."),
        QStringLiteral("pixels"));
    parser.addOption(widthOption);

    QCommandLineOption heightOption(
        QStringList() << QStringLiteral("height"),
        QStringLiteral("Set initial window height in pixels."),
        QStringLiteral("pixels"));
    parser.addOption(heightOption);

    parser.addPositionalArgument(QStringLiteral("files"),
                                QStringLiteral("Initial image files to load into session."),
                                QStringLiteral("[files...]"));

    parser.process(app);

    const bool isSmokeTest = parser.isSet(smokeTestOption);
    const QString screenshotPath = parser.value(screenshotOption);
    const bool isDarkTheme = (parser.value(themeOption).toLower() != QStringLiteral("light"));
    const bool startInPreview = parser.isSet(previewOption);
    const int customWidth = parser.value(widthOption).toInt();
    const int customHeight = parser.value(heightOption).toInt();
    const QStringList positionalFiles = parser.positionalArguments();

    // Initialize C++ Presentation Models and Controllers
    auto *catalogModel = new CatalogModel(&app);
    auto *filterModel = new CatalogFilterModel(&app);
    filterModel->setSourceModel(catalogModel);

    auto *selectionManager = new SelectionManager(filterModel, &app);
    auto *themeManager = new ThemeManager(isDarkTheme, &app);
    auto *sessionController = new SessionController(catalogModel, filterModel, selectionManager, &app);

    if (startInPreview)
    {
        sessionController->setPreviewMode(true);
    }

    // Populate session: load given files if any, otherwise load procedural demo fixtures
    if (!positionalFiles.isEmpty())
    {
        catalogModel->addFilePaths(positionalFiles);
    }
    else
    {
        catalogModel->loadDemoFixtures();
    }

    if (filterModel->count() > 0)
    {
        selectionManager->select(0);
    }

    // QQmlApplicationEngine setup
    QQmlApplicationEngine engine;

    // Register context properties
    QQmlContext *rootCtx = engine.rootContext();
    rootCtx->setContextProperty(QStringLiteral("catalogModel"), catalogModel);
    rootCtx->setContextProperty(QStringLiteral("filterModel"), filterModel);
    rootCtx->setContextProperty(QStringLiteral("selectionManager"), selectionManager);
    rootCtx->setContextProperty(QStringLiteral("themeManager"), themeManager);
    rootCtx->setContextProperty(QStringLiteral("sessionController"), sessionController);

    // QML Import Paths
    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.addImportPath(QStringLiteral("qrc:/photocarta/qml"));

    const QUrl url(QStringLiteral("qrc:/photocarta/qml/main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl)
        {
            if (!obj && url == objUrl)
            {
                qCritical() << "Fatal: Failed to load QML root object from" << objUrl;
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty())
    {
        qCritical() << "Fatal: No root objects loaded in QML engine.";
        return 1;
    }

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window)
    {
        qCritical() << "Fatal: Root object is not a QQuickWindow.";
        return 1;
    }

    if (customWidth > 0)
    {
        window->setWidth(customWidth);
    }
    if (customHeight > 0)
    {
        window->setHeight(customHeight);
    }

    // Handle automated startup smoke test
    if (isSmokeTest)
    {
        qInfo() << "=== PhotoCarta Startup Smoke Test ===";
        qInfo() << "Engine loaded successfully.";
        qInfo() << "Catalog items count:" << catalogModel->count();
        qInfo() << "Filter model count:" << filterModel->count();
        qInfo() << "Selected index:" << selectionManager->currentIndex();
        qInfo() << "Theme mode (isDark):" << themeManager->isDark();

        if (catalogModel->count() <= 0)
        {
            qCritical() << "Smoke test failed: CatalogModel has 0 items.";
            return 2;
        }

        if (selectionManager->currentIndex() < 0)
        {
            qCritical() << "Smoke test failed: SelectionManager has no valid selection.";
            return 3;
        }

        qInfo() << "Smoke test passed successfully. Exiting.";
        QTimer::singleShot(50, &app, [&app]() { app.quit(); });
        return app.exec();
    }

    // Handle screenshot capture CLI option
    if (!screenshotPath.isEmpty())
    {
        qInfo() << "Capturing screenshot to" << screenshotPath;
        window->show();

        QTimer::singleShot(250, &app, [window, screenshotPath, &app]() {
            app.processEvents();
            const QImage image = window->grabWindow();
            if (image.isNull())
            {
                qCritical() << "Failed to grab window image.";
                app.exit(1);
                return;
            }

            QFileInfo fi(screenshotPath);
            QDir().mkpath(fi.absolutePath());

            if (image.save(screenshotPath))
            {
                qInfo() << "Screenshot successfully saved to" << screenshotPath;
                app.exit(0);
            }
            else
            {
                qCritical() << "Failed to save screenshot image to" << screenshotPath;
                app.exit(1);
            }
        });

        return app.exec();
    }

    return app.exec();
}
