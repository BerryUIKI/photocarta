// SPDX-FileCopyrightText: 2026 PhotoCarta contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * Scale benchmark for the catalog and thumbnail pipeline.
 *
 * Measures the four targets recorded in docs/mvp.md against a real photo
 * directory:
 *
 *   - catalog open to an interactive grid
 *   - search and filter update latency (95th percentile)
 *   - thumbnail throughput
 *   - longest event-loop stall while thumbnails are generated
 *
 * plus resident memory. The catalog is built by scanning the media directory
 * through the same DirectoryScanner and CatalogDatabase path the application
 * uses, so the measurement covers real ingestion rather than a hand-written
 * schema. Results are printed as JSON and, optionally, as a Markdown table row
 * for docs/benchmarks.md.
 *
 * Usage:
 *   photocarta_benchmark --media-dir <dir> --work-dir <dir> [options]
 *
 * Exit codes: 0 on a completed run, 2 on a usage or input error. A completed
 * run exits 0 even when a target is missed; the report says which targets
 * failed. Reporting is the benchmark's job, failing a build is not.
 */

#include "catalog/catalogfiltermodel.h"
#include "catalog/catalogmodel.h"
#include "catalog/database/catalogdatabase.h"
#include "catalog/ingestion/directoryscanner.h"
#include "catalog/thumbnails/thumbnailcache.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#elif defined(Q_OS_LINUX)
#include <QFile>
#endif

using namespace PhotoCarta;

namespace
{

// Targets from docs/mvp.md, on the reference machine.
constexpr double OPEN_TARGET_MS = 3000.0;
constexpr double FILTER_P95_TARGET_MS = 400.0;
constexpr double THUMBNAIL_TARGET_PER_SECOND = 40.0;
constexpr double STALL_TARGET_MS = 100.0;
constexpr double MEMORY_TARGET_MB = 1536.0;

constexpr int DEFAULT_THUMBNAIL_COUNT = 500;
constexpr int EVENT_LOOP_POLL_MS = 5;

/// Resident set size in bytes, or -1 when unavailable on this platform.
qint64 residentBytes()
{
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS counters;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
    {
        return static_cast<qint64>(counters.WorkingSetSize);
    }
    return -1;
#elif defined(Q_OS_LINUX)
    QFile status(QStringLiteral("/proc/self/status"));
    if (!status.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return -1;
    }
    while (!status.atEnd())
    {
        const QByteArray line = status.readLine();
        if (line.startsWith("VmRSS:"))
        {
            const QList<QByteArray> fields = line.simplified().split(' ');
            if (fields.size() >= 2)
            {
                return fields.at(1).toLongLong() * 1024;
            }
        }
    }
    return -1;
#else
    return -1;
#endif
}

/// Peak resident set size in bytes, or -1 when the platform cannot report it.
qint64 peakResidentBytes()
{
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS counters;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
    {
        return static_cast<qint64>(counters.PeakWorkingSetSize);
    }
    return -1;
#elif defined(Q_OS_LINUX)
    QFile status(QStringLiteral("/proc/self/status"));
    if (!status.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return -1;
    }
    while (!status.atEnd())
    {
        const QByteArray line = status.readLine();
        if (line.startsWith("VmHWM:"))
        {
            const QList<QByteArray> fields = line.simplified().split(' ');
            if (fields.size() >= 2)
            {
                return fields.at(1).toLongLong() * 1024;
            }
        }
    }
    return -1;
#else
    return -1;
#endif
}

double bytesToMb(qint64 bytes)
{
    return bytes < 0 ? -1.0 : static_cast<double>(bytes) / (1024.0 * 1024.0);
}

double percentile(QList<double> values, double fraction)
{
    if (values.isEmpty())
    {
        return -1.0;
    }
    std::sort(values.begin(), values.end());
    const int index = static_cast<int>(std::ceil(fraction * values.size())) - 1;
    return values.at(std::clamp(index, 0, static_cast<int>(values.size()) - 1));
}

QString summarize(const QString &what, double actual, double target, bool lowerIsBetter)
{
    const bool pass = lowerIsBetter ? actual <= target : actual >= target;
    return QStringLiteral("%1: %2 (%3)").arg(what, QString::number(actual, 'f', 2), pass ? QStringLiteral("pass") : QStringLiteral("MISS"));
}

/// Counts JPEGs under a directory so the operator sees what will be scanned.
int countMediaFiles(const QString &directory)
{
    int count = 0;
    QDirIterator it(directory, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        if (DirectoryScanner::isSupportedImageFile(it.filePath()))
        {
            ++count;
        }
    }
    return count;
}

/// Runs the event loop while `work` proceeds, polling at a fixed interval and
/// recording the longest gap between polls. The gap is the closest available
/// proxy for a blocked UI thread without a real window.
class EventLoopStallProbe
{
public:
    void start()
    {
        m_longestMs = 0.0;
        m_ticks = 0;
        m_timer.start();
        m_last = m_timer.nsecsElapsed();
        m_poll.setInterval(EVENT_LOOP_POLL_MS);
        QObject::connect(&m_poll, &QTimer::timeout, &m_poll, [this]() {
            const qint64 now = m_timer.nsecsElapsed();
            const double gapMs = (now - m_last) / 1000000.0;
            m_longestMs = std::max(m_longestMs, gapMs);
            m_last = now;
            ++m_ticks;
        });
        m_poll.start();
    }

