# Coding Conventions

**Analysis Date:** 2026-04-19

## Naming Patterns

**Files:**
- Source files: `.cc` (not `.cpp` or `.cxx`)
- Headers: `.hpp` (not `.h`)
- Objective-C++: `.mm` (macOS platform code)
- PascalCase for class files: `ShijimaManager.cc`, `AssetLoader.hpp`, `SoundEffectManager.cc`
- Platform-specific files: `ActiveWindowObserver.cc`, `GNOME.cc`, `KWin.cc`

**Classes:**
- PascalCase: `ShijimaManager`, `ShijimaWidget`, `AssetLoader`, `MascotData`
- No prefix convention (not `CShijimaManager` or similar)

**Member Variables:**
- `m_` prefix with camelCase: `m_mascotTimer`, `m_loadedMascots`, `m_sandboxWidget`
- Examples from `ShijimaManager.hpp`:
  ```cpp
  int m_mascotTimer = -1;
  bool m_allowClose = false;
  QMap<QString, MascotData *> m_loadedMascots;
  std::mutex m_mutex;
  ```

**Functions:**
- camelCase: `spawn()`, `killAll()`, `loadData()`, `updateEnvironment()`
- No Hungarian notation on function names

**Constants/Macros:**
- UPPER_SNAKE_CASE: `SHIJIMAQT_SUBTICK_COUNT`
- Defined with `#define` or `static const`

**Namespaces:**
- `Platform::` for platform abstraction layer (`Platform::ActiveWindowObserver`)
- `shijima::` for libshijima submodule (`shijima::mascot::manager`)
- `shijima::math::` for math types (`shijima::math::vec2`)

## Code Style

**Formatting:**
- No `.clang-format`, `.editorconfig`, or `.prettierrc` present
- 4-space indentation observed consistently
- Braces: Allman style for class definitions, K&R for functions
- Line length: No enforced limit; long lines observed in `ShijimaManager.cc`

**Header Guards:**
- `#pragma once` exclusively (no `#ifndef` guards)
- Every `.hpp` file starts with `#pragma once`

**License Header:**
- GPL-3.0 license block at top of every source file:
  ```cpp
  // 
  // Shijima-Qt - Cross-platform shimeji simulation app for desktop
  // Copyright (C) 2025 pixelomer
  // ...
  ```

**Include Organization:**
1. Standard library headers (`<string>`, `<vector>`, `<memory>`)
2. Qt headers (`<QString>`, `<QImage>`, `<QWidget>`)
3. Submodule headers (`<shijima/mascot/manager.hpp>`)
4. Project headers with quotes (`"Asset.hpp"`, `"ShijimaWidget.hpp"`)
- No strict ordering enforced; mixed ordering observed in some files
- Duplicate includes exist (e.g., `<QDesktopServices>` imported 3 times in `ShijimaManager.cc`)

**Build System:**
- Makefile-based (no CMake for main project, submodules use CMake)
- `common.mk` shared configuration
- C++17 standard: `-std=c++17`
- Warning flags: `-Wall`
- Release mode: `-O3 -flto -DNDEBUG`
- Debug mode: `-g -O0`
- Platform detection via `uname -s` and compiler prefix

## Import Organization

**Order (observed, not enforced):**
1. Own header first (`.cc` includes matching `.hpp`)
2. Standard library headers
3. Qt headers
4. External library headers (`<httplib.h>`, `<shijima/...>`)
5. Project headers (quoted includes)

**Path Aliases:**
- None used; all includes are relative paths
- Submodule includes use angle brackets: `<shijima/mascot/manager.hpp>`
- Project includes use quotes: `"Asset.hpp"`

## Error Handling

**Exception Strategy:**
- Standard exceptions: `std::runtime_error`, `std::invalid_argument`, `std::system_error`
- Custom exceptions: `DBusCallError`, `DBusReturnError` (in `Platform/Linux/DBus.hpp`)
- Exceptions used for fatal/unexpected conditions only

**Patterns:**
- Catch by const reference: `catch (std::exception &ex)`
- Error logging to `std::cerr` with context:
  ```cpp
  catch (std::exception &ex) {
      std::cerr << "couldn't load mascot: " << name.toStdString() << std::endl;
      std::cerr << ex.what() << std::endl;
  }
  ```
