# Codebase Structure

**Analysis Date:** 2026-04-26

## Directory Layout

```
Shijima-claw/
├── main.cc                          # Dual-mode entry point (CLI vs GUI)
├── cli.cc / cli.hpp                 # CLI command implementation (HTTP client)
├── MatrixClient.cc / .hh / .moc     # Matrix protocol integration (2026-04-25)
├── ShijimaManager.cc / .hpp         # Central manager (1121 lines - largest file)
├── ShijimaWidget.cc / .hpp          # Mascot rendering widget
├── ShijimaHttpApi.cc / .hpp         # HTTP REST API server
├── Asset.cc / .hpp                  # Image frame container
├── AssetLoader.cc / .hpp            # Asset loading and caching
├── MascotData.cc / .hpp             # Mascot metadata container
├── SoundEffectManager.cc / .hpp     # Sound effect pooling
├── PlatformWidget.hpp               # Template base for platform-aware widgets
├── ShijimaContextMenu.cc / .hpp     # Right-click context menu for mascots
├── ShimejiInspectorDialog.cc / .hpp # Debug inspector dialog
├── ShijimaLicensesDialog.cc / .hpp  # License viewer dialog
├── ForcedProgressDialog.cc / .hpp   # Non-cancelable progress dialog
├── resources.rc                     # Windows resource file (icon)
├── DefaultMascot.cc / .hpp          # Generated at build time from DefaultMascot/
├── licenses_generated.hpp           # Generated at build time from licenses/
├── Makefile                         # Top-level build entry
├── common.mk                        # Shared build configuration
├── bundle-default.sh                # Script to generate DefaultMascot.cc
├── find_dlls.sh                     # Windows DLL discovery script
├── Platform/                        # Platform abstraction layer
│   ├── Platform.hpp                 # Shared platform interface
│   ├── ActiveWindow.hpp             # Active window data class
│   ├── ActiveWindowObserver.hpp     # Active window observer interface
│   ├── Makefile                     # Platform build dispatcher
│   ├── Linux/                       # Linux backend (KWin, GNOME)
│   │   ├── Makefile                 # Linux platform build
│   │   ├── DBus.cc / DBus.hpp       # DBus utility functions
│   │   ├── KWin.cc / KWin.hpp       # KWin observer backend
│   │   ├── KDEWindowObserverBackend.cc/.hpp  # KDE backend
│   │   ├── GNOME.cc / GNOME.hpp     # GNOME backend (active window)
│   │   ├── GNOMEWindowObserverBackend.cc/.hpp  # GNOME shell extension
│   │   ├── ExtensionFile.cc/.hpp    # File type association handling
│   │   ├── ActiveWindowObserver.cc   # Active window polling
│   │   ├── PrivateActiveWindowObserver.cc/.hpp  # Platform-specific observer
│   │   ├── Platform-Linux.hpp       # Linux-specific platform overrides
│   │   ├── kwin_script.js            # KWin scripting interface
│   │   ├── kwin_script.c             # Compiled KWin script (generated)
│   │   ├── gnome_script/             # GNOME shell extension source
│   │   ├── gnome_script.c            # Compiled GNOME script (generated)
│   │   ├── gnome_script.zip          # Packaged GNOME extension
│   │   └── .work/                    # Build workspace for GNOME scripts
│   │       ├── package.json          # Node.js dependencies
│   │       ├── yarn.lock             # Yarn lockfile
│   │       └── node_modules/         # Compiled GNOME extension output
│   ├── macOS/                       # macOS backend (Objective-C++)
│   ├── Windows/                     # Windows backend (Win32)
│   └── Stub/                        # Unsupported platform fallback
├── libshijima/                      # Submodule: mascot behavior engine
├── libshimejifinder/                # Submodule: archive extraction
├── cpp-httplib/                     # Submodule: HTTP server library
├── DefaultMascot/                   # Built-in mascot (46 frames + XML)
│   ├── img/                         # Frame images (shime1.png - shime46.png)
│   ├── behaviors.xml                # Behavior definitions
│   └── actions.xml                  # Action definitions
├── docs/                            # Design documents and specs
│   └── superpowers/                 # Feature specifications
│       ├── specs/2026-04-25-matrix-integration-design.md
│       └── plans/2026-04-25-matrix-integration-plan.md
├── dev-docker/                      # Docker build environment
├── licenses/                        # Dependency license texts
├── Shijima-Qt.app/                  # macOS app bundle template
├── com.pixelomer.ShijimaQt.desktop  # Linux desktop entry
├── com.pixelomer.ShijimaQt.metainfo.xml  # AppStream metadata
├── com.pixelomer.ShijimaQt.png      # Application icon (512x512)
├── shijima-qt.ico                   # Windows application icon
├── HTTP-API.md                      # API documentation
├── README.md                        # Project readme
├── LICENSE                          # GPL-3.0 license
└── .github/workflows/               # CI/CD workflows
    ├── build-debug.yaml
    └── build-release.yaml
```

