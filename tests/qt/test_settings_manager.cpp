// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/settings/settingsmanager.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

class TestSettingsManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testDefaultValues();
    void testPersistenceAndReload();
    void testSignalsEmitted();
    void testZoomFactorClamping();
    void testWindowGeometrySaving();
    void testExternalEditorPath();
    void testResetToDefaults();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_iniPath;
};

void TestSettingsManager::initTestCase()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    m_iniPath = m_tempDir->filePath(QStringLiteral("test_photocarta.ini"));
}

void TestSettingsManager::cleanupTestCase()
{
    m_tempDir.reset();
}

void TestSettingsManager::testDefaultValues()
{
    const QString freshIni = m_tempDir->filePath(QStringLiteral("defaults.ini"));
    SettingsManager settings(freshIni);

    QCOMPARE(settings.theme(), QStringLiteral("dark"));
    QCOMPARE(settings.leftPaneVisible(), true);
    QCOMPARE(settings.rightPaneVisible(), true);
    QCOMPARE(settings.gridZoomFactor(), 1.0);
    QCOMPARE(settings.sortCriterion(), QStringLiteral("date"));
    QCOMPARE(settings.sortAscending(), false);
    QCOMPARE(settings.lastDatabasePath(), QString());
    QCOMPARE(settings.lastCollectionId(), 0);
    QCOMPARE(settings.windowWidth(), 1200);
    QCOMPARE(settings.windowHeight(), 800);
    QCOMPARE(settings.windowMaximized(), false);
    QCOMPARE(settings.filmstripVisible(), true);
    QCOMPARE(settings.externalEditorPath(), QString());
}

void TestSettingsManager::testPersistenceAndReload()
{
    const QString iniPath = m_tempDir->filePath(QStringLiteral("persist.ini"));

    {
        SettingsManager settings(iniPath);
        settings.setTheme(QStringLiteral("light"));
        settings.setLeftPaneVisible(false);
        settings.setRightPaneVisible(false);
        settings.setGridZoomFactor(1.4);
        settings.setSortCriterion(QStringLiteral("rating"));
        settings.setSortAscending(true);
        settings.setLastDatabasePath(QStringLiteral("/path/to/my_catalog.db"));
        settings.setLastCollectionId(42);
        settings.setFilmstripVisible(false);
        settings.saveWindowGeometry(150, 200, 1440, 900, true);
        settings.sync();
    }

    // Reload from the same file in a brand-new instance
    {
        SettingsManager reloaded(iniPath);
        QCOMPARE(reloaded.theme(), QStringLiteral("light"));
        QCOMPARE(reloaded.leftPaneVisible(), false);
        QCOMPARE(reloaded.rightPaneVisible(), false);
        QCOMPARE(reloaded.gridZoomFactor(), 1.4);
        QCOMPARE(reloaded.sortCriterion(), QStringLiteral("rating"));
        QCOMPARE(reloaded.sortAscending(), true);
        QCOMPARE(reloaded.lastDatabasePath(), QStringLiteral("/path/to/my_catalog.db"));
        QCOMPARE(reloaded.lastCollectionId(), 42);
        QCOMPARE(reloaded.filmstripVisible(), false);
        QCOMPARE(reloaded.windowX(), 150);
        QCOMPARE(reloaded.windowY(), 200);
        QCOMPARE(reloaded.windowWidth(), 1440);
        QCOMPARE(reloaded.windowHeight(), 900);
        QCOMPARE(reloaded.windowMaximized(), true);
    }
}

void TestSettingsManager::testSignalsEmitted()
{
    const QString iniPath = m_tempDir->filePath(QStringLiteral("signals.ini"));
    SettingsManager settings(iniPath);

    QSignalSpy spyTheme(&settings, &SettingsManager::themeChanged);
    QSignalSpy spyLeft(&settings, &SettingsManager::leftPaneVisibleChanged);
    QSignalSpy spyRight(&settings, &SettingsManager::rightPaneVisibleChanged);
    QSignalSpy spyZoom(&settings, &SettingsManager::gridZoomFactorChanged);
    QSignalSpy spySort(&settings, &SettingsManager::sortCriterionChanged);
    QSignalSpy spyAsc(&settings, &SettingsManager::sortAscendingChanged);

    settings.setTheme(QStringLiteral("light"));
    QCOMPARE(spyTheme.count(), 1);
    QCOMPARE(spyTheme.takeFirst().at(0).toString(), QStringLiteral("light"));

    // Setting same value should not emit duplicate signal
    settings.setTheme(QStringLiteral("light"));
    QCOMPARE(spyTheme.count(), 0);

    settings.setLeftPaneVisible(false);
    QCOMPARE(spyLeft.count(), 1);
    QCOMPARE(spyLeft.takeFirst().at(0).toBool(), false);

    settings.setRightPaneVisible(false);
    QCOMPARE(spyRight.count(), 1);

    settings.setGridZoomFactor(1.5);
    QCOMPARE(spyZoom.count(), 1);

    settings.setSortCriterion(QStringLiteral("name"));
    QCOMPARE(spySort.count(), 1);

    settings.setSortAscending(true);
    QCOMPARE(spyAsc.count(), 1);
}

