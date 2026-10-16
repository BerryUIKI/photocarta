# Storage Choices and Scale

## Storage model

Photo originals, catalog databases, and derived data have separate placement
policies. A user can combine multiple collection roots in one catalog. Initial
implementation should expose and preserve upstream capabilities before adding
new storage backends.

| Original location | Intended user experience | Catalog and derived data |
| --- | --- | --- |
| Local disk | Register existing folders and browse in place. | Local catalog; local fast storage preferred for thumbnails. |
| Removable drive | Register an external drive; show its availability and support reconnection. | Keep catalog and cached thumbnails local by default. |
| Mounted NAS or network share | Register an OS-accessible SMB/NFS mount or supported network path. | Local catalog by default, or MariaDB server; local thumbnail storage preferred. |
| Mixed locations | Browse local, removable, and network collections together. | One catalog with explicit collection identities and availability. |

Registering a folder does not imply copying or moving originals. Existing import
tools may provide explicit copy operations. Do not add a new managed-library format
or automatic relocation workflow to the first release.

An unavailable drive or share must remain distinguishable from deleted photos.
Show previously indexed information and cached thumbnails where available; require
the original to be reachable for operations that need it. Reconnection and path
updates should preserve collection identity using upstream mechanisms where possible.
Do not infer deletion from a failed mount or temporary network interruption.

## Database choices

| Choice | Intended use | Delivery policy |
| --- | --- | --- |
| Local SQLite | Default setup without a database server. | Preserve the existing backend and evaluate WAL on local storage. |
| Local MariaDB | Optional server-based catalog for users who need it. | Preserve upstream support; bundled server distribution is not an initial requirement. |
| Remote MariaDB | An existing database server on a NAS or other host. | Preserve configuration access and validate the supported client workflow. |

Do not offer SQLite database files on SMB/NFS shares. SQLite WAL requires database
clients to run on the same host. Network photo storage does not require placing
the database file on that network share.

Use MariaDB rather than advertising generic MySQL compatibility: the inspected
9.2.0 upstream documentation explicitly drops MySQL support. A remote database
connection is not a promise of collaborative editing, live synchronization, or
conflict-free access by multiple application instances. Those require separate
design and validation and are outside the first release.

Provide simple storage choices during setup and retain advanced database settings
without requiring ordinary users to configure a server. Use existing configuration
paths or retained settings windows first if a full settings rewrite delays release.

## Cache placement

Prefer local SSD/NVMe storage for thumbnail and preview data. Reuse the upstream
thumbnail store initially; do not replace it with a new storage architecture for
the GUI release. Keep disposable previews distinguishable from catalog metadata.

Configurable preview cache limits, eviction, and recovery behavior are follow-up
work unless needed to fix a demonstrated release problem. Catalog and originals
need their own backup strategy; a thumbnail cache is not a backup.

## One-million-item design target

One million cataloged items is a target, not a release performance guarantee.
Preserve these boundaries while building the GUI:

- Instantiate delegates for the visible area and bounded prefetch region.
- Keep full photo records and image buffers out of the QML object graph.
- Run blocking collection, database, and media work outside the GUI thread.
- Request previews on demand and avoid unbounded background queues.
- Avoid adding full-catalog loads, sorts, or rescans to routine UI actions.

View virtualization alone does not bound backend memory use. Profile upstream
models and query paths before introducing pagination, query pushdown, indexing,
or scheduler changes. Make those changes incrementally where evidence justifies
them, without turning the first release into a backend rewrite.

## Validation and deferred capabilities

Establish reproducible measurements on 10K, 100K, and 1M catalog records. Record
hardware, database type, media location, warm/cold cache state, first-visible-item
latency, query latency, scrolling responsiveness, memory, and disk use. Synthetic
catalogs exercise query/model behavior; real media tests are still needed for
scanning, metadata, thumbnails, RAW previews, and network I/O.

Initial release validation covers the agreed GUI scope and regressions on all
three platforms. Million-item benchmark results and limitations should be reported
as they become available. Any discovered data-loss issue or unusable main workflow
is a release defect; comprehensive scale optimization is not a prerequisite for
shipping the initial GUI release.

Direct S3/object storage, cloud placeholder hydration, cloud synchronization,
automatic storage tiering, and collaborative access are not first-release promises.
Their priorities will be agreed later.

## References

- [digiKam collection settings](https://docs.digikam.org/en/setup_application/collections_settings.html)
- [digiKam database settings](https://docs.digikam.org/en/setup_application/database_settings.html)
- [SQLite write-ahead logging](https://www.sqlite.org/wal.html)
- [Appropriate uses for SQLite](https://www.sqlite.org/whentouse.html)
- Local source: `core/libs/database/collection/collectionlocation.h` and
  `core/libs/database/engine/dbengineparameters.h` at the inspected commit.
