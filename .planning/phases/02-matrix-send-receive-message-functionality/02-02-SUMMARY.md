---
phase: "02"
plan: "02"
status: complete
completed: 2026-04-26
wave: 2
---

## Plan 02-02 Summary: Fix config path and txnId collision

**Objective:** Fix two identified pitfalls: hardcoded Linux config path and txnId collision risk

**What was built:**

### MATRIX-02: Cross-platform config path
- Changed ShijimaManager.cc from hardcoded path:
  ```cpp
  // Old:
  QString configPath = QDir::homePath() + "/.config/shijima-qt/matrix.json";
  // New:
  QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/matrix.json";
  ```
- Uses platform-appropriate path:
  - Linux: ~/.config/Shijima-Qt/matrix.json
  - macOS: ~/Library/Application Support/Shijima-Qt/matrix.json
  - Windows: %APPDATA%/Shijima-Qt/matrix.json

### MATRIX-03: Timestamp-based txnId
- Added `#include <chrono>` to MatrixClient.cc
- Changed txnId generation from simple counter to timestamp-based:
  ```cpp
  // Old:
  static long long txnId = 0;
  txnId++;

  // New:
  static long long txnIdBase = std::chrono::steady_clock::now().time_since_epoch().count() % 100000;
  static long long txnIdCounter = 0;
  long long txnId = txnIdBase + (txnIdCounter++ % 100000);
  ```
- Reduces collision risk across process restarts and rapid sends

**Requirements verified:**
- MATRIX-02: Config path uses QStandardPaths::AppConfigLocation
- MATRIX-03: txnId uses timestamp-based generation

**Verification:**
- `grep -c "QStandardPaths::writableLocation.*AppConfigLocation" ShijimaManager.cc` = 1
- Old hardcoded path `~/.config/shijima-qt/matrix.json` NOT FOUND
- `grep -c "steady_clock" MatrixClient.cc` = 1
- `make shijima-qt` completes without errors

**Deviations:** None

**Commits:**
- `b332821` feat(02-02): fix config path and txnId collision