    void stop()
    {
        m_poll.stop();
    }

    [[nodiscard]] double longestMs() const { return m_longestMs; }
    [[nodiscard]] int ticks() const { return m_ticks; }

private:
    QTimer m_poll;
    QElapsedTimer m_timer;
    qint64 m_last = 0;
    double m_longestMs = 0.0;
    int m_ticks = 0;
};

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("photocarta_benchmark"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Measure PhotoCarta catalog and thumbnail performance on a real photo directory."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption mediaOption(QStringLiteral("media-dir"),
                                   QStringLiteral("Directory of photos to scan. Required."),
                                   QStringLiteral("path"));
    QCommandLineOption workOption(QStringLiteral("work-dir"),
                                  QStringLiteral("Directory for the benchmark catalog and thumbnails cache. Required."),
                                  QStringLiteral("path"));
    QCommandLineOption labelOption(QStringLiteral("label"),
                                   QStringLiteral("Free-form label recorded in the report, e.g. 'cold cache, SSD'."),
                                   QStringLiteral("text"));
    QCommandLineOption thumbnailsOption(QStringLiteral("thumbnails"),
                                        QStringLiteral("Number of thumbnails to generate for the throughput run (default %1).")
                                            .arg(DEFAULT_THUMBNAIL_COUNT),
                                        QStringLiteral("count"));
    QCommandLineOption jsonOption(QStringLiteral("json"),
                                  QStringLiteral("Write the JSON report to this file in addition to stdout."),
                                  QStringLiteral("path"));
    QCommandLineOption keepCatalogOption(QStringLiteral("keep-catalog"),
                                         QStringLiteral("Reuse an existing catalog in --work-dir instead of rescanning."));
    QCommandLineOption outputOption(QStringLiteral("output"),
                                    QStringLiteral("Markdown output: 'json' (default) or 'table' for a docs/benchmarks.md row."),
                                    QStringLiteral("format"),
                                    QStringLiteral("json"));

    parser.addOption(mediaOption);
    parser.addOption(workOption);
    parser.addOption(labelOption);
    parser.addOption(thumbnailsOption);
    parser.addOption(jsonOption);
    parser.addOption(keepCatalogOption);
    parser.addOption(outputOption);
    parser.process(app);

    const QString mediaDir = parser.value(mediaOption);
    const QString workDir = parser.value(workOption);
    if (mediaDir.isEmpty() || workDir.isEmpty())
    {
        fprintf(stderr, "error: --media-dir and --work-dir are required\n");
        return 2;
    }

    const QFileInfo mediaInfo(mediaDir);
    if (!mediaInfo.exists() || !mediaInfo.isDir())
    {
        fprintf(stderr, "error: --media-dir is not a directory: %s\n", qPrintable(mediaDir));
        return 2;
    }

    QDir().mkpath(workDir);
    const QString catalogPath = QDir(workDir).filePath(QStringLiteral("benchmark-catalog.db"));
    const QString thumbnailCacheDir = QDir(workDir).filePath(QStringLiteral("thumbnails"));

