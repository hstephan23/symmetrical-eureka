# Run Command V1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `run <command>` to execute a child process and display captured output in `*run-output*`.

**Architecture:** Reuse the existing blocking task runner and workspace text-buffer helper. Wire `run <command>` through app command dispatch, command palette lookup, and README docs without changing build diagnostics behavior.

**Tech Stack:** C11, CMake, CTest, POSIX shell execution through existing `tide_tasks_run_shell()`.

---

### Task 1: Run Command Dispatch

**Files:**
- Modify: `src/app.c`
- Modify: `tests/test_app.c`
- Modify: `src/command_palette.c`
- Modify: `tests/test_command_palette.c`
- Modify: `README.md`

- [x] Write failing app tests for `run <command>` success, nonzero exit, and missing command status.
- [x] Write failing command palette test for exact `run` lookup.
- [x] Run `cmake --build build` and focused tests to verify expected failures.
- [x] Implement `run <command>` using `tide_tasks_run_shell()` and `tide_workspace_open_text()`.
- [x] Document `run <command>`.
- [x] Run normal and sanitizer builds with CTest.
