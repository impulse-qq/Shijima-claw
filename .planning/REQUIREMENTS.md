# Requirements: Shijima-Qt

**Defined:** 2026-04-25
**Core Value:** A fun, reliable desktop mascot experience that works across platforms with minimal setup.

## v0.1 Requirements

### Build Fixes

- [ ] **FIX-01**: `SHIJIMA_LOGGING_ENABLED` macro is defined during libshijima build, allowing `shijima::set_log_level` to link correctly
- [ ] **FIX-02**: `ShijimaWidget::sendMatrixMessage(const QString&)` method is implemented, enabling Matrix message sending
- [ ] **FIX-03**: Project builds successfully with `make shijima-qt` and produces working executable
- [ ] **FIX-04**: Application launches without crashing and displays mascot

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| FIX-01 | Phase 1 | Pending |
| FIX-02 | Phase 1 | Pending |
| FIX-03 | Phase 1 | Pending |
| FIX-04 | Phase 1 | Pending |

**Coverage:**
- v0.1 requirements: 4 total
- Mapped to phases: 4
- Unmapped: 0 ✓

---
*Requirements defined: 2026-04-25*
