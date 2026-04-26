# Testing Patterns

**Analysis Date:** 2026/04/26

## Test Infrastructure Status

**No unit, integration, or E2E tests exist in this codebase.**

The project has zero test infrastructure:
- No `test/` or `tests/` directories
- No `*.test.*` or `*.spec.*` files
- No test configuration files (no `jest.config.*`, `vitest.config.*`, CMake test targets)
- No testing framework dependencies in build system

**Testing exists only in dependencies:**
- `cpp-httplib/test/` - httplib's own test suite (not part of this project)
- `libshimejifinder/unarr/test/` - unarr archive library tests (submodule)
- `libshimejifinder/libarchive/test_utils/` - libarchive utility tests (submodule)

## Build Integration

**Build Commands:**
```bash
CONFIG=release make -j8    # Release build
CONFIG=debug make -j8      # Debug build
```

**CI Pipeline (GitHub Actions):** `.github/workflows/build-debug.yaml`

The CI builds for three platforms but does NOT run tests:
- Windows (Ubuntu Docker with mingw64)
- Linux (ubuntu-22.04 and ubuntu-24.04-arm)
- macOS (macos-14)

Artifacts are archived but no test execution occurs.

## Unit Test Candidates

Based on code review, the following components should have unit tests:

**High Priority:**

1. **Asset (`Asset.cc`)**
   - `getRectForImage()` - image cropping algorithm
   - `setImage()` - alpha mask generation, mirroring
   - File: `/home/impulse/workspace/railgun/Shijima-claw/Asset.cc`

2. **MascotData (`MascotData.cc`)**
   - XML parsing validation
   - Path resolution (`imgRoot()`, `actionsXML()`, `behaviorsXML()`)
   - `valid()` check
   - File: `/home/impulse/workspace/railgun/Shijima-claw/MascotData.cc`

3. **AssetLoader (`AssetLoader.cc`)**
   - Asset caching mechanism
   - Path resolution
   - File: `/home/impulse/workspace/railgun/Shijima-claw/AssetLoader.cc`

4. **MatrixClient (`MatrixClient.cc`)**
   - JSON parsing (`loadConfig()`, `parseSyncNextBatch()`)
   - Event validation (`isValidEvent()`)
   - Message extraction (`extractBody()`, `extractSender()`)
   - File: `/home/impulse/workspace/railgun/Shijima-claw/MatrixClient.cc`

## Integration Test Candidates

1. **ShijimaHttpApi (`ShijimaHttpApi.cc`)**
   - REST endpoint handling
   - JSON request parsing
   - Error response formatting
   - Port 32456 binding
   - File: `/home/impulse/workspace/railgun/Shijima-claw/ShijimaHttpApi.cc`

2. **CLI (`cli.cc`)**
   - Argument parsing
   - API client calls to HTTP server
   - Error handling
   - File: `/home/impulse/workspace/railgun/Shijima-claw/cli.cc`

3. **ShijimaManager (`ShijimaManager.cc`)**
   - Mascot lifecycle (spawn, kill, killAll)
   - Import flow (archive extraction, mascot loading)
   - Multi-screen environment handling
   - File: `/home/impulse/workspace/railgun/Shijima-claw/ShijimaManager.cc`

## Test Data Fixtures

**DefaultMascot** directory serves as a reference mascot:
- Location: `/home/impulse/workspace/railgun/Shijima-claw/DefaultMascot/`
- Contains 46 PNG frames (`img/shime1.png` through `img/shime46.png`)
- `behaviors.xml` - behavior state machine
- `actions.xml` - action definitions

**Mascot Archive Structure** (for import testing):
- `.mascot` directory format
- `actions.xml` - required
- `behaviors.xml` - required
- `img/` directory with PNG frames
- `sound/` directory (optional) with audio files

## Recommended Testing Approach

Given the Qt6/C++17 stack, these testing options are available:

### Qt Test Framework (QTest)

Natural choice given Qt dependency:
```cpp
#include <QTest>

class TestAsset : public QObject {
    Q_OBJECT
private slots:
    void testGetRectForImage() {
        QImage image(10, 10, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        image.setPixelColor(5, 5, Qt::red);
        Asset asset;
        asset.setImage(image);
        QRect rect = asset.getRectForImage(image);
        QVERIFY(rect.width() > 0);
        QVERIFY(rect.height() > 0);
    }
};
QTEST_MAIN(TestAsset)
```

### Google Test

Requires adding `gtest` dependency to build:
```cpp
#include <gtest/gtest.h>

TEST(AssetTest, ImageRectCalculation) {
    QImage image(100, 100, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    image.setPixelColor(50, 50, Qt::red);
    Asset asset;
    asset.setImage(image);
    EXPECT_TRUE(asset.offset().width() > 0);
}
```

### Catch2

Header-only alternative:
```cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Asset trims transparent borders", "[asset]") {
    QImage image(64, 64, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    image.setPixelColor(10, 10, Qt::blue);
    Asset asset;
    asset.setImage(image);
    REQUIRE(asset.offset().topLeft().x() == 10);
}
```

## Mocking Strategy

**What to Mock:**
- HTTP responses from Matrix homeserver (mock `httplib::Client`)
- Platform backends (`Platform::ActiveWindowObserver` across Linux/DBus/KWin)
- File system operations (archive extraction, mascot directory creation)
- `QSettings` for configuration testing
- Qt event loop (using `QTest::qWait()`)

**What NOT to Mock:**
- `QString`, `QImage`, `QPoint` - test with real Qt types
- `shijima::mascot::manager` - integration test instead
- `libshimejifinder::analyze` - integration test instead

## Coverage Gaps

**Critical areas without tests:**
1. Import flow (archive analysis + extraction + mascot loading)
2. Mascot spawning and lifecycle
3. Multi-screen environment handling
4. Platform-specific window tracking (KWin/DBus/GNOME)
5. HTTP API endpoint handlers
6. Matrix sync loop and message parsing
7. Windowed mode transitions
8. Tick synchronization callbacks

**No test coverage enforcement:**
- No coverage tooling configured
- No CI test step
- No pre-commit test hook

## Testing Challenges

1. **Qt Application Requirement** - Most tests need `QApplication` instance
2. **Display Dependency** - GUI components (`ShijimaWidget`) need virtual framebuffer or headless rendering
3. **Singleton Pattern** - `ShijimaManager::defaultManager()` and `AssetLoader::defaultLoader()` require `finalize()` between tests
4. **Platform-Specific Code** - Linux/DBus/KWin backends need Linux environment
5. **HTTP Server Binding** - `ShijimaHttpApi` binds port 32456; tests need port isolation
6. **Submodule Initialization** - `libshijima` and `libshimejifinder` must be built first

## Adding Tests to Build System

To integrate tests into the Makefile:

```makefile
# In common.mk or Makefile
TEST_SOURCES = tests/unit/Asset_test.cc \
               tests/unit/MascotData_test.cc \
               tests/unit/MatrixClient_test.cc

TEST_OBJECTS = $(patsubst %.cc,%.o,$(TEST_SOURCES))

test: shijima-qt$(EXE) $(TEST_OBJECTS)
	$(CXX) -o test_runner $(TEST_OBJECTS) shijima-qt.a \
		$(TARGET_LDFLAGS) $(LDFLAGS) -lgtest -lgtest_main
	./test_runner

clean::
	rm -f $(TEST_OBJECTS) test_runner
```

---

*Testing analysis: 2026/04/26*