    const int thumbnailCount = std::max(1, parser.value(thumbnailsOption).toInt() > 0
                                               ? parser.value(thumbnailsOption).toInt()
                                               : DEFAULT_THUMBNAIL_COUNT);

    const QString label = parser.value(labelOption).trimmed();
    const QString outputFormat = parser.value(outputOption).toLower();

    // ---------------------------------------------------------------- ingest
    //
    // The catalog is created by the application's own scanner so ingestion is
    // measured on the real path (batch inserts of 50, one transaction per
    // batch) rather than on a synthetic writer.

    double ingestMs = -1.0;
    int scannedCount = 0;

    CatalogDatabase database(QStringLiteral("benchmark_catalog"));
    if (!database.open(catalogPath))
    {
        fprintf(stderr, "error: cannot open benchmark catalog at %s: %s\n",
                qPrintable(catalogPath), qPrintable(database.lastError()));
        return 2;
    }

    const bool reuseCatalog = parser.isSet(keepCatalogOption) && database.itemCount() > 0;

    if (reuseCatalog)
    {
        scannedCount = database.itemCount();
    }
    else
    {
        // Start from an empty catalog so the scan numbers describe this run.
        database.close();
        QFile::remove(catalogPath);
        QFile::remove(catalogPath + QStringLiteral("-wal"));
        QFile::remove(catalogPath + QStringLiteral("-shm"));
        if (!database.open(catalogPath))
        {
            fprintf(stderr, "error: cannot recreate benchmark catalog: %s\n", qPrintable(database.lastError()));
            return 2;
        }

        const int expected = countMediaFiles(mediaDir);
        if (expected == 0)
        {
            fprintf(stderr, "error: no supported images found under %s\n", qPrintable(mediaDir));
            return 2;
        }
        fprintf(stderr, "Scanning %d images from %s\n", expected, qPrintable(mediaDir));

        const int collectionId = database.addOrGetCollection(mediaDir, QStringLiteral("Benchmark"));

        DirectoryScanner scanner;
        QEventLoop scanLoop;
        QObject::connect(&scanner, &DirectoryScanner::batchDiscovered, &scanLoop,
                         [&database, collectionId](const QList<CatalogItem> &items) {
                             database.insertOrUpdateItems(items, collectionId);
                         });
        QObject::connect(&scanner, &DirectoryScanner::scanFinished, &scanLoop,
                         [&scanLoop](int totalFound, qint64 elapsedMs) {
                             Q_UNUSED(totalFound);
                             Q_UNUSED(elapsedMs);
                             scanLoop.quit();
                         });
        QObject::connect(&scanner, &DirectoryScanner::scanCanceled, &scanLoop, &QEventLoop::quit);

        QElapsedTimer scanTimer;
        scanTimer.start();
        QTimer::singleShot(0, &scanner, [&scanner, mediaDir]() { scanner.startScan(mediaDir, true); });
        scanLoop.exec();
        ingestMs = static_cast<double>(scanTimer.elapsed());
        scannedCount = database.itemCount();
    }

    const int itemCount = database.itemCount();
    if (itemCount == 0)
    {
        fprintf(stderr, "error: benchmark catalog is empty after scanning\n");
        return 2;
    }

    // ------------------------------------------------------------ catalog open
    //
    // Cold open: close the handle and reopen, then load every row into the
    // model. This is the path main.cpp takes at startup for a saved catalog.

    database.close();
    QThread::msleep(250); // let the OS drop the page cache hand-off
    const qint64 rssBeforeOpen = residentBytes();

    QElapsedTimer openTimer;
    openTimer.start();
    if (!database.open(catalogPath))
    {
        fprintf(stderr, "error: cannot reopen benchmark catalog: %s\n", qPrintable(database.lastError()));
        return 2;
    }
    CatalogModel model;
    model.setDatabase(&database);
    model.loadFromDatabase();
    CatalogFilterModel filterModel;
    filterModel.setSourceModel(&model);
    const double openMs = static_cast<double>(openTimer.elapsed());
    const int loadedCount = model.count();
    const qint64 rssAfterOpen = residentBytes();

