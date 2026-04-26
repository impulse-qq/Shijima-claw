# Roadmap: Shijima-Qt

**Milestone:** v0.1 — Build Fix
**Total phases:** 2
**Status:** In Progress

## Phase Summary

| # | Phase | Goal | Requirements | Success Criteria | Status |
|---|-------|------|--------------|-----------------|--------|
| 1 | Build Fix | Fix linker errors and verify launch | FIX-01, FIX-02, FIX-03, FIX-04 | 4 | Complete |
| 2 | Matrix send/receive message functionality | Route sendMatrixMessage through MatrixClient; add error feedback | MATRIX-01, MATRIX-02, MATRIX-03 | 2 | Complete |

## Phase 2: Matrix send/receive message functionality

**Goal:** Implement Matrix send/receive message functionality — routing `ShijimaWidget::sendMatrixMessage()` through `MatrixClient::sendMessage()` and handling incoming messages

**Requirements:**
- MATRIX-01: `ShijimaWidget::sendMatrixMessage()` routes to `MatrixClient::sendMessage()` when connected
- MATRIX-02: Config path uses `QStandardPaths::AppConfigLocation` for cross-platform
- MATRIX-03: txnId uses timestamp-based generation to avoid rapid-send collisions

**Depends on:** Phase 1

**Plans:** 2 plans

Plans:
- [x] 02-01-PLAN.md — Wire sendMatrixMessage to MatrixClient::sendMessage() with error handling
- [x] 02-02-PLAN.md — Fix config path (QStandardPaths) and txnId collision

---

## Phase 1: Build Fix

**Goal:** Fix linker errors and verify application launches

**Status:** Complete

**Requirements:**
- FIX-01: SHIJIMA_LOGGING_ENABLED defined
- FIX-02: sendMatrixMessage implemented
- FIX-03: Project builds
- FIX-04: Application launches

**Success Criteria:**
1. `nm libshijima/build/libshijima.a | grep set_log_level` returns symbols (no longer undefined)
2. `ShijimaWidget.cc` contains `sendMatrixMessage` implementation
3. `make shijima-qt` completes without linker errors
4. `./shijima-qt` starts and displays mascot window without crashing

**Files modified:**
- `libshijima/CMakeLists.txt` — added `target_compile_definitions(shijima PRIVATE SHIJIMA_LOGGING_ENABLED=1)`
- `ShijimaWidget.cc` — implemented `sendMatrixMessage(const QString&)` stub
- `ShijimaWidget.hpp` — removed Q_OBJECT and signals, converted sendMatrixMessage to public slot
- `Platform/Makefile` — fixed dependency chain, all:: target before include
- `Platform/Linux/Makefile` — added `all:: Linux.a` before include to prevent clean as default

**Verification:**
```bash
cd /home/impulse/workspace/railgun/Shijima-claw
make clean && make shijima-qt
LD_LIBRARY_PATH=libshimejifinder/build/unarr:$LD_LIBRARY_PATH ./shijima-qt --help
```

**Result:** Build succeeds, executable runs.
