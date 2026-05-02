# Build Output V1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a build command that runs a child process and displays captured output in the workbench.

**Architecture:** Add a `tasks` module for blocking child-process execution. Add a workspace helper for opening text output buffers. Wire `build [command]` through the app command dispatcher.

**Tech Stack:** C11, CMake, CTest, POSIX `fork`, `pipe`, `dup2`, `execl`, and `waitpid`.

---

### Task 1: Task Runner

**Files:**
- Create: `include/tide/tasks.h`
- Create: `src/tasks.c`
- Create: `tests/test_tasks.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [x] Write failing tests for output capture and nonzero exit code.
- [x] Run `cmake --build build` and verify failure on missing `tide/tasks.h`.
- [x] Implement blocking shell-command execution with stdout/stderr capture.
- [x] Run focused task tests.

### Task 2: Build Command Output Buffer

**Files:**
- Modify: `include/tide/workspace.h`
- Modify: `src/workspace.c`
- Modify: `src/app.c`
- Modify: `src/command_palette.c`
- Modify: `tests/test_app.c`
- Modify: `README.md`

- [x] Write failing app tests for `build <command>` success and failure.
- [x] Implement workspace text buffer helper and build command dispatch.
- [x] Document `build [command]`.
- [x] Run normal and sanitizer test suites.