    if (loadedCount != itemCount)
    {
        fprintf(stderr, "error: model loaded %d of %d rows\n", loadedCount, itemCount);
        return 2;
    }

    // ---------------------------------------------------------------- filtering
    //
    // Twenty filter changes over the loaded catalog: free-text search, rating,
    // and tag filters, each followed by a full row-count traversal so the cost
    // of populating the proxy is included.

    const QStringList availableTags = model.availableTags();
    QList<QString> searchTerms;
    searchTerms << QStringLiteral("a") << QStringLiteral("e") << QStringLiteral("i")
                << QStringLiteral("o") << QStringLiteral("u") << QStringLiteral("1")
                << QStringLiteral("2") << QStringLiteral("0") << QStringLiteral("img")
                << QStringLiteral("DSC") << QStringLiteral("IMG") << QStringLiteral("sun")
                << QStringLiteral("sky") << QStringLiteral("day") << QStringLiteral("2024")
                << QStringLiteral("2025") << QStringLiteral("p") << QStringLiteral("x");

    QList<double> filterTimes;
    auto measureFilter = [&filterModel, &filterTimes](const std::function<void()> &change) {
        QElapsedTimer timer;
        timer.start();
        change();
        const int count = filterModel.rowCount();
        Q_UNUSED(count);
        filterTimes.append(static_cast<double>(timer.nsecsElapsed()) / 1000000.0);
    };

    for (const QString &term : searchTerms)
    {
        measureFilter([&filterModel, term]() { filterModel.setSearchQuery(term); });
    }
    filterModel.setSearchQuery(QString());
    for (int rating = 1; rating <= 3; ++rating)
    {
        measureFilter([&filterModel, rating]() { filterModel.setMinRating(rating); });
    }
    filterModel.setMinRating(0);
    for (int i = 0; i < std::min(3, static_cast<int>(availableTags.size())); ++i)
    {
        const QString tag = availableTags.at(i);
        measureFilter([&filterModel, tag]() { filterModel.setTagFilter(tag); });
        measureFilter([&filterModel]() { filterModel.setTagFilter(QString()); });
    }
    filterModel.setSearchQuery(QString());
    filterModel.setCategoryFilter(QStringLiteral("all"));
    filterModel.setTagFilter(QString());
    filterModel.setMinRating(0);
    filterModel.setColorLabelFilter(0);
    filterModel.setCollectionFilter(0);

    const double filterP95 = percentile(filterTimes, 0.95);
    const double filterMedian = percentile(filterTimes, 0.50);
    const double filterMax = percentile(filterTimes, 1.0);

    // -------------------------------------------------------------- thumbnails

    QDir(thumbnailCacheDir).removeRecursively();
    ThumbnailCache &cache = ThumbnailCache::instance();
    cache.setDiskCachePath(thumbnailCacheDir);
    cache.clearMemoryCache();
    cache.setMemoryCacheMaxCost(64 * 1024 * 1024);

    const QList<CatalogItem> &items = model.items();
    QList<QString> thumbnailPaths;
    thumbnailPaths.reserve(thumbnailCount);
    const qsizetype stride = std::max<qsizetype>(1, items.size() / thumbnailCount);
    const qsizetype maxThumbnails = thumbnailCount;
    for (qsizetype i = 0; i < items.size() && thumbnailPaths.size() < maxThumbnails; i += stride)
    {
        thumbnailPaths.append(items.at(i).filePath);
    }

    EventLoopStallProbe probe;
    double longestStallMs = -1.0;
    int probeTicks = 0;
    int generated = 0;
    QElapsedTimer thumbTimer;

    if (!thumbnailPaths.isEmpty())
    {
        // Measure only generation: the cache is empty, so every call decodes.
        probe.start();
        thumbTimer.start();
        for (const QString &path : thumbnailPaths)
        {
            const QImage image = cache.getOrCreateThumbnail(path, QSize(256, 256));
            if (!image.isNull())
            {
                ++generated;
            }
            // Yield so the stall probe can observe event-loop responsiveness.
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        }
        probe.stop();
        longestStallMs = probe.longestMs();
        probeTicks = probe.ticks();
    }

