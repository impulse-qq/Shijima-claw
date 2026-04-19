# Technology Stack

**Analysis Date:** 2026-04-19

## Languages

**Primary:**
- C++17 - All application source code (`*.cc`, `*.hpp`)
- Objective-C++ - macOS platform backend (`Platform/macOS/*.mm`)

**Secondary:**
- JavaScript - KWin script for KDE Plasma window tracking (`Platform/Linux/kwin_script.js`)
- JavaScript - GNOME Shell extension (`Platform/Linux/gnome_script/extension.js`)
- Bash - Build scripts (`bundle-default.sh`)
- XML - Mascot behavior/action definitions (`DefaultMascot/behaviors.xml`, `DefaultMascot/actions.xml`)
- JSON - GNOME extension metadata (`Platform/Linux/gnome_script/metadata.json`)

## Runtime

**Environment:**
- Native compiled binary (no runtime interpreter)
- Qt6 application framework (GUI event loop via `QApplication`)

**Package Manager:**
- Not applicable — no package manifest (no `package.json`, `Cargo.toml`, etc.)
- Dependencies installed via system package managers:
  - **macOS:** MacPorts (`sudo port install qt6-qtbase qt6-qtmultimedia pkgconfig libarchive`)
  - **Linux:** apt (`libarchive-dev`, `libxcb-cursor0`, `libwayland-dev`, `wayland-protocols`)
  - **Windows:** MinGW64 via Docker/Fedora (see `dev-docker/Dockerfile`)

**Build System:**
- GNU Make (`Makefile`, `common.mk`, `Platform/Makefile`)
- CMake (for submodules: `libshijima/`, `libshimejifinder/`)

## Frameworks

**Core:**
- Qt6 (version 6.8.2 in CI) - GUI framework, cross-platform abstraction
  - `Qt6Widgets` - Main UI components (`QMainWindow`, `QWidget`, `QDialog`)
  - `Qt6Core` - Core utilities (`QString`, `QMap`, `QSettings`, `QTimer`)
  - `Qt6Gui` - Painting, events (`QPainter`, `QMouseEvent`, `QImage`)
  - `Qt6Concurrent` - Threading support
  - `Qt6Multimedia` - Sound effects via `QSoundEffect` (optional, `SHIJIMA_USE_QTMULTIMEDIA` flag)
  - `Qt6DBus` - Linux DBus integration (KWin/GNOME communication)

**HTTP Server:**
- cpp-httplib (header-only, via submodule `cpp-httplib/`) - Embedded HTTP server on `127.0.0.1:32456`

**Testing:**
- Not detected — zero test infrastructure

**Build/Dev:**
- `pkg-config` - Qt6 and libarchive discovery on Linux
- `qmake` - Qt6 build configuration resolution
- `cmake` - Submodule builds (`libshijima/`, `libshimejifinder/`)
- `strip` - Binary size optimization in release builds
- `hexdump` / `sed` - Asset bundling (`bundle-default.sh`)
- `yarn` / `minify` - GNOME extension JS minification (Linux only)
- `zip` - GNOME extension packaging (Linux only)
- `linuxdeploy` + `linuxdeploy-plugin-qt` - Linux AppImage generation
- `macdeployqt` - macOS `.app` bundle creation

## Key Dependencies

**Critical:**
- `libshijima` (git submodule, `git@github.com:pixelomer/libshijima`) - Mascot behavior engine, CMake build, produces `libshijima.a`. Provides `shijima::mascot::factory`, `shijima::mascot::manager`, `shijima::mascot::environment`, `shijima::log`. Uses Duktape for scripting.
- `libshimejifinder` (git submodule, `git@github.com:pixelomer/libshimejifinder`) - Archive extraction for mascot .zip files. CMake build, produces `libshimejifinder.a`. Depends on `unarr` and `libarchive`.
- `cpp-httplib` (git submodule, `https://github.com/yhirose/cpp-httplib`) - Header-only HTTP server/client library. Provides `httplib::Server` and `httplib::Client`.
- `libarchive` (system package, `pkg-config: libarchive`) - Archive format support for mascot import.
- `unarr` (bundled via `libshimejifinder/`) - RAR/7z/ZIP extraction engine. Produces `libunarr.so.1` / `libunarr.1.dylib` / `libunarr.dll`.

