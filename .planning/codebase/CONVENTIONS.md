# Coding Conventions

**Analysis Date:** 2026/04/26

## Language and Standards

**Primary Language:** C++17
- Standard flags defined in `common.mk`: `-std=c++17 -Wall -fPIC`
- Release: `-O3 -DNDEBUG`
- Debug: `-g -O0`

**Build System:** GNU Make (Makefile-based)
- `common.mk` provides shared build configuration
- Submodules (`libshijima`, `libshimejifinder`) use CMake internally

**Framework:** Qt 6 (with Qt5 fallback detection via `QMAKE ?= qmake-qt5`)

## File Organization

**Header Format:**
Every `.cc` and `.hpp` file begins with a GPL v3 license block:
```cpp
// 
// Shijima-Qt - Cross-platform shimeji simulation app for desktop
// Copyright (C) 2025 pixelomer
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
```

**Source Extensions:**
- `.cc` for C++ implementation (not `.cpp` or `.cxx`)
- `.hpp` for C++ headers
- `.mm` for Objective-C++ (macOS platform code)

**Includes Order (observed pattern):**
1. Local headers (matching `.hpp` first)
2. Standard library headers (`<cmath>`, `<exception>`, `<filesystem>`, `<string>`)
3. Qt headers (`<QWidget>`, `<QPainter>`, `<QJsonDocument>`)
4. External library headers (`<httplib.h>`, `<shijima/shijima.hpp>`, `<shimejifinder/utils.hpp>`)
5. Platform headers (`"Platform/Platform.hpp"`)

## Naming Conventions

**Files:**
- PascalCase matching class name: `ShijimaManager.cc`, `MatrixClient.cc`, `AssetLoader.hpp`
- Platform files: `ActiveWindowObserver.cc`, `cli.cc`

**Classes:** PascalCase
- Examples: `ShijimaManager`, `MatrixClient`, `AssetLoader`, `ForcedProgressDialog`, `ShimejiInspectorDialog`

**Member Variables:** `m_` prefix + camelCase
- Examples: `m_mascots`, `m_running`, `m_lastError`, `m_syncThread`, `m_connected`, `m_windowedMode`
- Boolean members often use adjectives: `m_visible`, `m_allowClose`, `m_firstShow`
- Thread-safe members use `std::atomic`: `m_running.load()`, `m_connected.store()`

**Methods/Functions:** camelCase
- Examples: `loadConfig()`, `startSyncLoop()`, `tick()`, `spawnClicked()`, `acquireLock()`
- Slot methods: `importAction()`, `deleteAction()`, `quitAction()`, `buildToolbar()`

**Constants/Macros:** UPPER_SNAKE_CASE
- Examples: `SHIJIMAQT_SUBTICK_COUNT`, `SHIJIMA_USE_QTMULTIMEDIA`
- Defined with `#define` or `static const int`

**Namespaces:**
- `shijima::` for libshijima mascot engine
- `shijima::mascot::` for mascot-related types
- `Platform::` for platform abstraction layer

## Indentation and Formatting

**Indentation:** 4 spaces (no tabs)

**Braces:** K&R style for functions, same-line for control structures
```cpp
void MatrixClient::syncLoop() {
    while (m_running.load()) {
        if (!m_connected.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }
        // ...
    }
}
```

**Control Flow:** Always braces for single statements
```cpp
if (m_accessToken.isEmpty()) {
    m_lastError = "No access token configured";
    emit errorOccurred(m_lastError);
    return;
}
```

**Member Initialization:** Constructor initializer lists with comma-separated members
```cpp
MatrixClient::MatrixClient(QObject *parent)
    : QObject(parent)
    , m_syncThread(nullptr)
    , m_running(false)
    , m_connected(false)
    , m_nextBatch()
    , m_retryCount(0)
{
}
```

## Error Handling

**Patterns:**

1. **Return codes with error member:** Methods return `bool` and store error in `m_lastError`
   - `MatrixClient::loadConfig()` returns `false` on error, `m_lastError` holds message
   ```cpp
   if (!file.open(QIODevice::ReadOnly)) {
       m_lastError = "Failed to open config: " + path;
       return false;
   }
   ```

2. **Exception throwing:** For unrecoverable programmer errors
   ```cpp
   throw std::runtime_error("loadData() called with invalid data");
   throw std::runtime_error("Impossible condition: New mascot name is incorrect");
   if (target->m_dragTargetPt != nullptr) {
       throw std::runtime_error("target widget being dragged by multiple widgets");
   }
   ```

3. **std::exception catching:** In import/processing code with context logging
   ```cpp
   catch (std::exception &ex) {
       std::cerr << "couldn't load mascot: " << name.toStdString() << std::endl;
       std::cerr << ex.what() << std::endl;
   }
   ```

4. **noexcept for guaranteed-failure operations:** Used on `ShijimaManager::import()`
   ```cpp
   std::set<std::string> import(QString const& path) noexcept;
   ```

**Qt Error Signals:**
```cpp
emit errorOccurred(m_lastError);
```

