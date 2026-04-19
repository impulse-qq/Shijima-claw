# External Integrations

**Analysis Date:** 2026-04-19

## APIs & External Services

**Local HTTP API:**
- **Shijima-Qt HTTP Server** — REST API on `127.0.0.1:32456`
  - SDK/Client: `cpp-httplib` (submodule, header-only)
  - Auth: None (loopback-only, no authentication)
  - Base URL: `http://127.0.0.1:32456/shijima/api/v1`
  - Endpoints:
    - `GET /mascots` — List active mascots
    - `POST /mascots` — Spawn new mascot
    - `DELETE /mascots` — Dismiss all mascots
    - `GET /mascots/:id` — Get single mascot data
    - `PUT /mascots/:id` — Alter mascot position/behavior
    - `GET /loadedMascots` — List available mascots
    - `GET /loadedMascots/:id` — Get mascot info
    - `GET /loadedMascots/:id/preview.png` — Get mascot preview image
    - `GET /ping` — Health check (used for single-instance detection)
  - Implementation: `ShijimaHttpApi.cc`, `ShijimaHttpApi.hpp`
  - Single-instance enforcement: `main.cc` pings `/ping` before starting; throws if already running

**CLI Interface:**
- **Local CLI commands** via `./shijima-qt` with arguments
  - `list` — List running mascots
  - `spawn <name>` — Spawn mascot by name
  - `alter <id> <x> <y> <behavior>` — Move/change mascot
  - `dismiss <id>` — Dismiss one mascot
  - `dismiss-all` — Dismiss all mascots
  - Implementation: `cli.cc`, `cli.hpp`
  - Communicates via HTTP API to running instance

## Data Storage

**Databases:**
- Not detected — No database usage

**File Storage:**
- **Local filesystem** — Mascot assets stored as directories with image frames and XML configs
  - Default mascot: `DefaultMascot/` (46 PNG frames + `behaviors.xml` + `actions.xml`)
  - User mascots: Imported from `.zip` archives via `libshimejifinder`
  - Settings: `QSettings` (platform-native: `~/.config` on Linux, `~/Library/Preferences` on macOS, registry on Windows)
  - Path: `ShijimaManager::mascotsPath()` — managed via `QStandardPaths`

**Archive Extraction:**
- `libshimejifinder` (submodule) — Extracts `.zip` mascot archives
  - Engine: `unarr` (RAR/7z/ZIP support)
  - Backend: `libarchive` (system package)
  - Mascot import flow: `.zip` → extract → analyze (`shimejifinder::analyze`) → load

**Caching:**
- Not detected — No caching layer

## Authentication & Identity

**Auth Provider:**
- Not applicable — No user authentication

**Permissions:**
- **macOS:** Accessibility permission required for frontmost window detection
- **Linux (GNOME):** Shell extension auto-installed; requires user to enable extensions and restart shell (logout/login) on first run
- **Linux (KDE):** KWin script auto-loaded via DBus; transparent to user

## Monitoring & Observability

**Error Tracking:**
- Not detected — No external error tracking service

**Logging:**
- **libshijima logging** — Conditional compilation via `SHIJIMA_LOGGING_ENABLED`
  - Levels: `SHIJIMA_LOG_PARSER`, `SHIJIMA_LOG_WARNINGS`
  - Set in `main.cc`: `shijima::set_log_level()`
- **Qt debugging** — `QDebug` used in `ShijimaWidget.cc`
- **Standard output** — `std::cout` / `std::cerr` used in `SoundEffectManager.cc`, `ShijimaManager.cc`

## Desktop Environment Integrations

**Linux — KDE Plasma 6:**
- **KWin DBus API** — `org.kde.KWin.Scripting`
  - Script loading: `KWin::loadScript()`, `KWin::runScript()`, `KWin::unloadScript()`
  - Implementation: `Platform/Linux/KWin.cc`, `Platform/Linux/KWin.hpp`
  - Script: `Platform/Linux/kwin_script.js` (embedded as C array)
- **KWin Window Observer** — Tracks active window for mascot interactions
  - Implementation: `Platform/Linux/KDEWindowObserverBackend.cc`

**Linux — GNOME 46/47:**
- **GNOME Shell Extension** — `shijima-helper@pixelomer.github.io`
  - Auto-installed on first run
  - Implementation: `Platform/Linux/GNOME.cc`, `Platform/Linux/GNOME.hpp`
  - Extension files: `Platform/Linux/gnome_script/` (metadata.json + extension.js)
  - Packaged as ZIP, embedded as C array
