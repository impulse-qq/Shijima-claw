# Codebase Concerns

**Analysis Date:** 2026-04-19

## Tech Debt

### ShijimaManager is a God Object (1121 lines)
- **Files:** `ShijimaManager.cc` (1121 lines), `ShijimaManager.hpp` (130 lines)
- **Issue:** Single class handles mascot lifecycle, UI management, settings persistence, HTTP API coordination, screen management, breeding logic, window tracking, import/export, toolbar building, and tick processing
- **Impact:** Any change to mascot behavior risks breaking unrelated functionality. The class has 30+ member variables and 40+ methods
- **Fix approach:** Extract responsibilities into separate classes: `MascotLifecycleManager`, `SettingsManager`, `ScreenEnvironmentManager`, `ImportManager`

### Build system relies on un-initialized git submodules
- **Files:** `.gitmodules`, `libshijima/`, `libshimejifinder/`, `cpp-httplib/`
- **Issue:** Three critical submodules use SSH URLs (`git@github.com:...`) which fail for users without SSH keys configured. The `cpp-httplib` submodule uses HTTPS but the other two do not
- **Impact:** Fresh clones cannot build without manual `git submodule update --init --recursive`. SSH-based URLs block CI and contributors without GitHub SSH access
- **Fix approach:** Convert submodule URLs to HTTPS: `https://github.com/pixelomer/libshijima` and `https://github.com/pixelomer/libshimejifinder`

### `.gitignore` uses blanket `*` with exceptions
- **Files:** `.gitignore`
- **Issue:** Line 1 is `*` (ignore everything), then ~70 lines of exceptions. This is fragile — any new file type not explicitly whitelisted gets silently ignored
- **Impact:** New source files, configs, or assets added without updating `.gitignore` will not be tracked, leading to "works on my machine" issues
- **Fix approach:** Invert the pattern: ignore specific build artifacts (`*.o`, `*.a`, `publish/`, `shijima-qt`, `*.AppImage`) instead of ignoring everything

### `DefaultMascot.cc` is auto-generated but not committed
- **Files:** `Makefile` (line 150-152), `.gitignore` (line 58)
- **Issue:** Generated at build time from 46 PNG files + 2 XML files. If any default mascot asset is missing, build fails silently or produces incomplete output
- **Impact:** Build is fragile if `DefaultMascot/` directory is incomplete
- **Fix approach:** Add build-time validation that all 46 expected PNG files exist before generating

### Qt5 scale-detection code still present but dead
- **Files:** `Platform/Linux/Platform.cc` (lines 20-101)
- **Issue:** `QT5_MANUALLY_DETERMINE_SCALE` block initializes QApplication twice, sets environment variables, and calls `execv()` to restart the process. This code path is only active if `QT_VERSION < 0x060000` but the project requires Qt6
- **Impact:** Dead code increases maintenance burden and confusion
- **Fix approach:** Remove the entire `#ifdef QT5_MANUALLY_DETERMINE_SCALE` block

## Known Bugs

### Mascot list refresh is O(n) full rebuild
- **Files:** `ShijimaManager.cc` (line 478), FIXME comment
- **Symptoms:** `refreshListWidget()` clears and rebuilds the entire list widget every time any mascot data changes
- **Trigger:** Any mascot import, reload, or deletion
- **Workaround:** None — functional but inefficient with many mascots
- **Fix approach:** Implement incremental refresh using `m_listItemsToRefresh` set (already declared but unused)

### `m_idCounter` is uninitialized in constructor
- **Files:** `ShijimaManager.hpp` (line 111), `ShijimaManager.cc` (line 702)
- **Symptoms:** `m_idCounter` has no default value in the header and is not in the constructor initializer list
- **Trigger:** First mascot spawn before `m_idCounter` is explicitly set
- **Impact:** Undefined behavior — first mascot ID could be garbage. In practice, zero-initialization of heap memory usually makes this 0, but this is not guaranteed
- **Fix approach:** Add `m_idCounter(0)` to the constructor initializer list