void TestSettingsManager::testZoomFactorClamping()
{
    const QString iniPath = m_tempDir->filePath(QStringLiteral("clamp.ini"));
    SettingsManager settings(iniPath);

    settings.setGridZoomFactor(0.1); // Below minimum 0.5
    QCOMPARE(settings.gridZoomFactor(), 0.5);

    settings.setGridZoomFactor(5.0); // Above maximum 2.5
    QCOMPARE(settings.gridZoomFactor(), 2.5);

    settings.setGridZoomFactor(1.25);
    QCOMPARE(settings.gridZoomFactor(), 1.25);
}

void TestSettingsManager::testWindowGeometrySaving()
{
    const QString iniPath = m_tempDir->filePath(QStringLiteral("geom.ini"));
    SettingsManager settings(iniPath);

    QSignalSpy spyGeom(&settings, &SettingsManager::windowGeometryChanged);
    settings.saveWindowGeometry(50, 80, 1600, 1000, false);

    QCOMPARE(spyGeom.count(), 1);
    QCOMPARE(settings.windowX(), 50);
    QCOMPARE(settings.windowY(), 80);
    QCOMPARE(settings.windowWidth(), 1600);
    QCOMPARE(settings.windowHeight(), 1000);
    QCOMPARE(settings.windowMaximized(), false);
}

void TestSettingsManager::testExternalEditorPath()
{
    const QString iniPath = m_tempDir->filePath(QStringLiteral("editor.ini"));
    SettingsManager settings(iniPath);
    QCOMPARE(settings.externalEditorPath(), QString());

    QSignalSpy spy(&settings, &SettingsManager::externalEditorPathChanged);
    settings.setExternalEditorPath(QStringLiteral("C:/tools/showfoto.exe"));
    QCOMPARE(settings.externalEditorPath(), QStringLiteral("C:/tools/showfoto.exe"));
    QCOMPARE(spy.count(), 1);

    // Setting the same value again does not emit another change.
    settings.setExternalEditorPath(QStringLiteral("C:/tools/showfoto.exe"));
    QCOMPARE(spy.count(), 1);

    settings.sync();
    SettingsManager reloaded(iniPath);
    QCOMPARE(reloaded.externalEditorPath(), QStringLiteral("C:/tools/showfoto.exe"));

    reloaded.resetToDefaults();
    QCOMPARE(reloaded.externalEditorPath(), QString());
}

void TestSettingsManager::testResetToDefaults()
{
    const QString iniPath = m_tempDir->filePath(QStringLiteral("reset.ini"));
    SettingsManager settings(iniPath);

    settings.setTheme(QStringLiteral("light"));
    settings.setLeftPaneVisible(false);
    settings.setGridZoomFactor(2.0);
    settings.setSortCriterion(QStringLiteral("size"));
    settings.saveWindowGeometry(20, 20, 800, 600, true);
    settings.setExternalEditorPath(QStringLiteral("C:/tools/digikam.exe"));

    settings.resetToDefaults();

    QCOMPARE(settings.theme(), QStringLiteral("dark"));
    QCOMPARE(settings.leftPaneVisible(), true);
    QCOMPARE(settings.rightPaneVisible(), true);
    QCOMPARE(settings.gridZoomFactor(), 1.0);
    QCOMPARE(settings.sortCriterion(), QStringLiteral("date"));
    QCOMPARE(settings.sortAscending(), false);
    QCOMPARE(settings.windowWidth(), 1200);
    QCOMPARE(settings.windowHeight(), 800);
    QCOMPARE(settings.windowMaximized(), false);
    QCOMPARE(settings.externalEditorPath(), QString());
}

QTEST_MAIN(TestSettingsManager)
#include "test_settings_manager.moc"