**HTTP API Error Responses:**
```cpp
QJsonObject obj;
obj["error"] = "400 Bad Request";
res.status = 400;
```

## Logging

**Console Output:** `std::cout` for info, `std::cerr` for errors
```cpp
std::cout << "Loaded mascot: " << data->name().toStdString() << std::endl;
std::cout << "Mascots path: " << m_mascotsPath.toStdString() << std::endl;
std::cerr << "warning: sandboxWidget is not initialized" << std::endl;
std::cerr << "import failed: " << ex.what() << std::endl;
```

**HTTP API Logging:**
```cpp
m_server->set_logger([](const Request &req, const Response &) {
    std::cout << req.method << " " << req.path << std::endl;
});
```

**No structured logging framework** - raw `std::cout`/`std::cerr` usage
**No log levels, no timestamps, no structured formatting**

## Comment Style

**File Headers:** GPL license block (required on every file)

**FIXME Comments:** Mark known issues
```cpp
//FIXME: refresh only changed items
//FIXME: is this position correct?
```

**Algorithm References:** Stack Overflow links when applicable
```cpp
// https://stackoverflow.com/questions/34135624/-/54029758#54029758
static void dispatchToMainThread(std::function<void()> callback) {
```

**Inline Comments:** Sparse usage, self-documenting code preferred
```cpp
// Determine the frame anchor within the window
if (isMirroredRender()) {
```

**No Doxygen/Javadoc style** - no `///` or `/** */` documentation blocks observed

## Code Organization Patterns

**Singleton Pattern:**
```cpp
static ShijimaManager *m_defaultManager = nullptr;

ShijimaManager *ShijimaManager::defaultManager() {
    if (m_defaultManager == nullptr) {
        m_defaultManager = new ShijimaManager;
    }
    return m_defaultManager;
}

void ShijimaManager::finalize() {
    if (m_defaultManager != nullptr) {
        delete m_defaultManager;
        m_defaultManager = nullptr;
    }
}
```

**Thread Dispatch to Main Thread:**
```cpp
static void dispatchToMainThread(std::function<void()> callback) {
    QTimer *timer = new QTimer;
    timer->moveToThread(qApp->thread());
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [timer, callback]() {
        callback();
        timer->deleteLater();
    });
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));
}
```

**RAII Lock Pattern:**
```cpp
std::unique_lock<std::mutex> ShijimaManager::acquireLock() {
    return std::unique_lock<std::mutex> { m_mutex };
}
```

**Lambda Signal Connections:**
```cpp
connect(action, &QAction::triggered, [this](bool checked){
    for (auto &env : m_env) {
        env->allows_breeding = checked;
    }
    m_settings.setValue(key, QVariant::fromValue(checked));
});
```

**Qt Queued Connection for Thread-Safe Signals:**
```cpp
QMetaObject::invokeMethod(this, [this, sender, body]() {
    emit messageReceived(sender, body, m_roomId);
}, Qt::QueuedConnection);
```

## Qt-Specific Patterns

**MOC Processing:** Files with `Q_OBJECT` require MOC generation
- `MatrixClient.hh` generates `MatrixClient.moc`
- Rule in `common.mk`:
```make
MatrixClient.moc: MatrixClient.hh
    $(MOC) MatrixClient.hh -o MatrixClient.moc
```
- Dependencies tracked: `MatrixClient.o: MatrixClient.cc MatrixClient.moc`
- Include at end of `.cc` file: `#include "MatrixClient.moc"`

**Header Guards:** `#pragma once` exclusively
```cpp
#pragma once
```

**Signal/Slot Connections:** Using new Qt5+ syntax
```cpp
connect(action, &QAction::triggered, this, &ShijimaManager::quitAction);
connect(&m_listWidget, &QListWidget::itemDoubleClicked,
    this, &ShijimaManager::itemDoubleClicked);
```

**Lambda Connections:** Preferred for capturing local state
```cpp
connect(action, &QAction::triggered, [this](){
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_mascotsPath));
});
```

**Thread-safe Atomic Variables:**
```cpp
std::atomic<bool> m_running;
std::atomic<bool> m_connected;
// Usage:
if (m_running.load()) { ... }
```

**QSettings for Persistent Configuration:**
```cpp
m_settings("pixelomer", "Shijima-Qt")
// Usage:
bool initial = m_settings.value(key, QVariant::fromValue(true)).toBool();
m_settings.setValue(key, QVariant::fromValue(checked));
```

## Platform Abstraction

**Directory Structure:**
```
Platform/
├── Linux/
│   ├── DBus.hpp
│   ├── GNOME.cc
│   ├── KWin.cc
│   └── ...
├── macOS/
├── Windows/
├── Stub/
├── Platform.hpp
├── PlatformWidget.hpp
└── ActiveWindowObserver.hpp
```

**Platform-Specific Compilation:**
```cpp
#if defined(__APPLE__)
    // macOS code
#elif defined(_WIN32)
    // Windows code
#else
    // Linux code
#endif
```

**Build System Platform Detection:**
```makefile
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    PLATFORM := Linux
endif
```

---

*Convention analysis: 2026/04/26*
