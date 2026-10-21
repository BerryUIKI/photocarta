// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDir>
#include <QFile>
#include <QTest>

// Accessibility and keyboard-focus coverage guard.
//
// QML accessibility annotations are attached properties and cannot be queried
// through the public Qt API without an active accessibility bridge, so this
// test inspects the declarative sources instead. It fails when an interactive
// component loses its accessible role, its tab stop, or its visible focus
// ring, which keeps the release check for keyboard navigation and screen
// reader support from silently regressing.

namespace
{
QString readQml(const QString &relativePath)
{
    QFile file(QStringLiteral(PHOTOCARTA_SOURCE_DIR) + QStringLiteral("/src/qml/") + relativePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

int countOccurrences(const QString &haystack, const QString &needle)
{
    int count = 0;
    int index = haystack.indexOf(needle);
    while (index >= 0)
    {
        ++count;
        index = haystack.indexOf(needle, index + needle.size());
    }
    return count;
}
} // namespace

class TestAccessibility : public QObject
{
    Q_OBJECT

private slots:
    void testInteractiveComponentsExposeRoles();
    void testInteractiveComponentsAreTabStops();
    void testDialogsAreKeyboardDismissable();
    void testFocusRingUsesThemeToken();
    void testGridIsAnAccessibleList();
};

void TestAccessibility::testInteractiveComponentsExposeRoles()
{
    struct Expectation
    {
        const char *file;
        int minimumRoles;
    };

    const Expectation expectations[] = {
        {"components/AppToolbar.qml", 14},
        {"components/CollectionNavigationPane.qml", 12},
        {"components/MetadataDetailsPane.qml", 20},
        {"components/EmptyStateView.qml", 3},
        {"components/ExportDialog.qml", 8},
        {"components/BatchRenameDialog.qml", 4},
        {"components/TrashConfirmDialog.qml", 3},
        {"components/ThumbnailCard.qml", 1},
        {"components/ThumbnailGridView.qml", 1},
        {"components/TagChip.qml", 1},
        {"components/RatingStars.qml", 1},
        {"components/ErrorBanner.qml", 1},
        {"components/AppStatusBar.qml", 1},
    };

    for (const Expectation &expectation : expectations)
    {
        const QString source = readQml(QString::fromUtf8(expectation.file));
        QVERIFY2(!source.isEmpty(), expectation.file);
        const int roles = countOccurrences(source, QStringLiteral("Accessible.role:"));
        QVERIFY2(roles >= expectation.minimumRoles,
                 qPrintable(QStringLiteral("%1 exposes %2 accessible roles, expected at least %3")
                                .arg(QString::fromUtf8(expectation.file))
                                .arg(roles)
                                .arg(expectation.minimumRoles)));
    }
}

void TestAccessibility::testInteractiveComponentsAreTabStops()
{
    struct Expectation
    {
        const char *file;
        int minimumTabStops;
    };

    const Expectation expectations[] = {
        {"components/AppToolbar.qml", 14},
        {"components/CollectionNavigationPane.qml", 12},
        {"components/MetadataDetailsPane.qml", 20},
        {"components/EmptyStateView.qml", 3},
        {"components/ExportDialog.qml", 7},
        {"components/BatchRenameDialog.qml", 3},
        {"components/TrashConfirmDialog.qml", 2},
        {"components/ThumbnailGridView.qml", 1},
        {"components/TagChip.qml", 1},
        {"components/RatingStars.qml", 1},
        {"components/ErrorBanner.qml", 1},
    };

    for (const Expectation &expectation : expectations)
    {
        const QString source = readQml(QString::fromUtf8(expectation.file));
        QVERIFY2(!source.isEmpty(), expectation.file);
        const int tabStops = countOccurrences(source, QStringLiteral("activeFocusOnTab:"));
        QVERIFY2(tabStops >= expectation.minimumTabStops,
                 qPrintable(QStringLiteral("%1 has %2 tab stops, expected at least %3")
                                .arg(QString::fromUtf8(expectation.file))
                                .arg(tabStops)
                                .arg(expectation.minimumTabStops)));
    }
}

void TestAccessibility::testDialogsAreKeyboardDismissable()
{
    const char *dialogFiles[] = {
        "components/TrashConfirmDialog.qml",
        "components/ExportDialog.qml",
        "components/BatchRenameDialog.qml",
    };

    for (const char *file : dialogFiles)
    {
        const QString source = readQml(QString::fromUtf8(file));
        QVERIFY2(!source.isEmpty(), file);
        QVERIFY2(source.contains(QStringLiteral("Accessible.role: Accessible.Dialog")), file);
        QVERIFY2(source.contains(QStringLiteral("Keys.onEscapePressed")), file);
        QVERIFY2(source.contains(QStringLiteral("forceActiveFocus()")), file);
    }
}

void TestAccessibility::testFocusRingUsesThemeToken()
{
    const QString source = readQml(QStringLiteral("components/FocusRing.qml"));
    QVERIFY(!source.isEmpty());
    QVERIFY(source.contains(QStringLiteral("themeManager.borderFocus")));
    QVERIFY(source.contains(QStringLiteral("parent.activeFocus")));
    QVERIFY(source.contains(QStringLiteral("Accessible.ignored: true")));
}

void TestAccessibility::testGridIsAnAccessibleList()
{
    const QString source = readQml(QStringLiteral("components/ThumbnailGridView.qml"));
    QVERIFY(!source.isEmpty());
    QVERIFY(source.contains(QStringLiteral("Accessible.role: Accessible.List")));
    QVERIFY(source.contains(QStringLiteral("Keys.onRightPressed")));
    QVERIFY(source.contains(QStringLiteral("Keys.onLeftPressed")));

    const QString card = readQml(QStringLiteral("components/ThumbnailCard.qml"));
    QVERIFY(!card.isEmpty());
    QVERIFY(card.contains(QStringLiteral("Accessible.role: Accessible.ListItem")));
    QVERIFY(card.contains(QStringLiteral("Accessible.selected: root.isSelected")));
}

QTEST_MAIN(TestAccessibility)
#include "test_accessibility.moc"
