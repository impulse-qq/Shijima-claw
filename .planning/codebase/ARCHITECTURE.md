# Architecture

**Analysis Date:** 2026/04/26

## Pattern Overview

**Overall:** Qt-based desktop mascot simulation with layered platform abstraction and event-driven game loop.

**Key Characteristics:**
- Singleton manager controlling mascot lifecycle (ShijimaManager::defaultManager)
- Factory pattern for mascot template registration and spawning (shijima::mascot::factory)
- Observer pattern for active window tracking (Platform::ActiveWindowObserver)
- Qt signal/slot for UI events and inter-component communication
- Platform abstraction layer for cross-desktop-environment support (GNOME/KDE/X11)
- Timer-based game loop at 40ms tick intervals subdivided into 4 subticks
- Dual-mode entry: CLI mode when arguments provided, GUI mode otherwise

## Layers

**Application Layer (Manager):**
- Purpose: Central coordinator for all mascots, UI state, and system integration
- Location: `ShijimaManager.cc` (1121 lines)
- Contains: Mascot lifecycle management, tick loop, environment updates, import/export, settings persistence
- Depends on: Qt, libshijima, libshimejifinder, Platform abstraction
- Used by: main.cc entry point

**Presentation Layer (Widgets):**
- Purpose: Visual rendering of mascots, UI dialogs, context menus
- Location: `ShijimaWidget.cc`, `ShijimaContextMenu.cc`, `ShimejiInspectorDialog.cc`, `ShijimaLicensesDialog.cc`, `ForcedProgressDialog.cc`
- Contains: Qt widgets, paint events, mouse handling, rendering pipeline
- Depends on: Asset layer, Platform layer, libshijima
- Used by: ShijimaManager

**Asset Management Layer:**
- Purpose: Image loading/caching and mascot metadata
- Location: `AssetLoader.cc`, `Asset.cc`, `SoundEffectManager.cc`, `MascotData.cc`
- Contains: Image/mask loading, sound effect pooling, mascot metadata parsing, XML extraction
- Depends on: Qt image/audio subsystems, libshimejifinder (archive extraction)
- Used by: ShijimaWidget, ShijimaManager

**HTTP API Layer:**
- Purpose: REST API for external control of mascots
- Location: `ShijimaHttpApi.cc`
- Contains: cpp-httplib server on port 32456, JSON request/response handling
- Depends on: ShijimaManager, Qt JSON, cpp-httplib
- Used by: CLI client, external automation tools

**Matrix Protocol Layer:**
- Purpose: Matrix messaging integration for receiving and sending messages
- Location: `MatrixClient.cc`, `MatrixClient.hh`
- Contains: MatrixClient (QObject subclass), sync loop in dedicated thread, config from ~/.config/shijima-qt/matrix.json
- Depends on: Qt Network, Qt Core (QObject, signals)
- Used by: ShijimaManager (connects messageReceived signal to mascot bubble display)

**Platform Abstraction Layer:**
- Purpose: OS-specific window management, active window tracking, desktop integration
- Location: `Platform/` (Linux/, macOS/, Windows/, Stub/)
- Contains: Platform::initialize(), Platform::showOnAllDesktops(), ActiveWindowObserver, ActiveWindow
- Depends on: OS APIs (X11/DBus/KWin on Linux, AppKit on macOS, Win32 on Windows)
- Used by: All widget classes via PlatformWidget<T>, ShijimaManager

**Core Mascot Engine (libshijima submodule):**
- Purpose: Mascot behavior, animation state machine, physics, breeding
- Location: `libshijima/shijima/mascot/` (factory.hpp/cc, manager.hpp/cc, environment.hpp, state.hpp/cc)
- Contains: shijima::mascot::factory (template registry), shijima::mascot::manager (per-mascot state), shijima::mascot::environment (screen/workspace data)
- Depends on: rapidxml (parsing), duktape (scripting)
- Used by: ShijimaManager (factory), ShijimaWidget (manager)

**Archive Extraction (libshimejifinder submodule):**
- Purpose: Shimeji archive analysis and extraction
- Location: `libshimejifinder/` (shimejifinder::analyze())
- Contains: unarr-based archive extraction (zip, rar, 7z support via libarchive)
- Used by: ShijimaManager (import)

## Data Flow

**Mascot Spawn Flow:**
1. User double-clicks mascot in manager list, calls spawn() via API/CLI, or Matrix message triggers bubble
2. ShijimaManager::spawn() calls m_factory.spawn(name) from libshijima
3. Factory returns shijima::mascot::manager with initialized state
4. ShijimaWidget created with mascot manager, MascotData, and parent widget
5. Widget added to m_mascots list and m_mascotsById map
6. Widget show() triggers PlatformWidget::showEvent which calls Platform::showOnAllDesktops
7. Timer tick loop begins calling ShijimaWidget::tick() every 40ms