## Directory Purposes

**Root (`./`):**
- Purpose: All primary application source files
- Contains: `.cc`/`.hpp` pairs for each component, build files, metadata files
- Key files: `main.cc`, `ShijimaManager.cc`, `ShijimaWidget.cc`, `MatrixClient.cc`, `Makefile`

**Platform (`Platform/`):**
- Purpose: OS-specific implementations of window management and active window tracking
- Contains: Shared headers (`Platform.hpp`, `ActiveWindow.hpp`, `ActiveWindowObserver.hpp`) and platform subdirectories
- Key files: `Platform/Platform.hpp` (interface), `Platform/Linux/KWin.cc`, `Platform/Linux/GNOME.cc`

**Platform/Linux (`Platform/Linux/`):**
- Purpose: Linux-specific backends for KDE Plasma 6 and GNOME 46+
- Contains: KWin DBus observer, GNOME shell extension integration, X11 window properties
- Key files: `KDEWindowObserverBackend.cc`, `GNOMEWindowObserverBackend.cc`, `DBus.cc`, `gnome_script/`
- Build: Uses `.c` output from Node.js compilation in `.work/` directory

**Platform/Linux/.work (`Platform/Linux/.work/`):**
- Purpose: Node.js build workspace for GNOME shell extension
- Contains: `package.json`, `yarn.lock`, `node_modules/`
- Build: Runs `yarn install` to compile GNOME extension to `node_modules/`

**Platform/macOS (`Platform/macOS/`):**
- Purpose: macOS-specific window management via Objective-C++
- Contains: `Platform.mm`, `PrivateActiveWindowObserver.mm`, `ActiveWindowObserver.cc`
- Build: Uses `.mm` extension for Obj-C++ files

**Platform/Windows (`Platform/Windows/`):**
- Purpose: Windows-specific window management via Win32 API
- Contains: `Platform.cc`, `PrivateActiveWindowObserver.cc`, `ActiveWindowObserver.cc`

**Platform/Stub (`Platform/Stub/`):**
- Purpose: Fallback implementations for unsupported platforms
- Contains: Minimal stub implementations that return defaults

**libshijima/ (submodule):**
- Purpose: Core mascot behavior engine (state machine, physics, breeding)
- Build: CMake - `libshijima/build/libshijima.a`
- Note: Submodule must be initialized (`git submodule update --init --recursive`)

**libshimejifinder/ (submodule):**
- Purpose: Shimeji archive analysis and extraction (unarr-based)
- Build: CMake - `libshimejifinder/build/libshimejifinder.a`
- Note: Submodule must be initialized

**cpp-httplib/ (submodule):**
- Purpose: Single-header HTTP server/client library
- Include: `#include <httplib.h>`
- Note: Submodule must be initialized

**DefaultMascot/:**
- Purpose: Built-in mascot shipped with the application
- Contains: 46 frame images, `behaviors.xml`, `actions.xml`
- Build: `DefaultMascot.cc` auto-generated by `bundle-default.sh` at build time

