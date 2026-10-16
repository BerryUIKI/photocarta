// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogmodel.h"
#include "catalog/selectionmanager.h"
#include "catalog/sessioncontroller.h"
#include "catalog/thememanager.h"

#include <QSignalSpy>
#include <QTest>

using namespace PhotoCarta;

class TestThemeAndSession : public QObject
{
    Q_OBJECT

private slots:
    void testThemeManager();
    void testSessionController();
    void testErrorHandling();
    void testZoomFactorClamping();
};

void TestThemeAndSession::testThemeManager()
{
    ThemeManager theme(true);
    QCOMPARE(theme.isDark(), true);

    const QColor darkBg = theme.background();
    const QColor darkSurface = theme.surface();
    QVERIFY(darkBg.isValid());
    QVERIFY(darkSurface.isValid());

    QSignalSpy themeSpy(&theme, &ThemeManager::themeChanged);
    theme.toggleTheme();

    QCOMPARE(theme.isDark(), false);
    QCOMPARE(themeSpy.count(), 1);

    const QColor lightBg = theme.background();
    QVERIFY(lightBg.isValid());
    QVERIFY(darkBg != lightBg);

    // Spacing and radius constants
    QCOMPARE(ThemeManager::spacingSm(), 8);
    QCOMPARE(ThemeManager::radiusMd(), 6);
}

void TestThemeAndSession::testSessionController()
{
    CatalogModel model;
    CatalogFilterModel filter;
    filter.setSourceModel(&model);
    SelectionManager selection(&filter);
    SessionController controller(&model, &filter, &selection);

    QCOMPARE(controller.previewMode(), false);
    QCOMPARE(controller.leftPaneVisible(), true);
    QCOMPARE(controller.rightPaneVisible(), true);
    QCOMPARE(controller.isEphemeralSession(), true);

    // Toggle Preview Mode
    QSignalSpy previewSpy(&controller, &SessionController::previewModeChanged);
    controller.togglePreviewMode();
    QCOMPARE(controller.previewMode(), true);
    QCOMPARE(previewSpy.count(), 1);

    controller.setPreviewMode(false);
    QCOMPARE(controller.previewMode(), false);

    // Toggle Panes
    controller.toggleLeftPane();
    QCOMPARE(controller.leftPaneVisible(), false);

    controller.toggleRightPane();
    QCOMPARE(controller.rightPaneVisible(), false);

    // Load demo samples via controller
    controller.loadDemoSamples();
    QCOMPARE(model.count(), 6);
    QVERIFY(controller.sessionTypeDescription().contains(QStringLiteral("Demo Fixtures")));
}

void TestThemeAndSession::testErrorHandling()
{
    CatalogModel model;
    CatalogFilterModel filter;
    filter.setSourceModel(&model);
    SelectionManager selection(&filter);
    SessionController controller(&model, &filter, &selection);

    QCOMPARE(controller.hasError(), false);

    controller.showError(QStringLiteral("Sample error notification"));
    QCOMPARE(controller.hasError(), true);
    QCOMPARE(controller.errorMessage(), QStringLiteral("Sample error notification"));

    controller.dismissError();
    QCOMPARE(controller.hasError(), false);
    QVERIFY(controller.errorMessage().isEmpty());
}

void TestThemeAndSession::testZoomFactorClamping()
{
    CatalogModel model;
    CatalogFilterModel filter;
    SelectionManager selection(&filter);
    SessionController controller(&model, &filter, &selection);

    controller.setZoomFactor(1.5);
    QCOMPARE(controller.zoomFactor(), 1.5);

    // Test minimum clamping (0.6)
    controller.setZoomFactor(0.1);
    QCOMPARE(controller.zoomFactor(), 0.6);

    // Test maximum clamping (2.0)
    controller.setZoomFactor(5.0);
    QCOMPARE(controller.zoomFactor(), 2.0);
}

QTEST_MAIN(TestThemeAndSession)
#include "test_theme_and_session.moc"