    const double thumbSeconds = static_cast<double>(thumbTimer.nsecsElapsed()) / 1e9;
    const double thumbPerSecond = thumbSeconds > 0.0 ? generated / thumbSeconds : -1.0;

    // ---------------------------------------------------------------- memory

    const qint64 rssAfterThumbnails = residentBytes();
    const qint64 peakRss = peakResidentBytes();

    // ----------------------------------------------------------------- report

    const double openTargetPass = openMs <= OPEN_TARGET_MS ? 1 : 0;
    const double filterPass = filterP95 <= FILTER_P95_TARGET_MS ? 1 : 0;
    const double thumbnailPass = thumbPerSecond >= THUMBNAIL_TARGET_PER_SECOND ? 1 : 0;
    const double stallPass = probeTicks >= 10 && longestStallMs >= 0 && longestStallMs <= STALL_TARGET_MS ? 1 : 0;
    const double memoryPass = bytesToMb(peakRss) >= 0 && bytesToMb(peakRss) <= MEMORY_TARGET_MB ? 1 : 0;

    QJsonObject root;
    root.insert(QStringLiteral("label"), label);
    root.insert(QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    root.insert(QStringLiteral("mediaDir"), QDir::toNativeSeparators(mediaDir));
    root.insert(QStringLiteral("catalogPath"), QDir::toNativeSeparators(catalogPath));
    root.insert(QStringLiteral("itemCount"), itemCount);
    root.insert(QStringLiteral("reusedCatalog"), reuseCatalog);

    QJsonObject environment;
    environment.insert(QStringLiteral("qtVersion"), QString::fromLatin1(qVersion()));
    environment.insert(QStringLiteral("cpuCores"), QThread::idealThreadCount());
    environment.insert(QStringLiteral("os"), QSysInfo::prettyProductName());
    environment.insert(QStringLiteral("buildType"),
#ifdef QT_NO_DEBUG
                       QStringLiteral("Release")
#else
                       QStringLiteral("Debug")
#endif
    );
    root.insert(QStringLiteral("environment"), environment);

    QJsonObject ingest;
    ingest.insert(QStringLiteral("scanMs"), ingestMs);
    ingest.insert(QStringLiteral("itemsScanned"), scannedCount);
    ingest.insert(QStringLiteral("itemsPerSecond"),
                  ingestMs > 0 ? scannedCount / (ingestMs / 1000.0) : -1.0);
    root.insert(QStringLiteral("ingest"), ingest);

    QJsonObject open;
    open.insert(QStringLiteral("openAndLoadMs"), openMs);
    open.insert(QStringLiteral("targetMs"), OPEN_TARGET_MS);
    open.insert(QStringLiteral("pass"), openMs <= OPEN_TARGET_MS);
    open.insert(QStringLiteral("rowsLoaded"), loadedCount);
    open.insert(QStringLiteral("rssBeforeMb"), bytesToMb(rssBeforeOpen));
    open.insert(QStringLiteral("rssAfterMb"), bytesToMb(rssAfterOpen));
    root.insert(QStringLiteral("catalogOpen"), open);

    QJsonObject filter;
    filter.insert(QStringLiteral("samples"), filterTimes.size());
    filter.insert(QStringLiteral("medianMs"), filterMedian);
    filter.insert(QStringLiteral("p95Ms"), filterP95);
    filter.insert(QStringLiteral("maxMs"), filterMax);
    filter.insert(QStringLiteral("targetP95Ms"), FILTER_P95_TARGET_MS);
    filter.insert(QStringLiteral("pass"), filterP95 <= FILTER_P95_TARGET_MS);
    root.insert(QStringLiteral("filter"), filter);

    QJsonObject thumbnails;
    thumbnails.insert(QStringLiteral("requested"), thumbnailPaths.size());
    thumbnails.insert(QStringLiteral("generated"), generated);
    thumbnails.insert(QStringLiteral("elapsedSeconds"), thumbSeconds);
    thumbnails.insert(QStringLiteral("perSecond"), thumbPerSecond);
    thumbnails.insert(QStringLiteral("targetPerSecond"), THUMBNAIL_TARGET_PER_SECOND);
    thumbnails.insert(QStringLiteral("pass"), thumbPerSecond >= THUMBNAIL_TARGET_PER_SECOND);
    thumbnails.insert(QStringLiteral("longestEventLoopStallMs"), longestStallMs);
    thumbnails.insert(QStringLiteral("stallProbeTicks"), probeTicks);
    // A run whose probe almost never fired cannot claim a stall measurement:
    // processEvents() starves the timer when the loop is saturated, so the
    // gap between ticks overstates real blocking. Require a usable sample.
    thumbnails.insert(QStringLiteral("stallMeasured"), probeTicks >= 10);
    thumbnails.insert(QStringLiteral("stallTargetMs"), STALL_TARGET_MS);
    thumbnails.insert(QStringLiteral("stallPass"), stallPass == 1);
    root.insert(QStringLiteral("thumbnails"), thumbnails);

    QJsonObject memory;
    memory.insert(QStringLiteral("rssAfterThumbnailsMb"), bytesToMb(rssAfterThumbnails));
    memory.insert(QStringLiteral("peakRssMb"), bytesToMb(peakRss));
    memory.insert(QStringLiteral("targetMb"), MEMORY_TARGET_MB);
    memory.insert(QStringLiteral("pass"), memoryPass == 1);
    root.insert(QStringLiteral("memory"), memory);

    QJsonObject verdict;
    verdict.insert(QStringLiteral("openPass"), openTargetPass == 1);
    verdict.insert(QStringLiteral("filterPass"), filterPass == 1);
    verdict.insert(QStringLiteral("thumbnailPass"), thumbnailPass == 1);
    verdict.insert(QStringLiteral("stallPass"), stallPass == 1);
    verdict.insert(QStringLiteral("memoryPass"), memoryPass == 1);
    verdict.insert(QStringLiteral("allPass"),
                   openTargetPass == 1 && filterPass == 1 && thumbnailPass == 1
                       && stallPass == 1 && memoryPass == 1);
    root.insert(QStringLiteral("verdict"), verdict);

    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);

