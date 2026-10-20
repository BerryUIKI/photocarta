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
    void testSelectionMetadataEditing();
    void testErrorHandling();
    void testZoomFactorClamping();
    void testViewerZoomAndRotation();
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

void TestThemeAndSession::testSelectionMetadataEditing()
{
    CatalogModel model;
    CatalogFilterModel filter;
    filter.setSourceModel(&model);
    SelectionManager selection(&filter);
    SessionController controller(&model, &filter, &selection);

    model.loadDemoFixtures();

    selection.selectById(QStringLiteral("demo-001"));
    QCOMPARE(selection.selectedCount(), 1);
    QCOMPARE(selection.currentId(), QStringLiteral("demo-001"));
    controller.setSelectionRating(2);
    QCOMPARE(model.itemAt(0)->rating, 2);

    selection.toggleSelectById(QStringLiteral("demo-002"));
    QCOMPARE(selection.selectedCount(), 2);

    controller.addTagToSelection(QStringLiteral("Shared"));
    QVERIFY(model.itemAt(0)->tags.contains(QStringLiteral("Shared")));
    QVERIFY(model.itemAt(1)->tags.contains(QStringLiteral("Shared")));

    controller.removeTagFromSelection(QStringLiteral("shared"));
    QVERIFY(!model.itemAt(0)->tags.contains(QStringLiteral("Shared")));
    QVERIFY(!model.itemAt(1)->tags.contains(QStringLiteral("Shared")));

    controller.setSelectionFlagged(true);
    QVERIFY(model.itemAt(0)->isFlagged);
    QVERIFY(model.itemAt(1)->isFlagged);

    // Selection targeting survives a filter that hides the selected rows
    filter.setSearchQuery(QStringLiteral("Macro"));
    QCOMPARE(selection.selectedCount(), 2);
    controller.addTagToSelection(QStringLiteral("Hidden"));
    QVERIFY(model.itemAt(0)->tags.contains(QStringLiteral("Hidden")));
    QVERIFY(model.itemAt(1)->tags.contains(QStringLiteral("Hidden")));
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

void TestThemeAndSession::testViewerZoomAndRotation()
{
    CatalogModel model;
    CatalogFilterModel filter;
    filter.setSourceModel(&model);
    SelectionManager selection(&filter);
    SessionController controller(&model, &filter, &selection);

    // Initial state
    QCOMPARE(controller.viewerZoom(), 1.0);
    QCOMPARE(controller.viewerRotation(), 0);
    QCOMPARE(controller.isFullscreen(), false);

    // Zoom tests
    QSignalSpy zoomSpy(&controller, &SessionController::viewerZoomChanged);
    controller.zoomIn(0.5);
    QCOMPARE(controller.viewerZoom(), 1.5);
    QCOMPARE(zoomSpy.count(), 1);

    controller.zoomOut(0.25);
    QCOMPARE(controller.viewerZoom(), 1.25);

    // Clamping min / max
    controller.setViewerZoom(0.01);
    QCOMPARE(controller.viewerZoom(), 0.1);

    controller.setViewerZoom(50.0);
    QCOMPARE(controller.viewerZoom(), 10.0);

    controller.resetViewerZoom();
    QCOMPARE(controller.viewerZoom(), 1.0);

    // Rotation tests
    QSignalSpy rotSpy(&controller, &SessionController::viewerRotationChanged);
    controller.rotateClockwise();
    QCOMPARE(controller.viewerRotation(), 90);
    QCOMPARE(rotSpy.count(), 1);

    controller.rotateClockwise();
    QCOMPARE(controller.viewerRotation(), 180);

    controller.rotateClockwise();
    QCOMPARE(controller.viewerRotation(), 270);

    controller.rotateClockwise();
    QCOMPARE(controller.viewerRotation(), 0);

    controller.rotateCounterClockwise();
    QCOMPARE(controller.viewerRotation(), 270);

    controller.resetViewerRotation();
    QCOMPARE(controller.viewerRotation(), 0);

    // Fullscreen toggle
    QSignalSpy fsSpy(&controller, &SessionController::fullscreenChanged);
    controller.toggleFullscreen();
    QCOMPARE(controller.isFullscreen(), true);
    QCOMPARE(fsSpy.count(), 1);

    controller.toggleFullscreen();
    QCOMPARE(controller.isFullscreen(), false);
    QCOMPARE(fsSpy.count(), 2);

    // Changing selection resets zoom and rotation
    controller.zoomIn(1.0);
    controller.rotateClockwise();
    QCOMPARE(controller.viewerZoom(), 2.0);
    QCOMPARE(controller.viewerRotation(), 90);

    controller.loadDemoSamples();
    selection.select(1);
    QCOMPARE(controller.viewerZoom(), 1.0);
    QCOMPARE(controller.viewerRotation(), 0);
}

QTEST_MAIN(TestThemeAndSession)
#include "test_theme_and_session.moc"
