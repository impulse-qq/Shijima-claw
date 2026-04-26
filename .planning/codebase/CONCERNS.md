# Codebase Concerns

**Analysis Date:** 2026-04-26

## Technical Debt

### ShijimaManager is a God Object (1153 lines)
- **Files:** `ShijimaManager.cc` (1153 lines), `ShijimaManager.hpp` (135 lines)
- **Issue:** Single class handles mascot lifecycle, UI management, settings persistence, HTTP API coordination, screen management, breeding logic, window tracking, import/export, toolbar building, tick processing, and Matrix integration. The class has 31 member variables and 40+ methods
- **Impact:** Any change to mascot behavior risks breaking unrelated functionality. The class handles responsibility domains that should be separate
- **Fix approach:** Extract responsibilities into separate classes: `MascotLifecycleManager`, `SettingsManager`, `ScreenEnvironmentManager`, `ImportManager`, `MatrixIntegrationManager`

### Platform subdirectory build dependency chain is fragile
- **Files:** `Platform/Makefile`, `Makefile` (lines 142-143)
- **Issue:** The main Makefile depends on `Platform/Platform.a` but the Platform subdirectory makefile has a complex dependency chain involving `$(BUILD_PLATFORM)` that triggers `clean` before `all` in some cases. The `TODO_build_platform.md` document describes this issue and a workaround requiring manual `cp Platform/Linux/Linux.a Platform/Platform.a`
- **Impact:** Fresh builds or builds after cleaning may fail with "cannot find Platform/Platform.a"
- **Fix approach:** Make Platform build a proper dependency, not a file copy. The git log shows "fix: resolve build errors and Platform/Makefile dependency chain" but the `TODO_build_platform.md` file was not removed, suggesting the issue may persist

### Build system relies on SSH-based git submodules
- **Files:** `.gitmodules` (lines 3, 6)
- **Issue:** `libshijima` and `libshimejifinder` submodules use SSH URLs (`git@github.com:pixelomer/...`) which fail for users without SSH keys configured. Only `cpp-httplib` uses HTTPS
- **Impact:** Fresh clones cannot build without manual `git submodule update --init --recursive`. SSH-based URLs block CI systems and contributors without GitHub SSH access
- **Fix approach:** Convert to HTTPS: `https://github.com/pixelomer/libshijima` and `https://github.com/pixelomer/libshimejifinder`

### `.gitignore` uses blanket `*` with exceptions
- **Files:** `.gitignore` (line 1)
- **Issue:** Line 1 is `*` (ignore everything), then ~70 lines of exceptions. This is fragile — any new file type not explicitly whitelisted gets silently ignored
- **Impact:** New source files, configs, or assets added without updating `.gitignore` will not be tracked, leading to "works on my machine" issues
- **Fix approach:** Invert the pattern: ignore specific build artifacts (`*.o`, `*.a`, `*.AppImage`, `shijima-qt`) instead of ignoring everything

### DefaultMascot.cc is auto-generated at build time
- **Files:** `Makefile` (lines 27-28, 150-152), `.gitignore` (line 58), `DefaultMascot.cc`
- **Issue:** Generated at build time from 46 PNG files + 2 XML files. If any default mascot asset is missing, build fails or produces incomplete output. The file is generated into the source tree (not build tree)
- **Impact:** Build is fragile if `DefaultMascot/` directory is incomplete. Generated file is 820KB of binary image data committed to the repo
- **Fix approach:** Add build-time validation that all expected files exist, or move generation to a separate build step with a stamp file

### cpp-httplib is an unpinned submodule
- **Files:** `.gitmodules` (line 9)
- **Issue:** Submodule points to `https://github.com/yhirose/cpp-httplib` with no branch/tag/commit specified
- **Impact:** Updates to the submodule could introduce breaking changes to the HTTP server behavior without warning
- **Fix approach:** Pin to a specific release tag (e.g., `v0.14.0`)

### Qt6 version not pinned in build config
- **Files:** `common.mk` (uses `pkg-config --modversion Qt6`)
- **Issue:** Different Qt6 minor versions may have ABI or behavior differences. The `common.mk` does not enforce a minimum version
- **Impact:** Build may succeed but runtime behavior may differ across systems
- **Fix approach:** Add minimum Qt6 version check in `common.mk`

## Known Bugs

### Mascot list refresh is O(n) full rebuild
- **Files:** `ShijimaManager.cc` (lines 477-489)
- **Symptoms:** `refreshListWidget()` clears and rebuilds the entire list widget every time any mascot data changes
- **Trigger:** Any mascot import, reload, or deletion
- **Workaround:** Functional but inefficient with many mascots
- **Fix approach:** Implement incremental refresh using `m_listItemsToRefresh` set (already declared in `ShijimaManager.hpp` line 117 but appears unused)

