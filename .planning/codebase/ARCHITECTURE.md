# Architecture

**Analysis Date:** 2026-04-19

## Pattern Overview

**Overall:** Event-driven desktop application with timer-based game loop, layered architecture, and platform abstraction via strategy pattern.

**Key Characteristics:**
- **Dual-mode entry** — `main.cc` branches on `argc`: CLI mode (HTTP client) if args provided, GUI mode (QApplication + HTTP server) otherwise
- **Singleton managers** — `ShijimaManager::defaultManager()` and `AssetLoader::defaultLoader()` provide global access points
- **Timer-driven game loop** — 40ms tick interval subdivided into 4 subticks (`SHIJIMAQT_SUBTICK_COUNT = 4`)
- **Platform abstraction layer** — `Platform/` directory mirrors OS support (Linux/macOS/Windows/Stub) with shared interfaces and platform-specific implementations
- **Template-based widget base** — `PlatformWidget<T>` is a CRTP-style template that wraps any Qt widget type with platform-specific show behavior
- **Single-instance enforcement** — Pings `127.0.0.1:32456` at startup; throws if already running

## Layers

**Application Layer:**
- Purpose: Entry points, CLI commands, main window management
- Location: `main.cc`, `cli.cc`, `ShijimaManager.cc`
- Contains: Application lifecycle, command routing, mascot orchestration
- Depends on: All lower layers
- Used by: End user (GUI) and shell scripts (CLI)

**Presentation Layer:**
- Purpose: Visual rendering of mascots, UI dialogs, context menus
- Location: `ShijimaWidget.cc`, `ShijimaContextMenu.cc`, `ShimejiInspectorDialog.cc`, `ShijimaLicensesDialog.cc`, `ForcedProgressDialog.cc`
- Contains: Qt widgets, paint events, mouse handling, rendering pipeline
- Depends on: Asset layer, Platform layer, libshijima
- Used by: ShijimaManager

**Asset Layer:**
- Purpose: Image loading, caching, sound management
- Location: `AssetLoader.cc`, `Asset.cc`, `SoundEffectManager.cc`, `MascotData.cc`
- Contains: Image/mask loading, sound effect pooling, mascot metadata parsing
- Depends on: Qt image/audio subsystems, libshimejifinder
- Used by: ShijimaWidget, ShijimaManager

**HTTP API Layer:**
- Purpose: REST API for external control of mascots
- Location: `ShijimaHttpApi.cc`
- Contains: cpp-httplib server, JSON request/response handling, selector evaluation
- Depends on: ShijimaManager, Qt JSON, libshijima (script context)
- Used by: CLI client, external automation tools

**Platform Abstraction Layer:**
- Purpose: OS-specific window management, active window tracking, desktop integration
- Location: `Platform/` (Linux/, macOS/, Windows/, Stub/)
- Contains: `Platform::initialize()`, `Platform::showOnAllDesktops()`, `ActiveWindowObserver`, `ActiveWindow`
- Depends on: OS APIs (X11/DBus/KWin on Linux, AppKit on macOS, Win32 on Windows)
- Used by: All widget classes via `PlatformWidget<T>`, ShijimaManager for window tracking

**Behavior Engine (External):**
- Purpose: Mascot AI, behavior state machine, physics, breeding
- Location: `libshijima/` (submodule, CMake build)
- Contains: `shijima::mascot::manager`, `shijima::mascot::factory`, `shijima::mascot::environment`
- Depends on: None (pure C++ library)
- Used by: ShijimaManager (factory), ShijimaWidget (manager)

**Archive Extraction (External):**
- Purpose: Shimeji archive analysis and extraction
- Location: `libshimejifinder/` (submodule, CMake build)
- Contains: `shimejifinder::analyze()`, `shimejifinder::utils`
- Depends on: libarchive, unarr
- Used by: ShijimaManager (import), ShijimaWidget (asset path resolution)

## Data Flow

**Mascot Spawn Flow:**

1. User double-clicks mascot in manager list or calls `spawn()` via API/CLI
2. `ShijimaManager::spawn()` calls `m_factory.spawn(name)` from libshijima
3. Factory returns a `shijima::mascot::manager` with initialized state
4. `ShijimaWidget` is created with the mascot manager, MascotData, and parent
5. Widget is added to `m_mascots` list and `m_mascotsById` map
6. Widget calls `show()` — `PlatformWidget<T>::showEvent` triggers `showOnAllDesktops`
7. Timer tick loop begins calling `ShijimaWidget::tick()` every 40ms

**Game Loop (Tick) Flow:**

1. `ShijimaManager::timerEvent()` fires every 10ms (40ms / 4 subticks)
2. `ShijimaManager::tick()` processes tick callbacks (for thread-safe API operations)
3. `updateEnvironment()` updates screen geometry, cursor position, active window bounds
4. For each mascot in `m_mascots`:
   - Skip if not visible
   - Call `ShijimaWidget::tick()`
   - `ShijimaWidget::tick()` calls `m_mascot->tick()` (libshijima behavior engine)
   - Check for frame/sound changes → repaint if needed
   - Check for breed requests → spawn new mascot if available
   - Check for death state → mark for deletion
5. Remove deleted mascots from lists
6. If no mascots remain, show manager window

**API Request Flow:**

