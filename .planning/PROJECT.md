# Shijima-Qt

## What This Is

Cross-platform shimeji desktop pet simulator. Built with Qt6, supports macOS, Linux, and Windows. Displays animated mascot characters on the desktop that interact with the user and each other.

## Core Value

A fun, reliable desktop mascot experience that works across platforms with minimal setup.

## Requirements

### Validated

(None yet — initial build setup in progress)

### Active

- [ ] Fix `SHIJIMA_LOGGING_ENABLED` undefined reference linking error
- [ ] Implement missing `ShijimaWidget::sendMatrixMessage` method
- [ ] Project builds and launches successfully

### Out of Scope

- Matrix integration features beyond basic send (future milestone)
- Platform-specific plugin installation (future milestone)

## Context

- Qt6-based desktop mascot application
- Uses libshijima (C++ mascot engine) and libshimejifinder (archive handler)
- Recent commits added MatrixClient for sending messages but implementations incomplete
- Build infrastructure issue: Platform/Makefile dependency chain problem

## Constraints

- **Tech stack**: Qt6, C++17, libshijima, libshimejifinder
- **Platform**: Linux (primary), macOS, Windows via Docker
- **Build system**: Make with CMake sub-builds

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Use Qt6 | Modern Qt with better cross-platform support | ✓ Good |
| Matrix integration | Enable messaging via Matrix protocol | In progress |

---

*Last updated: 2026-04-25 after initial GSD setup*