**Game Loop (Tick) Flow:**
1. ShijimaManager::timerEvent() fires every 10ms (40ms / 4 subticks via SHIJIMAQT_SUBTICK_COUNT)
2. ShijimaManager::tick() processes tick callbacks (thread-safe API operations via mutex + condition variable)
3. updateEnvironment() updates per-screen geometry, cursor position, active window bounds
4. For each mascot in m_mascots:
   - Skip if not visible, call tick(), advance mascot animation state
   - Check for breed requests, spawn new mascot via factory if available
   - Check for death state, mark for deletion
5. Remove deleted mascots from lists
6. If no mascots remain, show manager window

**API Request Flow:**
1. HTTP request received on 127.0.0.1:32456 (cpp-httplib server thread)
2. Handler calls ShijimaManager::onTickSync() with callback
3. onTickSync() acquires mutex, queues callback, waits on condition variable
4. Next game loop tick executes callback on main thread
5. Callback completes, condition variable notified, HTTP thread resumes
6. JSON response sent back to client

**Matrix Message Flow:**
1. MatrixClient::syncLoop() polls homeserver via /sync API in dedicated thread
2. New messages emit messageReceived signal
3. ShijimaManager connects this signal to first mascot's showMessageBubble()
4. Shortcut Ctrl+Shift+M triggers showMatrixSendDialog() for sending

## Key Abstractions

**ShijimaManager (Singleton):**
- Purpose: Central coordinator for all mascots and UI state
- Examples: ShijimaManager::defaultManager() always returns same instance
- Pattern: Singleton with lazy initialization

**shijima::mascot::factory:**
- Purpose: Registry and factory for mascot templates
- Examples: m_factory.register_template(), m_factory.spawn()
- Pattern: Factory with template registry

**ShijimaWidget (Per-mascot window):**
- Purpose: Individual mascot rendering and interaction
- Examples: One ShijimaWidget per active mascot
- Pattern: Qt widget subclass wrapping mascot::manager

**Platform::ActiveWindowObserver:**
- Purpose: Track which window has focus across desktop environments
- Examples: m_windowObserver.tick(), getActiveWindow()
- Pattern: Observer with platform-specific backends (DBus/GNOME, KWin/KDE)

**PlatformWidget<T> (CRTP Template):**
- Purpose: Add platform-specific show behavior to any Qt widget type
- Examples: ShijimaManager : PlatformWidget<QMainWindow>, ShijimaWidget : PlatformWidget<QWidget>
- Pattern: Template inheritance with flag-based configuration (ShowOnAllDesktops)
- File: PlatformWidget.hpp

**MascotData:**
- Purpose: Cached metadata for loaded mascot templates
- Examples: behaviorsXML(), actionsXML(), preview() icon
- Pattern: Value object with lazy loading

**MatrixClient (QObject):**
- Purpose: Matrix protocol client for messaging
- Examples: loadConfig(), login(), startSyncLoop(), sendMessage()
- Pattern: QObject with signals, runs sync loop in separate thread
- Config: ~/.config/shijima-qt/matrix.json (homeserver, userId, accessToken, roomId)

## Entry Points

**main():**
- Location: main.cc:30
- Triggers: Application launch (argc == 1) or CLI mode (argc > 1)
- Responsibilities: QApplication setup, single-instance check (pings 127.0.0.1:32456), manager initialization, Qt event loop

**shijimaRunCli():**
- Location: cli.cc
- Triggers: CLI invocation (./shijima-qt <command>)
- Responsibilities: Command-line interface for mascot spawning, list, alter, dismiss operations

**ShijimaHttpApi server:**
- Location: ShijimaHttpApi.cc
- Triggers: HTTP requests on 127.0.0.1:32456
- Responsibilities: CRUD operations for mascots, loaded mascot listing, preview image serving

## Error Handling

**Strategy:** Exception-based with top-level catch in main.cc, graceful degradation for platform features.

**Patterns:**
- Startup failure: main.cc catches std::exception, shows QMessageBox with error message
- Invalid mascot data: loadData() throws std::runtime_error for invalid data
- Import failure: import() catches exceptions, returns empty set, logs to stderr
- API errors: Return HTTP 400/404 with JSON error object
- Platform feature unavailability: Stub implementations return defaults

## Cross-Cutting Concerns

**Logging:** shijima::set_log_level() from libshijima; conditional on SHIJIMA_LOGGING_ENABLED macro; stdout/stderr for operational messages.

**Validation:** Mascot XML validation via libshijima factory; MascotData::valid() checks parse success.

**Authentication:** Matrix access token loaded from ~/.config/shijima-qt/matrix.json (local file only)

**Threading:** HTTP server and Matrix sync loop run in dedicated std::thread instances; all mascot operations marshaled to main thread via onTickSync() mutex + condition variable pattern.

**Settings persistence:** QSettings("pixelomer", "Shijima-Qt") for user preferences (scale, multiplication, windowed mode background).

---

*Architecture analysis: 2026/04/26*