**docs/superpowers/:**
- Purpose: Feature specifications and implementation plans
- Contains: Markdown documents for upcoming features (Matrix integration)
- Pattern: `specs/` for design docs, `plans/` for implementation plans

**dev-docker/:**
- Purpose: Docker environment for Windows cross-compilation
- Contains: Dockerfile for Fedora-based MinGW build

**licenses/:**
- Purpose: Text of all dependency licenses
- Build: Concatenated into `licenses_generated.hpp` at build time

**.github/workflows/:**
- Purpose: GitHub Actions CI/CD pipelines
- Contains: `build-debug.yaml` (push/PR), `build-release.yaml` (manual)

## Key File Locations

**Entry Points:**
- `main.cc`: GUI entry (no args) and CLI dispatch (with args)
- `cli.cc`: CLI implementation - `shijimaRunCli()` function

**Configuration:**
- `Makefile`: Top-level build, source list, publish targets
- `common.mk`: Shared build config (C++17, Qt6, platform detection, CONFIG=release|debug)
- `Platform/Makefile`: Platform library build dispatcher

**Core Logic:**
- `ShijimaManager.cc`: Central orchestrator - mascot lifecycle, tick loop, environment updates, import/export, settings
- `ShijimaWidget.cc`: Individual mascot widget - rendering, mouse handling, tick delegation
- `ShijimaHttpApi.cc`: REST API server - route registration, JSON serialization, thread-safe main thread dispatch
- `AssetLoader.cc`: Singleton asset cache - loads and caches images by path
- `MascotData.cc`: Mascot metadata - parses XML, extracts preview, validates

**Matrix Integration (2026-04-25):**
- `MatrixClient.cc`: Matrix protocol client implementation (sync loop, message sending)
- `MatrixClient.hh`: Qt QObject-based Matrix client with signals/slots
- `MatrixClient.moc`: MOC-generated metadata for Qt signals/slots
- Note: Not yet integrated into ShijimaManager; standalone component

**UI Components:**
- `ShijimaContextMenu.cc`: Right-click menu (close, inspect, etc.)
- `ShimejiInspectorDialog.cc`: Debug dialog showing live mascot state
- `ShijimaLicensesDialog.cc`: License viewer (generated from `licenses/`)
- `ForcedProgressDialog.cc`: Non-cancelable progress dialog for imports

**Platform Abstraction:**
- `PlatformWidget.hpp`: Template base class for platform-aware widgets
- `Platform/Platform.hpp`: `Platform::initialize()`, `Platform::showOnAllDesktops()`, `Platform::useWindowMasks()`
- `Platform/ActiveWindow.hpp`: `Platform::ActiveWindow` data class
- `Platform/ActiveWindowObserver.hpp`: `Platform::ActiveWindowObserver` for polling active window

**Testing:**
- No test infrastructure exists in this project

## Naming Conventions

**Files:**
- Source: `.cc` for C++ implementation files
- Headers: `.hpp` for C++ header files (exception: `MatrixClient.hh` uses `.hh`)
- Objective-C++: `.mm` for macOS implementation files
- Resources: `.rc` for Windows resource files
- MOC output: `.moc` for Qt meta-object compiler output
- Headers use `#pragma once` (not `#ifndef` guards)

**Classes:**
- PascalCase: `ShijimaManager`, `ShijimaWidget`, `AssetLoader`, `MascotData`, `MatrixClient`
- Qt subclasses: Inherit from Qt classes (`QMainWindow`, `QWidget`, `QDialog`, `QMenu`)

**Member Variables:**
- `m_` prefix: `m_mascotTimer`, `m_loadedMascots`, `m_httpApi`, `m_sandboxWidget`
- Boolean members: `m_allowClose`, `m_firstShow`, `m_wasVisible`, `m_windowedMode`

**Functions:**
- camelCase: `spawn()`, `killAll()`, `tick()`, `updateEnvironment()`, `loadData()`
- Event overrides: `timerEvent()`, `showEvent()`, `closeEvent()`, `paintEvent()`, `mousePressEvent()`
- Action handlers: `spawnClicked()`, `importAction()`, `deleteAction()`, `quitAction()`

