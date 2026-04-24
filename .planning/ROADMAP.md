# Roadmap: Shijima-Qt

**Milestone:** v0.1 — Build Fix
**Total phases:** 1

## Phase Summary

| # | Phase | Goal | Requirements | Success Criteria |
|---|-------|------|--------------|-----------------|
| 1 | Build Fix | Fix linker errors and verify launch | FIX-01, FIX-02, FIX-03, FIX-04 | 4 |

---

## Phase 1: Build Fix

**Goal:** Fix linker errors and verify application launches

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

**Files to modify:**
- `libshijima/CMakeLists.txt` — add `-DSHIJIMA_LOGGING_ENABLED=1` to CMAKE_CXX_FLAGS
- `ShijimaWidget.cc` — implement `sendMatrixMessage(const QString&)`
- `ShijimaWidget.hpp` — verify declaration matches implementation

**Verification:**
```bash
cd /home/impulse/workspace/railgun/Shijima-claw
make -C Platform/Linux all && cp Platform/Linux/Linux.a Platform/Platform.a
make clean && make shijima-qt
./shijima-qt  # should launch without crash
```
