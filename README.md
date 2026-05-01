# Terminal C IDE

`tide` is a macOS-first terminal IDE written in C. The project is intentionally built from scratch: no `ncurses`, terminal UI library, JSON library, async library, editor library, or LSP client library.

The current implementation target is Foundation: terminal raw mode, input parsing, virtual screen rendering, ANSI output, layout primitives, and a minimal command-centric app shell.

## Design

- [Terminal C IDE Design](docs/superpowers/specs/2026-04-30-terminal-c-ide-design.md)
- [Foundation Implementation Plan](docs/superpowers/plans/2026-04-30-terminal-c-ide-foundation.md)

## Requirements

- macOS
- CMake 3.20+
- Apple Clang or compatible C11 compiler

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

## Sanitizer Build

```bash
cmake -S . -B build-asan -DTIDE_ENABLE_SANITIZERS=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

## Run

```bash
./build/tide --version
./build/tide --render-demo
./build/tide
```

In interactive mode, press `q` to exit.

## Edit A File

```bash
./build/tide path/to/file.c
```

Interactive keys:

- Printable characters insert text.
- Arrow keys move the cursor.
- Enter inserts a newline.
- Backspace deletes before the cursor.
- Ctrl-S saves.
- Ctrl-P opens the command prompt.
- Ctrl-Q quits.
- C keywords, literals, comments, and preprocessor lines are syntax highlighted.

Command prompt:

- `write`, `save`, or `w` saves.
- `quit` or `q` quits.
- `wq` saves and quits.
- `open <path>` opens a file in the current editor.
- `reload` reloads the current file from disk.
- `find <text>` searches in the current file.
- `next` jumps to the next match.
- `prev` jumps to the previous match.
- `undo` reverts the last edit.
- `redo` reapplies the last undone edit.
- Escape or Ctrl-P closes the prompt.