    if (outputFormat == QStringLiteral("table"))
    {
        // Row for docs/benchmarks.md. Missing platforms stay explicit rather
        // than being filled with guessed values.
        printf("| %s | %d | %.0f ms (%s) | %.1f ms (%s) | %.1f/s (%s) | %.0f ms (%s) | %.0f MB (%s) |\n",
               label.isEmpty() ? "unlabelled" : qPrintable(label),
               itemCount,
               openMs, openMs <= OPEN_TARGET_MS ? "pass" : "MISS",
               filterP95, filterP95 <= FILTER_P95_TARGET_MS ? "pass" : "MISS",
               thumbPerSecond, thumbPerSecond >= THUMBNAIL_TARGET_PER_SECOND ? "pass" : "MISS",
               longestStallMs, stallPass == 1 ? "pass" : "MISS",
               bytesToMb(peakRss), memoryPass == 1 ? "pass" : "MISS");
    }
    else
    {
        fputs(json.constData(), stdout);
        fflush(stdout);
    }

    fprintf(stderr, "\nSummary (%d items)\n", itemCount);
    fprintf(stderr, "  %s\n", qPrintable(summarize(QStringLiteral("Catalog open"), openMs, OPEN_TARGET_MS, true)));
    fprintf(stderr, "  %s\n", qPrintable(summarize(QStringLiteral("Filter p95"), filterP95, FILTER_P95_TARGET_MS, true)));
    fprintf(stderr, "  %s\n", qPrintable(summarize(QStringLiteral("Thumbnails/s"), thumbPerSecond, THUMBNAIL_TARGET_PER_SECOND, false)));
    fprintf(stderr, "  %s\n", qPrintable(summarize(QStringLiteral("Longest stall"), longestStallMs, STALL_TARGET_MS, true)));
    fprintf(stderr, "  %s\n", qPrintable(summarize(QStringLiteral("Peak RSS"), bytesToMb(peakRss), MEMORY_TARGET_MB, true)));

    const QString jsonPath = parser.value(jsonOption);
    if (!jsonPath.isEmpty())
    {
        QFile out(jsonPath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            fprintf(stderr, "error: cannot write %s\n", qPrintable(jsonPath));
            return 2;
        }
        out.write(json);
    }

    return 0;
}
