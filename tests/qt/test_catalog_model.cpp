// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "catalog/catalogitem.h"
#include "catalog/catalogmodel.h"

#include <QSignalSpy>
#include <QTest>

using namespace PhotoCarta;

class TestCatalogModel : public QObject
{
    Q_OBJECT

private slots:
    void testFormatBytes();
    void testDemoFixturesLoaded();
    void testModelRoles();
    void testGetVariantMap();
    void testClearSession();
    void testFileLoadError();
    void testAvailableTags();
    void testAddAndRemoveTag();
    void testBatchOperations();
};

void TestCatalogModel::testFormatBytes()
{
    QCOMPARE(CatalogItem::formatBytes(0), QStringLiteral("0 B"));
    QCOMPARE(CatalogItem::formatBytes(-10), QStringLiteral("0 B"));
    QCOMPARE(CatalogItem::formatBytes(500), QStringLiteral("500 B"));
    QCOMPARE(CatalogItem::formatBytes(2048), QStringLiteral("2.0 KB"));
    QCOMPARE(CatalogItem::formatBytes(2048 * 1024), QStringLiteral("2.0 MB"));
}

void TestCatalogModel::testDemoFixturesLoaded()
{
    CatalogModel model;
    QSignalSpy countSpy(&model, &CatalogModel::countChanged);
    QSignalSpy sessionSpy(&model, &CatalogModel::sessionLoaded);

    QCOMPARE(model.count(), 0);
    QCOMPARE(model.hasDemoContent(), false);

    model.loadDemoFixtures();

    QCOMPARE(model.count(), 6);
    QCOMPARE(model.hasDemoContent(), true);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(sessionSpy.count(), 1);
    QCOMPARE(sessionSpy.takeFirst().at(0).toInt(), 6);

    const CatalogItem *first = model.itemAt(0);
    QVERIFY(first != nullptr);
    QCOMPARE(first->id, QStringLiteral("demo-001"));
    QCOMPARE(first->isDemoFixture, true);
    QCOMPARE(first->rating, 5);
    QVERIFY(first->dimensions.width() > 0);
    QVERIFY(first->dimensions.height() > 0);
    QVERIFY(!first->cameraModel.isEmpty());
    QVERIFY(first->tags.contains(QStringLiteral("Landscape")));

    const CatalogItem *last = model.itemAt(5);
    QVERIFY(last != nullptr);
    QCOMPARE(last->id, QStringLiteral("demo-006"));
    QCOMPARE(last->isDemoFixture, true);
    QVERIFY(last->tags.contains(QStringLiteral("Astro")));
}

void TestCatalogModel::testModelRoles()
{
    CatalogModel model;
    model.loadDemoFixtures();

    const QModelIndex firstIndex = model.index(0, 0);
    QVERIFY(firstIndex.isValid());

    const QString title = model.data(firstIndex, CatalogModel::TitleRole).toString();
    QCOMPARE(title, QStringLiteral("Alpine Sunrise at Lake Oeschinen"));

    const QString camera = model.data(firstIndex, CatalogModel::CameraModelRole).toString();
    QCOMPARE(camera, QStringLiteral("Sony Alpha 7 IV"));

    const int rating = model.data(firstIndex, CatalogModel::RatingRole).toInt();
    QCOMPARE(rating, 5);

    const bool isDemo = model.data(firstIndex, CatalogModel::IsDemoFixtureRole).toBool();
    QCOMPARE(isDemo, true);

    const auto roles = model.roleNames();
    QVERIFY(roles.contains(CatalogModel::IdRole));
    QVERIFY(roles.contains(CatalogModel::FileNameRole));
    QVERIFY(roles.contains(CatalogModel::TitleRole));
    QVERIFY(roles.contains(CatalogModel::DimensionsRole));
    QVERIFY(roles.contains(CatalogModel::CameraModelRole));
    QVERIFY(roles.contains(CatalogModel::RatingRole));
    QVERIFY(roles.contains(CatalogModel::TagsRole));
}

void TestCatalogModel::testGetVariantMap()
{
    CatalogModel model;
    model.loadDemoFixtures();

    const QVariantMap map = model.get(0);
    QVERIFY(!map.isEmpty());
    QCOMPARE(map.value(QStringLiteral("id")).toString(), QStringLiteral("demo-001"));
    QCOMPARE(map.value(QStringLiteral("cameraModel")).toString(), QStringLiteral("Sony Alpha 7 IV"));
    QCOMPARE(map.value(QStringLiteral("isDemoFixture")).toBool(), true);

    const QVariantMap invalidMap = model.get(999);
    QVERIFY(invalidMap.isEmpty());
}