### `m_hasTickCallbacks` is uninitialized
- **Files:** `ShijimaManager.hpp` (line 126), `ShijimaManager.cc` (line 703)
- **Symptoms:** Same pattern as `m_idCounter` — declared without default, not in initializer list
- **Impact:** Undefined behavior on first `tick()` call
- **Fix approach:** Add `m_hasTickCallbacks(false)` to the constructor initializer list

### `sendJson` dereferences potentially empty QByteArray
- **Files:** `ShijimaHttpApi.cc` (lines 113-117)
- **Issue:** `sendJson` does `&bytes[0]` without checking if `bytes` is empty. `QJsonDocument::toJson(QJsonDocument::Compact)` on an empty object returns `"{}\n"` (non-empty), but if the document were truly empty this would be UB
- **Impact:** Low risk with current usage, but fragile if `sendJson` is called with edge-case documents
- **Fix approach:** Add `if (bytes.isEmpty()) return;` guard

### `jsonForRequest` silently returns empty optional on parse errors
- **Files:** `ShijimaHttpApi.cc` (lines 92-111)
- **Issue:** JSON parse errors are logged to stderr but the caller only sees an empty optional, returning a generic 400 without details
- **Impact:** API clients receive no information about what went wrong with their request body
- **Fix approach:** Return a 400 with the parse error message in the JSON response

## Security Considerations

### HTTP API binds to localhost with no authentication
- **Files:** `ShijimaHttpApi.cc` (line 354), `main.cc` (line 42)
- **Risk:** API listens on `127.0.0.1:32456` with no auth, CORS, or rate limiting. Any local process can spawn/dismiss mascots, execute script selectors, and read mascot state
- **Files:** `ShijimaHttpApi.cc` (lines 126-139) — `selectorEval` executes user-provided strings through a script context
- **Risk:** The `selector` query parameter is evaluated as a script expression via `mascot->mascot().script_ctx->eval_bool(selector)`. While limited to boolean evaluation, this is effectively arbitrary code execution within the script context
- **Current mitigation:** Bound to localhost only
- **Recommendations:** 
  - Add a simple token-based auth (env var or config file)
  - Validate/sanitize `selector` input before eval
  - Add request size limits to prevent DoS

### Mascot import uses `--force` flag on extension install
- **Files:** `Platform/Linux/GNOME.cc` (lines 63, 68)
- **Risk:** `gnome-extensions install --force` overwrites existing extensions without confirmation. A malicious archive could replace a legitimate GNOME extension
- **Current mitigation:** Only triggered by explicit user action (import dialog)
- **Recommendations:** Add extension signature/integrity check before `--force` install

### `std::filesystem::remove_all` on user-controlled paths
- **Files:** `ShijimaManager.cc` (lines 264-272)
- **Risk:** The delete action removes mascot directories using `std::filesystem::remove_all`. While the code comments acknowledge this ("remove_all(path) could be dangerous"), path traversal in a `.mascot` directory name could escape the intended directory
- **Current mitigation:** Path is constructed from `mascotData->path()` which comes from the known mascots directory
- **Recommendations:** Validate that the resolved path is within `m_mascotsPath` before deletion

### `WAYLAND_DISPLAY` environment variable is forcibly cleared
- **Files:** `Platform/Linux/Platform.cc` (line 126)
- **Risk:** `setenv("WAYLAND_DISPLAY", "", 1)` modifies the process environment globally. This could affect other libraries or Qt plugins that check for Wayland
- **Impact:** Forces X11 backend even on Wayland compositors that support XWayland. This is intentional for window positioning but may break other Qt Wayland features
- **Recommendations:** Document this behavior clearly; consider if Qt's `QT_QPA_PLATFORM=xcb` is a cleaner approach

## Performance Bottlenecks