**Infrastructure:**
- `X11` (Linux only, `pkg-config: x11`) - X11 display server support
- `DBus` (Linux only, `Qt6DBus`) - Inter-process communication for KDE/GNOME integration
- `AppKit` (macOS only, `-framework AppKit`) - Native macOS window management
- `ApplicationServices` (macOS only, `-framework ApplicationServices`) - macOS accessibility APIs
- `ws2_32` (Windows only) - Winsock2 networking for HTTP API
- `libobjc` (macOS only, `-lobjc`) - Objective-C runtime

**Transitive (via submodules):**
- `Duktape` - Embedded JavaScript engine used by libshijima for behavior scripting
- `RapidXML` - XML parser used by libshijima for `behaviors.xml` / `actions.xml`

## Configuration

**Environment:**
- No `.env` file or environment variable configuration required
- Build-time configuration via Makefile variables:
  - `CONFIG` — `release` (default, `-O3 -flto`) or `debug` (`-g -O0`)
  - `SHIJIMA_USE_QTMULTIMEDIA` — `1` (default, enables sound) or `0`
  - `PREFIX` — `/usr/local` (install path, `make install`)
  - `QMAKE` — `qmake` (Qt6) or `qmake-qt5` (Qt5 override)

**Build:**
- `common.mk` — Shared build configuration (C++17, Qt6, platform detection, config modes)
- `Makefile` — Top-level build rules, source list, publish targets
- `Platform/Makefile` — Platform library build dispatcher
- `Platform/{Linux,macOS,Windows,Stub}/Makefile` — Per-platform build rules
- `resources.rc` — Windows resource file (icon, metadata)
- `com.pixelomer.ShijimaQt.desktop` — Linux desktop entry
- `com.pixelomer.ShijimaQt.metainfo.xml` — Linux AppStream metadata

**Generated Sources:**
- `DefaultMascot.cc` — Auto-generated by `bundle-default.sh` from `DefaultMascot/` files (46 PNG frames + XML)
- `licenses_generated.hpp` — Auto-generated from `licenses/` directory for in-app license display
- `kwin_script.c` / `gnome_script.c` — Auto-generated C arrays from minified JS/ZIP (Linux only, `bin2c`)

**Build Modes:**
| Mode | Flags | CMake |
|------|-------|-------|
| `release` | `-O3 -flto -DNDEBUG` | `-DCMAKE_BUILD_TYPE=Release` |
| `debug` | `-g -O0` | `-DCMAKE_BUILD_TYPE=Debug` |

## Platform Requirements

**Development:**
- **Linux:** Qt6 (Widgets, Core, Gui, Concurrent, DBus, Multimedia), libarchive-dev, libxcb-cursor0, libwayland-dev, wayland-protocols, pkg-config, make, cmake, gcc/g++
- **macOS:** Xcode command-line tools, MacPorts with qt6-qtbase, qt6-qtmultimedia, pkgconfig, libarchive
- **Windows:** MinGW64 toolchain (via Docker), Fedora 42 base with mingw64-qt6-qtbase, mingw64-qt6-qtmultimedia, mingw64-gcc, mingw64-libarchive

**Production:**
- **Linux:** x86_64 or arm64. Distributed as raw binary + `libunarr.so.1` or as AppImage (`Shijima-Qt.AppImage`)
- **macOS:** arm64 (Apple Silicon). Distributed as `.app` bundle via `macdeployqt`
- **Windows:** x86_64. Distributed as `.exe` + DLLs (Qt6, libunarr) via `find_dlls.sh`
- **GNOME:** Shell extension auto-installed on first run; requires logout/login on GNOME 46/47
- **KDE:** KWin script auto-installed; transparent to user on Plasma 6

## CI/CD

**GitHub Actions:**
| Workflow | Trigger | Platforms | Retention |
|----------|---------|-----------|-----------|
| `build-debug.yaml` | Push to main + PRs | Linux (x64/arm64), macOS (arm64), Windows (x64) | 1 day |
| `build-release.yaml` | Manual dispatch | Linux (x64/arm64), macOS (arm64), Windows (x64) | 1 day |

**Docker Build Image:**
- Base: `fedora:42`
- Toolchain: mingw64-gcc, mingw64-gcc-c++, mingw64-qt6-qtbase, mingw64-qt6-qtmultimedia, mingw64-libarchive
- Defined in: `dev-docker/Dockerfile`

---

*Stack analysis: 2026-04-19*