### `sendJson` accesses QJsonDocument byte array without emptiness check
- **Files:** `ShijimaHttpApi.cc` (lines 113-117)
- **Issue:** `sendJson` does `&bytes[0]` without checking if `bytes` is empty. If `doc.toJson(QJsonDocument::Compact)` ever returns an empty array (edge case with empty documents), this would be undefined behavior
- **Impact:** Low risk with current usage, but fragile if `sendJson` is called with edge-case documents
- **Fix approach:** Add `if (bytes.isEmpty()) return;` guard or ensure `doc` never produces empty output

### `jsonForRequest` silently returns empty optional on parse errors
- **Files:** `ShijimaHttpApi.cc` (lines 92-111)
- **Issue:** JSON parse errors are logged to stderr but the caller only sees an empty optional, returning a generic 400 without details
- **Impact:** API clients receive no information about what went wrong with their request body
- **Fix approach:** Return a 400 with the specific parse error message in the JSON response

### ActiveWindowObserver timer may fire after observer state changes
- **Files:** `ShijimaManager.cc` (lines 743-745), `ShijimaManager.hpp` (line 107)
- **Issue:** Timer is started based on `m_windowObserver.tickFrequency()` but there is no corresponding `killTimer()` when the observer's internal state changes or on destruction
- **Impact:** Timer may fire with stale data or after the observed state is invalidated
- **Fix approach:** Stop/restart timer when window observer configuration changes

### `dispatchToMainThread` creates QTimer without cleanup guarantee
- **Files:** `ShijimaManager.cc` (lines 84-93)
- **Issue:** Creates a `QTimer` with `new` and relies on `deleteLater()` in the callback. If the callback throws or the event loop is destroyed before the timer fires, the timer leaks
- **Impact:** Memory leak on error paths
- **Fix approach:** Use `std::unique_ptr<QTimer>` with a custom deleter that calls `deleteLater()`

## Security Considerations

### HTTP API binds to localhost with no authentication
- **Files:** `ShijimaHttpApi.cc` (line 354), `main.cc` (line 43)
- **Risk:** API listens on `127.0.0.1:32456` with no auth, CORS, or rate limiting. Any local process can spawn/dismiss mascots, execute script selectors, and read mascot state
- **Additional risk:** `ShijimaHttpApi.cc` (lines 126-139) — the `selector` query parameter is evaluated as a script expression via `mascot->mascot().script_ctx->eval_bool(selector)`. While limited to boolean evaluation, this is effectively arbitrary code execution within the duktape script context
- **Current mitigation:** Bound to localhost only
- **Recommendations:** Add token-based auth (env var or config), validate/sanitize `selector` input before eval, add request size limits

### std::filesystem::remove_all on user-controlled paths
- **Files:** `ShijimaManager.cc` (lines 264-272)
- **Risk:** The delete action removes mascot directories using `std::filesystem::remove_all`. The path is constructed from `mascotData->path()` which comes from the mascots directory, but path traversal in a `.mascot` directory name could theoretically escape the intended directory
- **Current mitigation:** Path is constructed from known mascots directory
- **Recommendations:** Validate that the resolved path is within `m_mascotsPath` before deletion using `fs::canonical` comparison

### WAYLAND_DISPLAY environment variable is forcibly cleared
- **Files:** `Platform/Linux/Platform.cc` (line 126)
- **Risk:** `setenv("WAYLAND_DISPLAY", "", 1)` modifies the process environment globally, affecting other libraries or Qt plugins that check for Wayland
- **Impact:** Forces X11 backend even on Wayland compositors with XWayland support. May break other Qt Wayland features
- **Recommendations:** Document this behavior; consider using `QT_QPA_PLATFORM=xcb` instead of environment manipulation

## Performance Bottlenecks

### Mascot tick runs synchronously on main thread
- **Files:** `ShijimaManager.cc` (lines 949-1052), `ShijimaWidget.cc` (lines 244-285)
- **Problem:** All mascot ticks run sequentially in a single timer callback. With N mascots, each tick involves: state update, asset loading, sound check, repaint, and inspector update
- **Cause:** Timer fires every 10ms (40ms / 4 subticks). Each mascot's `tick()` is blocking
- **Improvement path:** For large mascot counts, consider batching repaints, using a render thread, or offloading state updates to workers

### Asset loading has no eviction policy
- **Files:** `AssetLoader.cc` (lines 43-63)
- **Problem:** `loadAsset()` loads images into a `QMap` cache that grows unbounded. Images are never evicted unless `unloadAssets()` is explicitly called with a root path
- **Cause:** No LRU or memory-limit policy
- **Improvement path:** Add memory-based cache eviction or LRU with configurable size limit

### `refreshListWidget()` rebuilds entire list on every change
- **Files:** `ShijimaManager.cc` (lines 477-489)
- **Problem:** Every mascot data change triggers `m_listWidget.clear()` followed by re-adding all items. Causes visible flicker and is O(n)
- **Improvement path:** Use `m_listItemsToRefresh` (already declared at `ShijimaManager.hpp` line 117) for incremental updates

