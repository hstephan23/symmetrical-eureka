# Build Diagnostics V1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Parse Clang-style build diagnostics, show them in a diagnostics buffer, and add commands to jump between diagnostics.

**Architecture:** Add a `diagnostics` module that owns diagnostic parsing and navigation. Store `TideDiagnostics` on `TideWorkspace`; have `build [command]` refresh the diagnostics state from captured output and open `*diagnostics*` when diagnostics exist.

**Tech Stack:** C11, CMake, CTest, existing Tide workspace/app/test infrastructure.

---

### Task 1: Diagnostics Module

**Files:**
- Create: `include/tide/diagnostics.h`
- Create: `src/diagnostics.c`
- Create: `tests/test_diagnostics.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [x] Write failing tests for Clang error/warning/note parsing, fatal error parsing, ignored non-diagnostic lines, and next/previous wraparound.
- [x] Run `cmake --build build` and verify failure on missing `tide/diagnostics.h`.
- [x] Implement `TideDiagnostics`, parsing, cleanup, and navigation.
- [x] Run `cmake --build build` and `ctest --test-dir build --output-on-failure -R test_diagnostics`.

### Task 2: Workspace And Build Integration

**Files:**
- Modify: `include/tide/workspace.h`
- Modify: `src/workspace.c`
- Modify: `src/app.c`
- Modify: `src/command_palette.c`
- Modify: `tests/test_workspace.c`
- Modify: `tests/test_app.c`
- Modify: `tests/test_command_palette.c`
- Modify: `README.md`

- [x] Write failing workspace/app tests for build diagnostics buffer creation, stale diagnostics clearing, and diagnostic navigation to source locations.
- [x] Run focused tests and verify expected failures.
- [x] Add workspace-owned diagnostics state and app command wiring.
- [x] Document `diagnostics`, `dn`, and `dp`.
- [x] Run normal and sanitizer builds with CTest.
