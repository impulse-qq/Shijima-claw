# Testing Patterns

**Analysis Date:** 2026-04-19

## Test Framework

**Runner:**
- **None detected** — Project has zero test infrastructure
- No `test/`, `tests/`, or `spec/` directories
- No `*.test.*` or `*.spec.*` files found
- No test configuration files (no `jest.config.*`, `vitest.config.*`, `CMakeLists.txt` with test targets)

**Assertion Library:**
- Not applicable

**Run Commands:**
- No test commands exist
- Build commands only:
  ```bash
  CONFIG=release make -j8    # Build release
  CONFIG=debug make -j8      # Build debug
  ```

## Test File Organization

**Location:**
- Not applicable — no test files exist

**Naming:**
- Not applicable

**Structure:**
- Not applicable

## Test Structure

**Not applicable.** The following guidance is provided for future test implementation:

Given the project structure, a recommended test layout would be:

```
tests/
├── unit/
│   ├── Asset_test.cc
│   ├── MascotData_test.cc
│   └── AssetLoader_test.cc
├── integration/
│   ├── ShijimaHttpApi_test.cc
│   └── cli_test.cc
└── fixtures/
    └── test_mascot.mascot/
```

Or co-located alongside source:
```
src/
├── Asset.cc
├── Asset_test.cc
├── MascotData.cc
└── MascotData_test.cc
```

## Mocking

**Framework:** Not applicable

**Patterns:**
- Not applicable

**What to Mock (when tests are added):**
- HTTP server responses (`httplib::Server`)
- Platform-specific backends (`Platform::ActiveWindowObserver`)
- File system operations (mascot loading, archive extraction)
- Qt event loops and timers
- `QSettings` for configuration testing

**What NOT to Mock:**
- `libshijima` mascot behavior engine (integration test instead)
- Qt core types (`QString`, `QImage`, `QPoint`)
- Math utilities (`shijima::math::vec2`)

## Fixtures and Factories

**Test Data:**
- Not applicable

**Location:**
- Not applicable

**Suggested Test Mascot:**
- `DefaultMascot/` directory could serve as a reference fixture
- Contains 46 PNG frames + `behaviors.xml` + `actions.xml`

## Coverage

**Requirements:** None enforced

**View Coverage:**
- Not applicable — no coverage tooling configured

## Test Types

**Unit Tests:**
- Not implemented
- Candidates: `Asset`, `MascotData`, `AssetLoader`, `SoundEffectManager`

**Integration Tests:**
- Not implemented
- Candidates: `ShijimaHttpApi` (HTTP endpoints), CLI commands, mascot spawning/dismissal flow

**E2E Tests:**
- Not implemented
- Would require Qt test framework (`QTest`) or external automation
- Candidates: Full mascot lifecycle (spawn → animate → dismiss)

## Recommended Testing Approach

Given the Qt6/C++17 stack, the following options are available:

### Option 1: Qt Test Framework (QTest)
```cpp
#include <QTest>

class TestMascotData : public QObject {
    Q_OBJECT
private slots:
    void testValidMascot() {
        MascotData data { "test.mascot", 0 };
        QVERIFY(data.valid());
    }
    void testInvalidPath() {
        QVERIFY_EXCEPTION_THROWN(
            MascotData { "invalid", 0 },
            std::runtime_error
        );
    }
};
QTEST_MAIN(TestMascotData)
```

### Option 2: Google Test
```cpp
#include <gtest/gtest.h>

TEST(MascotDataTest, ValidMascot) {
    MascotData data { "test.mascot", 0 };
    EXPECT_TRUE(data.valid());
}

TEST(MascotDataTest, InvalidPath) {
    EXPECT_THROW(MascotData { "invalid", 0 }, std::runtime_error);
}
```

### Option 3: Catch2
```cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("MascotData validates correctly", "[mascot]") {
    MascotData data { "test.mascot", 0 };
    REQUIRE(data.valid());
}
```

## Build Integration

To add tests to the existing Makefile build system:

```makefile
# Example addition to Makefile
TEST_SOURCES = tests/unit/Asset_test.cc \
               tests/unit/MascotData_test.cc

TEST_OBJECTS = $(patsubst %.cc,%.o,$(TEST_SOURCES))

test: shijima-qt$(EXE) $(TEST_OBJECTS)
	$(CXX) -o test_runner $(TEST_OBJECTS) shijima-qt.a \
		$(TARGET_LDFLAGS) $(LDFLAGS) -lgtest -lgtest_main
	./test_runner
```

## Critical Areas Requiring Test Coverage

1. **HTTP API** (`ShijimaHttpApi.cc`) — REST endpoints, JSON parsing, error responses
2. **Mascot Loading** (`MascotData.cc`, `AssetLoader.cc`) — Archive extraction, XML parsing, asset caching
3. **CLI** (`cli.cc`) — Argument parsing, API client calls, error handling
4. **Platform Abstraction** (`Platform/`) — KWin/DBus/GNOME backends, window tracking
5. **ShijimaManager** (`ShijimaManager.cc`) — Mascot lifecycle, multi-screen handling, tick synchronization
6. **Asset** (`Asset.cc`) — Image trimming, alpha mask generation

## Known Testing Challenges

- **Qt dependency** — Requires `QApplication` instance for most Qt operations
- **Platform-specific code** — Linux/macOS/Windows backends need separate test environments
- **Submodule dependencies** — `libshijima` and `libshimejifinder` must be initialized
- **GUI components** — `ShijimaWidget` rendering tests require display or virtual framebuffer
- **HTTP server** — `ShijimaHttpApi` binds to port 32456; tests need port isolation
- **Singleton pattern** — `ShijimaManager::defaultManager()` and `AssetLoader::defaultLoader()` require `finalize()` calls between tests

---

*Testing analysis: 2026-04-19*
