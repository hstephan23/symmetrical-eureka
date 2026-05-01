# Project File Finder V1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add project file discovery and fuzzy `open` resolution.

**Architecture:** Create a `project_files` module for recursive scanning and fuzzy matching. Keep app command execution responsible for resolving an `open` argument to a concrete path before calling workspace APIs.

**Tech Stack:** C11, CMake, CTest, POSIX directory APIs, standard C library only.

---

### Task 1: Project File Scanner

**Files:**
- Create: `include/tide/project_files.h`
- Create: `src/project_files.c`
- Create: `tests/test_project_files.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] Write failing tests for recursive scan, ignored directories, fuzzy filtering, and cleanup.
- [ ] Run `cmake --build build` and verify failure on missing `tide/project_files.h`.
- [ ] Implement project file storage, recursive scanning, ignore rules, and fuzzy filtering.
- [ ] Run `cmake --build build` and `ctest --test-dir build --output-on-failure -R test_project_files`.

### Task 2: Fuzzy Open

**Files:**
- Modify: `src/app.c`
- Modify: `tests/test_app.c`
- Modify: `README.md`

- [ ] Write failing app tests for `open <fuzzy-query>` and literal path fallback.
- [ ] Implement lazy project file index resolution in workspace command execution.
- [ ] Document fuzzy `open`.
- [ ] Run normal and sanitizer builds with CTest.
