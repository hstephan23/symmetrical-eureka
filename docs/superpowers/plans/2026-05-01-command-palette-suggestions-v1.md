# Command Palette Suggestions V1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add visible command palette suggestions with fuzzy command filtering and prompt selection.

**Architecture:** Add a standalone `command_palette` module for catalog/matching logic. Extend `TideEditor` with prompt selection state, render matches above the status line, and resolve selected commands from `src/app.c` only when safe to do so.

**Tech Stack:** C11, CMake, CTest, POSIX/macOS syscalls, standard C library only.

---

### Task 1: Command Palette Matcher

**Files:**
- Create: `include/tide/command_palette.h`
- Create: `src/command_palette.c`
- Create: `tests/test_command_palette.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] Write failing tests for empty catalog results, fuzzy subsequence matching, non-matches, and sorted filtering.
- [ ] Run `cmake --build build` and verify failure on missing `tide/command_palette.h`.
- [ ] Implement command catalog, scoring, filtering, and exact lookup.
- [ ] Run `cmake --build build` and `ctest --test-dir build --output-on-failure -R test_command_palette`.

### Task 2: Prompt Selection And Rendering

**Files:**
- Modify: `include/tide/editor.h`
- Modify: `src/editor.c`
- Modify: `src/editor_render.c`
- Modify: `tests/test_editor.c`
- Modify: `tests/test_editor_render.c`

- [ ] Write failing tests for prompt selection reset/movement and visible suggestion rows.
- [ ] Implement editor selection helpers and renderer overlay.
- [ ] Run `cmake --build build` and `ctest --test-dir build --output-on-failure -R "test_editor|test_editor_render"`.

### Task 3: App Command Resolution

**Files:**
- Modify: `src/app.c`
- Modify: `tests/test_app.c`
- Modify: `README.md`

- [ ] Write failing app tests for fuzzy Enter executing the selected command and argument commands staying raw.
- [ ] Implement prompt Enter resolution and Up/Down selection wiring.
- [ ] Document command palette behavior.
- [ ] Run normal and sanitizer builds with CTest.
