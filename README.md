# Terminal C IDE

`tide` is a macOS-first terminal IDE written in C. The project is intentionally built from scratch: no `ncurses`, terminal UI library, JSON library, async library, editor library, or LSP client library.

The current implementation has the editor core in place and is moving into the Workbench milestone: multiple open buffers, command-centric switching, raw terminal input, virtual screen rendering, ANSI color output, editing, search, undo/redo, file commands, and C syntax highlighting.

## Design

- [Terminal C IDE Design](docs/superpowers/specs/2026-04-30-terminal-c-ide-design.md)
- [Foundation Implementation Plan](docs/superpowers/plans/2026-04-30-terminal-c-ide-foundation.md)
- [Multi-Buffer Workbench V1 Design](docs/superpowers/specs/2026-05-01-multi-buffer-workbench-v1-design.md)

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

- Ctrl-P opens a command palette with fuzzy command suggestions.
- Typing filters suggestions; Up and Down move the selected suggestion.
- Enter runs the selected suggestion when the prompt contains a command prefix.
- Commands with arguments, such as `open <path>` and `find <text>`, run as typed.
- `write`, `save`, or `w` saves.
- `quit` or `q` quits.
- `wq` saves and quits.
- `open <path>` opens a file in the current editor.
- In interactive file mode, `open <path>` opens another buffer and switches to it. If the path does not exist, `open <query>` fuzzy-matches project files from the current directory.
- `reload` reloads the current file from disk.
- `find <text>` searches in the current file.
- `next` jumps to the next match.
- `prev` jumps to the previous match.
- `undo` reverts the last edit.
- `redo` reapplies the last undone edit.
- `buffers` lists open buffers in the status line.
- `buffer <n>` switches to a 1-based buffer index.
- `bn` or `next-buffer` switches to the next buffer.
- `bp` or `prev-buffer` switches to the previous buffer.
- `session-save [path]` saves open buffers and the current buffer to a session file. Defaults to `.tide-session`.
- `session-load [path]` loads a saved session. Defaults to `.tide-session` and refuses to load when any open buffer is dirty.
- `build [command]` runs a build command and opens the captured output in `*build-output*`. Defaults to `cmake --build build`.
- Escape or Ctrl-P closes the prompt.
