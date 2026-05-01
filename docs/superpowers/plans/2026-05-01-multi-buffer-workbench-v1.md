# Multi-Buffer Workbench V1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add multiple open buffers with command-centric buffer switching.

**Architecture:** Create a `workspace` module that owns buffer/editor pairs and exposes current-editor operations. Update app command execution so workbench commands operate on the workspace while existing editor-local commands continue to work unchanged.

**Tech Stack:** C11, CMake, CTest, POSIX/macOS syscalls, standard C library only.

---

### Task 1: Workspace Ownership

**Files:**
- Create: `include/tide/workspace.h`
- Create: `src/workspace.c`
- Create: `tests/test_workspace.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] Write failing tests for opening two files, preserving each editor state, reusing an already-open path, and next/previous switching.
- [ ] Run `cmake --build build` and verify the failure is the missing `tide/workspace.h`.
- [ ] Implement `TideWorkspace`, open/switch/navigation APIs, and cleanup.
- [ ] Run `cmake --build build` and `ctest --test-dir build --output-on-failure`.

### Task 2: Workspace Commands

**Files:**
- Modify: `include/tide/app.h`
- Modify: `src/app.c`
- Modify: `tests/test_app.c`

- [ ] Add failing tests for `open <path>`, `buffers`, `bn`, `bp`, and `buffer <n>` through `tide_app_execute_workspace_command()`.
- [ ] Run `cmake --build build` and verify the failure is the missing command API.
- [ ] Implement workspace command dispatch and update the interactive file loop to use `TideWorkspace`.
- [ ] Run `cmake --build build` and `ctest --test-dir build --output-on-failure`.

### Task 3: Documentation And Verification

**Files:**
- Modify: `README.md`

- [ ] Document multi-buffer commands.
- [ ] Run normal build and CTest.
- [ ] Run sanitizer build and CTest.
- [ ] Commit the completed feature branch.