void TestCatalogModel::testClearSession()
{
    CatalogModel model;
    model.loadDemoFixtures();
    QCOMPARE(model.count(), 6);

    QSignalSpy countSpy(&model, &CatalogModel::countChanged);
    model.clearSession();

    QCOMPARE(model.count(), 0);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(model.hasDemoContent(), false);
}

void TestCatalogModel::testFileLoadError()
{
    CatalogModel model;
    QSignalSpy errorSpy(&model, &CatalogModel::fileLoadError);

    model.addFilePaths({QStringLiteral("non_existent_photo_12345.jpg")});

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(model.count(), 0);
}

void TestCatalogModel::testAvailableTags()
{
    CatalogModel model;
    QSignalSpy tagsSpy(&model, &CatalogModel::availableTagsChanged);

    QCOMPARE(model.availableTags().size(), 0);

    model.loadDemoFixtures();
    QCOMPARE(tagsSpy.count(), 1);

    const QStringList tags = model.availableTags();
    QVERIFY(tags.contains(QStringLiteral("Landscape")));
    QVERIFY(tags.contains(QStringLiteral("Portrait")));
    QVERIFY(tags.contains(QStringLiteral("Architecture")));
    QVERIFY(tags.contains(QStringLiteral("Street")));
    QVERIFY(tags.contains(QStringLiteral("Macro")));
    QVERIFY(tags.contains(QStringLiteral("Astro")));
}

void TestCatalogModel::testAddAndRemoveTag()
{
    CatalogModel model;
    model.loadDemoFixtures();

    QSignalSpy tagsSpy(&model, &CatalogModel::availableTagsChanged);

    const CatalogItem *item0 = model.itemAt(0);
    QVERIFY(item0 != nullptr);
    QVERIFY(!item0->tags.contains(QStringLiteral("Travel")));

    // Add tag
    model.addTagToItem(0, QStringLiteral("Travel"));
    QVERIFY(model.itemAt(0)->tags.contains(QStringLiteral("Travel")));
    QVERIFY(model.availableTags().contains(QStringLiteral("Travel")));
    QCOMPARE(tagsSpy.count(), 1);

    // Duplicate add should be ignored (case-insensitive)
    model.addTagToItem(0, QStringLiteral("travel"));
    QCOMPARE(model.itemAt(0)->tags.count(QStringLiteral("Travel")), 1);
    QCOMPARE(tagsSpy.count(), 1);

    // Empty or whitespace tag should be ignored
    model.addTagToItem(0, QStringLiteral("   "));
    QCOMPARE(tagsSpy.count(), 1);

    // Remove tag
    model.removeTagFromItem(0, QStringLiteral("Travel"));
    QVERIFY(!model.itemAt(0)->tags.contains(QStringLiteral("Travel")));
    QVERIFY(!model.availableTags().contains(QStringLiteral("Travel")));
    QCOMPARE(tagsSpy.count(), 2);

    // Out-of-bounds safety
    model.addTagToItem(-1, QStringLiteral("Invalid"));
    model.removeTagFromItem(999, QStringLiteral("Invalid"));
    QCOMPARE(tagsSpy.count(), 2);
}

void TestCatalogModel::testBatchOperations()
{
    CatalogModel model;
    model.loadDemoFixtures();

    const QList<int> batch = {0, 1, 2};

    // Batch Rating
    model.updateItemsRating(batch, 1);
    QCOMPARE(model.itemAt(0)->rating, 1);
    QCOMPARE(model.itemAt(1)->rating, 1);
    QCOMPARE(model.itemAt(2)->rating, 1);

    // Batch Flagging
    model.updateItemsFlagged(batch, true);
    QCOMPARE(model.itemAt(0)->isFlagged, true);
    QCOMPARE(model.itemAt(1)->isFlagged, true);
    QCOMPARE(model.itemAt(2)->isFlagged, true);

    // Batch Tag Addition
    model.addTagToItems(batch, QStringLiteral("BatchTag"));
    QVERIFY(model.itemAt(0)->tags.contains(QStringLiteral("BatchTag")));
    QVERIFY(model.itemAt(1)->tags.contains(QStringLiteral("BatchTag")));
    QVERIFY(model.itemAt(2)->tags.contains(QStringLiteral("BatchTag")));

    // Batch Tag Removal
    model.removeTagFromItems(batch, QStringLiteral("BatchTag"));
    QVERIFY(!model.itemAt(0)->tags.contains(QStringLiteral("BatchTag")));
    QVERIFY(!model.itemAt(1)->tags.contains(QStringLiteral("BatchTag")));
    QVERIFY(!model.itemAt(2)->tags.contains(QStringLiteral("BatchTag")));
}

QTEST_MAIN(TestCatalogModel)
#include "test_catalog_model.moc"
