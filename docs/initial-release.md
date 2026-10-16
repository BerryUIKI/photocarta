# Initial Release Scope

## Agreed priority

Complete the GUI changes and release early. Improve functionality incrementally
afterward. The project owner and development team will agree later milestones;
this document does not schedule a full digiKam replacement or feature expansion.

## Initial implementation focus

1. Bring up the reused application/backend and a new Qt Quick shell on Windows,
   macOS, and Linux. Validate the media-preview and backend integration approach.
2. Replace the main photo management interface: navigation, collection browsing,
   thumbnail grid, preview, metadata panel, selection, and commonly used actions.
3. Connect the new interface to existing search/filter, tagging, rating, and
   background-operation behavior. Keep advanced tools accessible through retained
   windows where practical.
4. Fix integration regressions, verify installation and the main workflows on all
   three platforms, and prepare an initial release with known limitations.

These are the initial implementation order, not separate dated release promises.
Avoid adding new product features to the GUI milestone unless agreed explicitly.

## Visual direction

Use a photo-focused professional desktop layout, neutral colors, consistent
spacing and typography, collapsible side panels, and light/dark themes. Build a
shared component style rather than applying isolated visual changes to each view.
Validate the main layout with an early visual prototype before broad conversion.
The first runnable GUI slice (`photocarta`) satisfies this milestone with dark/light themes,
responsive sidebars, a virtualized thumbnail grid, preview mode, and procedural demo fixtures;
see [Local build instructions](build-instructions.md).

## Release checks

- The application builds, installs, starts, and closes on all three target platforms.
- Users can browse registered collections and preview supported photos.
- Selection, navigation, implemented filters, tagging, and rating reach the existing
  backend correctly and persist across restarts.
- Retained tools have valid context and report unsupported transitions clearly.
- Main-window resizing, display scaling, keyboard navigation, input, and both themes
  work in the converted interface.
- Offline storage is represented correctly without treating absent media as deleted.
- GUI integration has been tested against copied catalogs and disposable media.
- Known compatibility and performance limits, source license notices, and packaging
  dependencies are documented before distribution.

## Later work

Replacement of remaining tool windows, new functionality, additional storage
integrations, and deeper million-item optimization remain a backlog. Their order
and release scope will be decided together after the first usable GUI is available.