### Mascot tick runs synchronously on main thread
- **Files:** `ShijimaManager.cc` (lines 949-1052), `ShijimaWidget.cc` (lines 244-285)
- **Problem:** All mascot ticks run sequentially in a single timer callback. With N mascots, each tick involves: state update, asset loading, sound check, repaint, and inspector update
- **Cause:** Timer fires every 10ms (40ms / 4 subticks). Each mascot's `tick()` is blocking
- **Improvement path:** For large mascot counts, consider batching repaints or using a render thread. Asset loading in `getActiveAsset()` could be cached more aggressively

### Asset loading has no eviction policy
- **Files:** `AssetLoader.cc` (lines 43-63)
- **Problem:** `loadAsset()` loads images into a `QMap` cache that grows unbounded. Images are never evicted unless `unloadAssets()` is explicitly called with a root path
- **Cause:** No LRU or memory-limit policy
- **Improvement path:** Add memory-based cache eviction (e.g., max 100MB) or LRU with configurable size

### `refreshListWidget()` rebuilds entire list
- **Files:** `ShijimaManager.cc` (lines 477-489)
- **Problem:** Every mascot data change triggers `m_listWidget.clear()` followed by re-adding all items. This causes visible flicker and is O(n) where n = loaded mascot count
- **Improvement path:** Use `m_listItemsToRefresh` (already declared at `ShijimaManager.hpp` line 116) for incremental updates

### `onTickSync` blocks calling thread until next tick
- **Files:** `ShijimaManager.cc` (lines 638-643)
- **Problem:** HTTP API handlers call `onTickSync()` which acquires a mutex and waits on a condition variable for the next timer tick. This means every HTTP request blocks for up to 10ms (the tick interval)
- **Impact:** API latency is bounded by tick frequency. Concurrent requests queue behind the mutex
- **Improvement path:** Consider a separate read path for non-mutating API endpoints that doesn't require the tick lock

## Fragile Areas

### `ShijimaManager::tick()` iterates and mutates `m_mascots` during iteration
- **Files:** `ShijimaManager.cc` (lines 991-1042)
- **Why fragile:** The tick loop iterates `m_mascots` in reverse, deleting widgets and erasing from the list. Breeding logic adds new widgets to the same list during iteration
- **Safe modification:** The reverse iteration + `erasePos` pattern is correct for deletion, but adding new mascots via breeding during the same loop is risky if the container is ever changed from `std::list` to something with iterator invalidation on insert
- **Test coverage:** Zero — no tests exist for any code path

### `setWindowedMode` recreates all mascot widgets
- **Files:** `ShijimaManager.cc` (lines 645-696)
- **Why fragile:** Toggling windowed mode closes all existing mascot widgets, creates new ones with copied state, and re-parents them. Any state not explicitly copied is lost
- **Safe modification:** The copy constructor `ShijimaWidget(ShijimaWidget &old, ...)` is used, but inspector state is the only thing explicitly preserved (`inspectorWasVisible`)
- **Test coverage:** None

### Linux platform code modifies global state
- **Files:** `Platform/Linux/Platform.cc` (lines 40-41, 110-127)
- **Why fragile:** `terminateServerFd` and `terminateClientFd` are global variables. `windowMasksEnabled` is a global bool modified by `KWin::running()`. Signal handlers use these globals
- **Safe modification:** Any code that calls `KWin::running()` before `initialize()` will see incorrect `windowMasksEnabled` state
- **Test coverage:** None

### `dispatchToMainThread` leaks QTimer on error
- **Files:** `ShijimaManager.cc` (lines 81-90)
- **Why fragile:** Creates a `QTimer` with `new` and relies on `deleteLater()` in the callback. If the callback throws or the event loop is destroyed before the timer fires, the timer leaks
- **Safe modification:** Use `std::unique_ptr<QTimer>` with a custom deleter that calls `deleteLater()`