- HTTP API returns error JSON objects with `"error"` key:
  ```cpp
  QJsonObject obj;
  obj["error"] = "400 Bad Request";
  res.status = 400;
  ```
- `noexcept` used sparingly (only on `ShijimaManager::import()`)

**Return Values:**
- `nullptr` for optional pointer returns
- Empty containers (`{}`, `{}`) for collection returns
- `std::optional` for nullable value returns
- `EXIT_SUCCESS`/`EXIT_FAILURE` for CLI exit codes

## Logging

**Framework:** None — raw `std::cout`/`std::cerr` throughout

**Patterns:**
- Status/info to `std::cout`:
  ```cpp
  std::cout << "Loaded mascot: " << data->name().toStdString() << std::endl;
  std::cout << "Mascots path: " << m_mascotsPath.toStdString() << std::endl;
  ```
- Errors/warnings to `std::cerr`:
  ```cpp
  std::cerr << "warning: sandboxWidget is not initialized" << std::endl;
  std::cerr << "import failed: " << ex.what() << std::endl;
  ```
- HTTP API logging via custom logger:
  ```cpp
  m_server->set_logger([](const Request &req, const Response &) {
      std::cout << req.method << " " << req.path << std::endl;
  });
  ```
- CLI uses `cout`/`cerr` macros for platform-specific redirection (Windows uses MessageBox)
- No structured logging, no log levels, no timestamps

## Comments

**When to Comment:**
- FIXME comments for known issues: `//FIXME: refresh only changed items`
- URL references for sourced algorithms: `// https://stackoverflow.com/questions/...`
- Platform-specific code commented with `#if defined()` blocks
- Inline comments rare; self-documenting code preferred

**JSDoc/TSDoc:**
- Not applicable (C++ project)
- No Doxygen-style comments observed
- No `///` or `/** */` documentation blocks

## Function Design

**Size:**
- Wide range: from 1-line accessors to 200+ line methods (`buildToolbar()` at ~180 lines)
- No enforced maximum

**Parameters:**
- `const&` for non-trivial types: `QString const&`, `std::string const&`
- Raw pointers for nullable references: `QWidget *parent = nullptr`
- `std::function` for callbacks: `std::function<void(ShijimaManager *)>`
- `std::initializer_list` for variadic-style args: `ArgumentList(std::initializer_list<Argument>)`

**Return Values:**
- `const&` for internal state accessors
- Raw pointers for factory methods: `static ShijimaManager *defaultManager()`
- `std::unique_lock<std::mutex>` for RAII lock acquisition
- `void` for mutating operations

## Module Design

**Exports:**
- Each `.cc`/`.hpp` pair represents one class or module
- No barrel files or re-export patterns
- Public API via class methods, not free functions (except CLI)

**Singleton Pattern:**
- `defaultManager()` / `finalize()` pattern for global access:
  ```cpp
  static ShijimaManager *m_defaultManager = nullptr;
  ShijimaManager *ShijimaManager::defaultManager() {
      if (m_defaultManager == nullptr) {
          m_defaultManager = new ShijimaManager;
      }
      return m_defaultManager;
  }
  ```
- Same pattern for `AssetLoader::defaultLoader()`

**Platform Abstraction:**
- `Platform/` directory with OS-specific subdirectories
- Common interface in `Platform/Platform.hpp`, `Platform/ActiveWindowObserver.hpp`
- Private implementations in `Platform/{Linux,macOS,Windows,Stub}/`
- Build system selects correct backend at compile time

**Qt Signal/Slot Connections:**
- Lambda-based connections preferred:
  ```cpp
  connect(action, &QAction::triggered, [this](){
      QDesktopServices::openUrl(QUrl::fromLocalFile(m_mascotsPath));
  });
  ```
- Traditional slot connections for class methods:
  ```cpp
  connect(action, &QAction::triggered, this, &ShijimaManager::importAction);
  ```

**Macros:**
- `#define` used sparingly
- `addPreset` macro in `buildToolbar()` for repetitive scale preset creation
- `cout`/`cerr` macros in `cli.cc` for platform-specific stream redirection

---

*Convention analysis: 2026-04-19*