1. HTTP request received on `127.0.0.1:32456` (cpp-httplib server thread)
2. Handler calls `ShijimaManager::onTickSync()` with callback
3. `onTickSync()` acquires mutex, queues callback, waits on condition variable
4. Next game loop tick executes callback on main thread
5. Callback completes, condition variable notified, HTTP thread resumes
6. JSON response sent back to client

**State Management:**
- Mascot state held in `shijima::mascot::manager::state` (libshijima)
- Environment state (`shijima::mascot::environment`) updated each tick with screen geometry, cursor, active window
- Per-screen environments stored in `QMap<QScreen *, shared_ptr<environment>>`
- Window observer polls active window via platform-specific backends

## Key Abstractions

**PlatformWidget<T>:**
- Purpose: Template base class that adds platform-specific show behavior to any Qt widget
- Examples: `ShijimaManager : PlatformWidget<QMainWindow>`, `ShijimaWidget : PlatformWidget<QWidget>`
- Pattern: Template inheritance with flag-based configuration (`ShowOnAllDesktops`)
- File: `PlatformWidget.hpp`

**ActiveWindowObserver:**
- Purpose: Cross-platform active window detection
- Examples: `KDEWindowObserverBackend`, `GNOMEWindowObserverBackend` (Linux), `PrivateActiveWindowObserver` (macOS/Windows)
- Pattern: PIMPL with `PrivateActiveWindowObserver` holding platform-specific state
- File: `Platform/ActiveWindowObserver.hpp`

**ActiveWindow:**
- Purpose: Data class representing the currently focused window
- Examples: UID, PID, geometry (x, y, width, height)
- Pattern: Simple value class with `available` flag for "no window" state
- File: `Platform/ActiveWindow.hpp`

**MascotData:**
- Purpose: Metadata container for a loaded mascot (XML paths, preview, validity)
- Examples: `behaviorsXML()`, `actionsXML()`, `imgRoot()`, `preview()`
- Pattern: Immutable data object loaded once, cached, optionally reloadable
- File: `MascotData.hpp`

**Asset:**
- Purpose: Loaded image frame with offset, size, and optional mask (Linux)
- Examples: Normal and mirrored variants, window mask for non-translucent backgrounds
- Pattern: Lazy-loaded via `AssetLoader`, cached by path
- File: `Asset.hpp`

## Entry Points

**GUI Entry (`main.cc`):**
- Location: `main.cc:30`
- Triggers: `./shijima-qt` (no arguments)
- Responsibilities: Initialize platform, create QApplication, ping for single-instance, show ShijimaManager, start event loop

**CLI Entry (`cli.cc`):**
- Location: `cli.cc:642` (`shijimaRunCli`)
- Triggers: `./shijima-qt <command> [options]`
- Responsibilities: Parse command (list, spawn, alter, dismiss, dismiss-all, list-loaded), make HTTP requests to running instance, format output

**HTTP API (`ShijimaHttpApi.cc`):**
- Location: `ShijimaHttpApi.cc:142` (constructor registers routes)
- Triggers: HTTP requests on `127.0.0.1:32456`
- Responsibilities: CRUD operations for mascots, loaded mascot listing, ping endpoint, preview image serving
- Routes:
  - `GET /shijima/api/v1/mascots` — List active mascots (supports `?selector=` filter)
  - `POST /shijima/api/v1/mascots` — Spawn new mascot
  - `GET /shijima/api/v1/mascots/:id` — Get mascot by ID
  - `PUT /shijima/api/v1/mascots/:id` — Update mascot position/behavior
  - `DELETE /shijima/api/v1/mascots/:id` — Dismiss mascot
  - `DELETE /shijima/api/v1/mascots` — Dismiss all (supports selector)
  - `GET /shijima/api/v1/loadedMascots` — List loaded mascot definitions
  - `GET /shijima/api/v1/loadedMascots/:id` — Get loaded mascot by ID
  - `GET /shijima/api/v1/loadedMascots/:id/preview.png` — Get mascot preview image
  - `GET /shijima/api/v1/ping` — Health check / single-instance probe

## Error Handling

**Strategy:** Exception-based with top-level catch in `main.cc`, graceful degradation for platform features.

**Patterns:**
- **Startup failure** — `main.cc` catches `std::exception`, shows QMessageBox with error message
- **Invalid mascot data** — `loadData()` throws `std::runtime_error` for invalid data
- **Import failure** — `import()` catches exceptions, returns empty set, logs to stderr
- **API errors** — Return HTTP 400/404 with JSON error object
- **Platform feature unavailability** — Stub implementations return defaults (e.g., `ActiveWindow::available = false`)
- **Signal handling (Linux)** — SIGINT/SIGTERM/SIGHUP handled via socket pair for clean Qt event loop shutdown

## Cross-Cutting Concerns

**Logging:** `shijima::set_log_level()` from libshijima; conditional on `SHIJIMA_LOGGING_ENABLED` macro. stdout/stderr used for operational messages.

**Validation:** Mascot XML validation via libshijima factory; `MascotData::valid()` checks parse success.

**Authentication:** None — HTTP API binds to `127.0.0.1` only (loopback).

**Threading:** HTTP server runs in dedicated `std::thread`; all mascot operations marshaled to main thread via `onTickSync()` mutex + condition variable pattern.

**Settings persistence:** `QSettings("pixelomer", "Shijima-Qt")` for user preferences (scale, multiplication, windowed mode background).

---

*Architecture analysis: 2026-04-19*