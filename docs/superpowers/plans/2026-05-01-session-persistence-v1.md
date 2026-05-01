# Session Persistence V1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add explicit save/load session commands for open workspace buffers.

**Architecture:** Create a `session` module that serializes/deserializes `TideWorkspace`. Keep app command code responsible for dirty-buffer refusal and default session path selection.

**Tech Stack:** C11, CMake, CTest, standard C library file I/O.

---

### Task 1: Session Module

**Files:**
- Create: `include/tide/session.h`
- Create: `src/session.c`
- Create: `tests/test_session.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] Write failing round-trip and replace-existing-workspace tests.
- [ ] Run `cmake --build build` and verify failure on missing `tide/session.h`.
- [ ] Implement line-based session save/load.
- [ ] Run `cmake --build build` and `ctest --test-dir build --output-on-failure -R test_session`.

### Task 2: App Commands

**Files:**
- Modify: `src/app.c`
- Modify: `src/command_palette.c`
- Modify: `tests/test_app.c`
- Modify: `README.md`

- [ ] Write failing tests for `session-save`, `session-load`, and dirty load refusal.
- [ ] Implement app command dispatch with `.tide-session` default path.
- [ ] Add command catalog entries.
- [ ] Run normal and sanitizer builds with CTest.