**Namespaces:**
- `Platform::` for platform abstraction functions and classes
- `shijima::` for libshijima library (external submodule)
- `shijima::mascot::` for mascot-specific types in libshijima
- `shimejifinder::` for archive extraction library (external submodule)
- `httplib::` for HTTP library (external submodule)

**Constants/Macros:**
- `SHIJIMAQT_SUBTICK_COUNT` - subtick divisor (4)
- `SHIJIMA_USE_QTMULTIMEDIA` - build flag for Qt Multimedia
- `SHIJIMA_LOGGING_ENABLED` - conditional log level setting

## Where to Add New Code

**New Feature (mascot behavior):**
- Implementation: `libshijima/` (external submodule - modify upstream)
- XML definitions: `DefaultMascot/behaviors.xml`, `DefaultMascot/actions.xml`

**New UI Dialog:**
- Implementation: `NewDialog.cc` + `NewDialog.hpp` in root directory
- Pattern: Inherit from `QDialog`, follow `ShimejiInspectorDialog` pattern

**New HTTP API Endpoint:**
- Implementation: `ShijimaHttpApi.cc` constructor - add route registration
- Pattern: Lambda handler calling `m_manager->onTickSync()` for thread safety

**New Platform Backend:**
- Implementation: `Platform/<NewPlatform>/` directory with `Platform.cc`, `ActiveWindowObserver.cc`, `PrivateActiveWindowObserver.cc`
- Update: `Platform/Makefile` to detect and build new platform
- Update: `common.mk` platform detection logic

**New CLI Command:**
- Implementation: `cli.cc` - add `else if` branch in `cliMain()` with `ArgumentList`

**New Matrix Integration:**
- Implementation: `MatrixClient.cc` already exists; integrate into `ShijimaManager.cc`
- Pattern: Use `QObject::connect()` to bind Matrix signals to manager slots

**New Utility/Helper:**
- Shared helpers: New `.cc`/`.hpp` pair in root directory
- Platform-specific: `Platform/<platform>/` directory

**New Mascot Type:**
- Archive format: `.mascot` directory with `img/`, `sound/`, `behaviors.xml`, `actions.xml`
- Import: Drag-and-drop to manager or `File > Import shimeji...`

## Special Directories

**libshijima/, libshimejifinder/, cpp-httplib/:**
- Purpose: Git submodules for external dependencies
- Generated: No - external repositories
- Committed: Submodule references only (`.gitmodules`)
- Note: Must be initialized before building: `git submodule update --init --recursive`

**DefaultMascot/:**
- Purpose: Built-in mascot shipped with application
- Generated: `DefaultMascot.cc` is auto-generated at build time by `bundle-default.sh`
- Committed: Yes - source images and XML files are committed

**publish/:**
- Purpose: Build output directory (created during build)
- Generated: Yes - by Makefile publish targets
- Committed: No - in `.gitignore`
- Structure: `publish/<Platform>/<CONFIG>/` (e.g., `publish/Linux/release/`)

**licenses/:**
- Purpose: Dependency license texts
- Build: Concatenated into `licenses_generated.hpp` at build time
- Committed: Yes

**docs/superpowers/:**
- Purpose: Feature design documents and implementation plans
- Generated: No - manually authored markdown
- Committed: Yes

**Platform/Linux/.work/:**
- Purpose: Node.js build workspace for GNOME shell extension compilation
- Generated: Yes - `node_modules/` created by `yarn install`
- Committed: No - in `.gitignore`
- Note: Contains `package.json` and `yarn.lock` for reproducibility

**Shijima-Qt.app/:**
- Purpose: macOS app bundle template (Info.plist, structure)
- Committed: Yes - copied and populated during macOS build

**dev-docker/:**
- Purpose: Docker build environment for Windows cross-compilation
- Committed: Yes

---

*Structure analysis: 2026-04-26*