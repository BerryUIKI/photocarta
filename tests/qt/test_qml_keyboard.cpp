// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/thememanager.h"

#include <QFile>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace PhotoCarta;

// Behavioral keyboard-access checks for the QML components.
//
// The accessibility annotations themselves are static (see test_accessibility);
// this test loads real components into an offscreen window and verifies the
// keyboard contract: Tab moves focus between custom controls, Space activates
// the focused control, Delete removes a tag, and the focus ring follows focus.

class TestQmlKeyboard : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testTabMovesFocusAndSpaceActivates();
    void testDeleteRequestsTagRemoval();
    void testFocusRingFollowsActiveFocus();

private:
    QTemporaryDir m_tempDir;
    QQuickView *m_view = nullptr;
    QQuickItem *m_root = nullptr;
    ThemeManager *m_theme = nullptr;
};

void TestQmlKeyboard::init()
{
    QVERIFY(m_tempDir.isValid());

    const QByteArray harness = R"QML(
import QtQuick
import "qrc:/photocarta/qml/components"

Item {
    width: 400
    height: 200

    Rectangle {
        objectName: "button"
        width: 100
        height: 32
        activeFocusOnTab: true

        FocusRing {
            objectName: "ring"
            targetRadius: 4
        }
    }

    TagChip {
        objectName: "chip"
        y: 40
        text: "one"
    }

    TagChip {
        objectName: "removableChip"
        y: 80
        text: "two"
        removable: true
    }
}
)QML";

    const QString path = m_tempDir.filePath(QStringLiteral("harness.qml"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(harness), qint64(harness.size()));
    file.close();

    m_view = new QQuickView;
    m_view->engine()->addImportPath(QStringLiteral("qrc:/"));
    m_view->engine()->addImportPath(QStringLiteral("qrc:/photocarta/qml"));

    m_theme = new ThemeManager(true, m_view);
    m_view->rootContext()->setContextProperty(QStringLiteral("themeManager"), m_theme);

    m_view->setResizeMode(QQuickView::SizeRootObjectToView);
    m_view->setSource(QUrl::fromLocalFile(path));
    QCOMPARE(m_view->status(), QQuickView::Ready);
    QVERIFY(m_view->rootObject());
    m_root = qobject_cast<QQuickItem *>(m_view->rootObject());
    QVERIFY(m_root);

    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view));
    m_view->requestActivate();
}

void TestQmlKeyboard::cleanup()
{
    delete m_view;
    m_view = nullptr;
    m_root = nullptr;
    m_theme = nullptr;
}

void TestQmlKeyboard::testTabMovesFocusAndSpaceActivates()
{
    auto *button = m_root->findChild<QQuickItem *>(QStringLiteral("button"));
    auto *chip = m_root->findChild<QQuickItem *>(QStringLiteral("chip"));
    QVERIFY(button);
    QVERIFY(chip);

    button->forceActiveFocus();
    QTRY_VERIFY(button->hasActiveFocus());

    QSignalSpy clicked(chip, SIGNAL(clicked()));

    QTest::keyClick(m_view, Qt::Key_Tab);
    QTRY_VERIFY(chip->hasActiveFocus());

    QTest::keyClick(m_view, Qt::Key_Space);
    QCOMPARE(clicked.count(), 1);
}

void TestQmlKeyboard::testDeleteRequestsTagRemoval()
{
    auto *removableChip = m_root->findChild<QQuickItem *>(QStringLiteral("removableChip"));
    QVERIFY(removableChip);

    QSignalSpy removal(removableChip, SIGNAL(removeRequested()));

    removableChip->forceActiveFocus();
    QTRY_VERIFY(removableChip->hasActiveFocus());

    QTest::keyClick(m_view, Qt::Key_Delete);
    QCOMPARE(removal.count(), 1);
}

void TestQmlKeyboard::testFocusRingFollowsActiveFocus()
{
    auto *button = m_root->findChild<QQuickItem *>(QStringLiteral("button"));
    auto *ring = m_root->findChild<QQuickItem *>(QStringLiteral("ring"));
    QVERIFY(button);
    QVERIFY(ring);

    QVERIFY(!ring->property("visible").toBool());

    button->forceActiveFocus();
    QTRY_VERIFY(button->hasActiveFocus());
    QTRY_VERIFY(ring->property("visible").toBool());
}

QTEST_MAIN(TestQmlKeyboard)
#include "test_qml_keyboard.moc"