- **GNOME Window Observer** — Tracks active window via extension
  - Implementation: `Platform/Linux/GNOMEWindowObserverBackend.cc`

**Linux — DBus (General):**
- **Qt DBus** (`Qt6DBus`) — Generic DBus method calls and property access
  - Implementation: `Platform/Linux/DBus.cc`, `Platform/Linux/DBus.hpp`
  - Error types: `DBusReturnError`, `DBusCallError`

**macOS:**
- **AppKit** — Native window management via Objective-C++
  - Implementation: `Platform/macOS/Platform.mm`
- **ApplicationServices** — Accessibility APIs for frontmost window detection
  - Implementation: `Platform/macOS/PrivateActiveWindowObserver.mm`
  - Frameworks: `-framework AppKit -framework ApplicationServices`
  - Memory management: ARC (`-fobjc-arc`)

**Windows:**
- **Win32 API** — Native window tracking
  - Implementation: `Platform/Windows/Platform.cc`, `Platform/Windows/PrivateActiveWindowObserver.cc`
  - Networking: `ws2_32` (Winsock2) for HTTP API

**Unsupported Platforms:**
- **Stub** — Minimal implementations for unsupported platforms
  - Implementation: `Platform/Stub/Platform.cc`, `Platform/Stub/ActiveWindowObserver.cc`

## CI/CD & Deployment

**Hosting:**
- **GitHub Releases** — `https://github.com/pixelomer/Shijima-Qt/releases`

**CI Pipeline:**
- **GitHub Actions** — `/.github/workflows/`
  - `build-debug.yaml` — Debug builds on push/PR
  - `build-release.yaml` — Release builds on manual dispatch
  - Runners: `ubuntu-22.04` (Linux x64), `ubuntu-24.04-arm` (Linux arm64), `macos-14` (macOS arm64), `ubuntu-latest` (Windows via Docker)

**Distribution Formats:**
- **Linux:** Raw binary + `libunarr.so.1`, or AppImage (`Shijima-Qt.AppImage`)
- **macOS:** `.app` bundle (`Shijima-Qt.app`)
- **Windows:** `.exe` + DLLs in `publish/Windows/{debug,release}/`

**Linux Packaging:**
- **AppImage** — Generated via `linuxdeploy` + `linuxdeploy-plugin-qt`
  - Makefile target: `make appimage`
  - Desktop entry: `com.pixelomer.ShijimaQt.desktop`
  - AppStream metadata: `com.pixelomer.ShijimaQt.metainfo.xml`
  - Icon: `com.pixelomer.ShijimaQt.png` (512x512)
- **System Install** — `make install` / `make uninstall`
  - Binary: `$(PREFIX)/bin/shijima-qt`
  - Library: `$(PREFIX)/lib/libunarr.so.1`
  - Desktop entry: `$(PREFIX)/share/applications/`
  - Icon: `$(PREFIX)/share/icons/hicolor/512x512/apps/`

## Environment Configuration

**Required env vars:**
- None required at runtime
- Build-time:
  - `CONFIG` — `release` or `debug`
  - `SHIJIMA_USE_QTMULTIMEDIA` — `1` or `0`
  - `PREFIX` — Install prefix (default `/usr/local`)
  - `bindir` — MinGW bin directory (Windows cross-compile only)

**Secrets location:**
- Not applicable — No secrets, API keys, or credentials

## Webhooks & Callbacks

**Incoming:**
- None — HTTP API is local-only (`127.0.0.1`), no external webhooks

**Outgoing:**
- None — No outbound HTTP calls to external services

## Submodule Dependencies

| Submodule | URL | Purpose | Build Output |
|-----------|-----|---------|-------------|
| `libshijima/` | `git@github.com:pixelomer/libshijima` | Mascot behavior engine (Duktape scripting, XML parsing) | `libshijima/build/libshijima.a` |
| `libshimejifinder/` | `git@github.com:pixelomer/libshimejifinder` | Archive extraction (unarr + libarchive) | `libshimejifinder/build/libshimejifinder.a` |
| `cpp-httplib/` | `https://github.com/yhirose/cpp-httplib` | HTTP server/client (header-only) | N/A (header-only) |

**Note:** Submodules are currently empty (not initialized). Run `git submodule update --init --recursive` before building.

---

*Integration audit: 2026-04-19*