### `onTickSync` blocks calling thread until next tick
- **Files:** `ShijimaManager.cc` (lines 638-643)
- **Problem:** HTTP API handlers call `onTickSync()` which acquires a mutex and waits on a condition variable for the next timer tick. This means every HTTP request blocks for up to 10ms
- **Impact:** API latency is bounded by tick frequency. Concurrent requests queue behind the mutex
- **Improvement path:** Consider a separate read path for non-mutating API endpoints that doesn't require the tick lock

## Fragile Areas

### `ShijimaManager::tick()` iterates and mutates `m_mascots` during iteration
- **Files:** `ShijimaManager.cc` (lines 991-1042)
- **Why fragile:** The tick loop iterates `m_mascots` in reverse, deleting widgets and erasing from the list. Breeding logic adds new widgets to the same list during iteration
- **Safe modification:** The reverse iteration + `erasePos` pattern is correct for deletion, but adding new mascots via breeding during the same loop is risky if the container is ever changed from `std::list` to something with iterator invalidation on insert
- **Test coverage:** None

### `setWindowedMode` recreates all mascot widgets
- **Files:** `ShijimaManager.cc` (lines 645-696)
- **Why fragile:** Toggling windowed mode closes all existing mascot widgets, creates new ones with copied state, and re-parents them. Any state not explicitly copied is lost
- **Safe modification:** The copy constructor `ShijimaWidget(ShijimaWidget &old, ...)` is used, but inspector state is the only thing explicitly preserved (`inspectorWasVisible`)
- **Test coverage:** None

### Linux platform code uses global variables for signal handler communication
- **Files:** `Platform/Linux/Platform.cc` (lines 40-41, 110-127)
- **Why fragile:** `terminateServerFd` and `terminateClientFd` are global variables. `windowMasksEnabled` is a global bool modified by `KWin::running()`. Signal handlers use these globals
- **Safe modification:** Any code that calls `KWin::running()` before `initialize()` will see incorrect `windowMasksEnabled` state
- **Test coverage:** None

### Matrix sync loop creates new HTTP client per iteration
- **Files:** `MatrixClient.cc` (lines 115, 142)
- **Why fragile:** `Client cli(m_homeserver.toStdString())` is created inside `sendMessage()` and `syncLoop()`. Each invocation creates a new TCP connection instead of reusing a connection pool
- **Safe modification:** Create the client once and reuse it, or use a connection pool
- **Test coverage:** None

## Scaling Limits

### Mascot count limited by main-thread tick budget
- **Current capacity:** ~10-20 mascots before visible stutter (estimated, not measured)
- **Limit:** Single-threaded tick at 10ms intervals. Each mascot requires: state update, asset lookup, sound check, repaint, and inspector sync
- **Scaling path:** Offload state updates to a worker thread; batch repaints; use dirty-flag rendering

### HTTP API has no request queuing or concurrency control
- **Current capacity:** Serial request processing (one at a time due to mutex in `onTickSync`)
- **Limit:** `onTickSync` serializes all requests. Under load, requests queue indefinitely
- **Scaling path:** Add request timeout, queue size limit, or read-write lock separation

## Missing Critical Features

### No error reporting or crash collection
- **Problem:** Errors are logged to stderr only (`std::cerr`, `std::cout`). In GUI mode, stderr may not be visible to users
- **Blocks:** Debugging user-reported issues, especially on Windows where console output is hidden
- **Priority:** High — add a log file in the app data directory with rotation

### No configuration for HTTP API port
- **Problem:** Port 32456 is hardcoded in `main.cc` and `ShijimaManager.cc`
- **Blocks:** Running multiple instances (though single-instance check exists), or using a different port if 32456 is occupied
- **Priority:** Medium — make port configurable via environment variable or command-line argument

### No graceful shutdown on SIGINT/SIGTERM (Linux)
- **Problem:** Signal handler in `Platform/Linux/Platform.cc` triggers cleanup via socket but bypasses `ShijimaManager::finalize()` and `AssetLoader::finalize()` cleanup
- **Blocks:** Clean resource release on signal-based termination
- **Priority:** Medium — call finalize functions in the signal handler or use `QCoreApplication::aboutToQuit`

### No test coverage for any code path
- **What's missing:** Entire codebase has zero tests
- **Files:** All `.cc` and `.hpp` files
- **Risk:** Any change could introduce regressions that go undetected until runtime
- **Priority:** High — start with unit tests for:
  - `ShijimaHttpApi` endpoint handlers (mock `ShijimaManager`)
  - `AssetLoader` caching behavior
  - `MascotData` parsing
  - `SoundEffectManager` play/stop lifecycle
  - CLI argument parsing (`cli.cc`)

---

*Concerns audit: 2026-04-26*