### `m_windowObserverTimer` may fire after observer is destroyed
- **Files:** `ShijimaManager.cc` (lines 739-741), `ShijimaManager.hpp` (line 107)
- **Why fragile:** Timer is started based on `m_windowObserver.tickFrequency()` but there's no corresponding `killTimer()` in the destructor. If the observer's internal state changes, the timer may fire with stale data
- **Test coverage:** None

## Scaling Limits

### Mascot count limited by main-thread tick budget
- **Current capacity:** ~10-20 mascots before visible stutter (estimated, not measured)
- **Limit:** Single-threaded tick at 10ms intervals. Each mascot requires: state update, asset lookup, sound check, repaint, and inspector sync
- **Scaling path:** Offload state updates to a worker thread; batch repaints; use dirty-flag rendering

### HTTP API has no request queuing or concurrency control
- **Current capacity:** Serial request processing (one at a time due to mutex)
- **Limit:** `onTickSync` serializes all requests. Under load, requests queue indefinitely
- **Scaling path:** Add request timeout, queue size limit, or read-write lock separation

## Dependencies at Risk

### cpp-httplib is a header-only dependency with no version pinning
- **Risk:** Submodule points to `https://github.com/yhirose/cpp-httplib` with no branch/tag specified. Updates to the submodule could introduce breaking changes
- **Impact:** HTTP server behavior could change without warning
- **Migration plan:** Pin to a specific commit or release tag in `.gitmodules`

### Qt6 version not pinned in build config
- **Risk:** `common.mk` uses `pkg-config` to find Qt6 libraries. Different Qt6 minor versions may have ABI or behavior differences
- **Impact:** Build may succeed but runtime behavior may differ across systems
- **Migration plan:** Add minimum Qt6 version check in `common.mk` (e.g., `QT_VERSION >= 6.4`)

## Missing Critical Features

### No error reporting or crash collection
- **Problem:** Errors are logged to stderr only (`std::cerr`). In GUI mode, stderr may not be visible to users
- **Blocks:** Debugging user-reported issues, especially on Windows where console output is hidden
- **Priority:** High — add a log file in the app data directory

### No configuration for HTTP API port
- **Problem:** Port 32456 is hardcoded in `main.cc` and `ShijimaManager.cc`
- **Blocks:** Running multiple instances (though single-instance is enforced), or using a different port if 32456 is occupied
- **Priority:** Medium — make port configurable via environment variable or settings

### No graceful shutdown on SIGINT/SIGTERM (Linux)
- **Problem:** Signal handler writes to a socket which triggers `QGuiApplication::exit(0)`, but this bypasses `ShijimaManager::finalize()` and `AssetLoader::finalize()` cleanup
- **Blocks:** Clean resource release on signal-based termination
- **Priority:** Medium — call finalize functions in the signal handler or use `QCoreApplication::aboutToQuit`

## Test Coverage Gaps

### Entire codebase has zero tests
- **What's not tested:** Every function, class, and code path
- **Files:** All `.cc` and `.hpp` files
- **Risk:** Any change could introduce regressions that go undetected until runtime
- **Priority:** High — start with unit tests for:
  - `ShijimaHttpApi` endpoint handlers (mock `ShijimaManager`)
  - `AssetLoader` caching behavior
  - `MascotData` parsing
  - `SoundEffectManager` play/stop lifecycle
  - CLI argument parsing (`cli.cc`)

### No integration tests for cross-platform behavior
- **What's not tested:** Platform-specific code paths in `Platform/Linux/`, `Platform/macOS/`, `Platform/Windows/`
- **Risk:** Platform-specific bugs only discovered by users
- **Priority:** High — at minimum, CI should build on all three platforms

### No tests for error handling paths
- **What's not tested:** Exception handling in `import()`, `spawn()`, DBus calls, file I/O
- **Risk:** Error paths may crash or leak resources
- **Priority:** Medium — test that errors are caught and reported gracefully

---

*Concerns audit: 2026-04-19*