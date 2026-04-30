# Terminal C IDE Design

## Purpose

Build a standalone macOS-first terminal IDE in C to stress-test AI coding ability on a large systems project. The final product should be a command-centric terminal IDE for C projects with a from-scratch editor core, terminal renderer, workbench, build integration, LSP client, project symbols, debugger hooks, persistent sessions, themes, and command extensibility.

This project is separate from `mysh`; this repository only holds the planning document until implementation begins in a new project workspace.

## Constraints

- Language: C.
- Initial platform: macOS-first.
- Dependency rule: hardcore from scratch.
- Allowed implementation dependencies: C standard library and POSIX/macOS syscalls.
- Disallowed implementation dependencies: `ncurses`, terminal UI libraries, JSON libraries, async/event-loop libraries, editor libraries, LSP client libraries.
- Allowed external tools: `clang`, `clangd`, `lldb`, `make`, `cmake`, and other command-line tools invoked as child processes.

## Product Direction

The IDE should feel like a fast terminal-native editor first, then grow into a full IDE. The default screen is an editor surface with a compact status line. Most actions are launched through a command palette rather than permanent panels.

Core user actions include:

- Open files through a command palette.
- Edit, search, undo/redo, and save C source files.
- Run build commands and inspect output.
- Jump from diagnostics to source locations.
- Use language intelligence through `clangd`.
- Open temporary panels for diagnostics, build output, project files, completions, hover, and debugger interaction.
- Split panes later through commands instead of making split panes always visible.

## Milestones

### 1. Foundation

Implement raw terminal mode, input decoding, ANSI output, virtual screen rendering, damage tracking, layout primitives, error cleanup, and test infrastructure.

Completion criteria:

- The terminal always restores normal mode after clean exit, startup failure, fatal error, and interrupt paths.
- Input bytes become normalized key/text events.
- The renderer can draw a virtual screen and flush changed cells.
- Rendering and input parsing are testable without an interactive terminal.

### 2. Editor Core

Implement the first complete product target: a usable terminal editor for C files.

Completion criteria:

- Open, edit, search, undo/redo, save, and reload files.
- Maintain cursor, viewport, dirty state, and status messages.
- Highlight C syntax with a custom tokenizer.
- Save local regular files by writing a same-directory temporary file, flushing it, and renaming it over the original. Unsupported save targets fail cleanly without modifying the original file.
- Support command palette actions for common editor commands.

### 3. Workbench

Add IDE workbench structure around the editor without losing the command-centric model.

Completion criteria:

- Manage multiple open buffers.
- Add command palette fuzzy matching for files and commands.
- Add persistent session state for open files and layout.
- Add temporary project tree, diagnostics, and output panels.
- Add split panes through explicit commands.

### 4. Build IDE

Integrate build and run workflows.

Completion criteria:

- Run configurable build and run commands as child processes.
- Capture stdout and stderr asynchronously.
- Parse Clang-style diagnostics.
- Show diagnostics in a temporary panel and status summaries.
- Jump from a diagnostic to the correct file, line, and column.

### 5. C Intelligence

Implement language-server support from scratch.

Completion criteria:

- Implement enough JSON parsing and serialization for LSP messages.
- Implement JSON-RPC framing over `clangd` stdio.
- Start, initialize, monitor, and shut down `clangd`.
- Surface LSP diagnostics, hover, completion, go-to-definition, and document symbols.

### 6. Advanced IDE

Add the features that make the project feel like a full IDE.

Completion criteria:

- Project-wide symbol search.
- Debugger command hooks through `lldb`.
- Themes.
- Command registration system for extension-like internal commands.
- Stable split-pane workflows.

## Architecture

The codebase should be split into small modules with clear ownership.

- `platform`: macOS terminal setup, raw mode, terminal size, file/process syscalls, monotonic time.
- `input`: escape-sequence parser for keyboard input and future mouse support.
- `render`: virtual screen buffer, ANSI emission, damage tracking, color/style attributes.
- `layout`: rectangular pane tree, status bars, popups, command palette surfaces.
- `buffer`: text storage, file loading/saving, line indexing, dirty state.
- `editor`: cursor, viewport, editing operations, search, undo/redo.
- `syntax`: incremental C tokenizer and highlighter.
- `workspace`: project root detection, file tree, open buffer registry, session state.
- `tasks`: build/run subprocesses, async output capture, diagnostics parsing.
- `lsp`: JSON parser, JSON-RPC framing, `clangd` process, LSP message handling.
- `commands`: command registry connecting keybindings and palette actions to operations.
- `app`: event loop, state orchestration, startup, shutdown, and fatal cleanup.

The terminal, renderer, buffer, and editor modules must not depend on build tooling or LSP. Build and LSP features consume editor and workspace state through explicit interfaces.

## Data Flow

Input flow:

1. Terminal bytes enter the input parser.
2. The parser emits normalized events such as `KEY_ARROW_LEFT`, `KEY_CTRL_S`, `TEXT_INPUT`, and `COMMAND_SUBMIT`.
3. The commands layer maps events to operations.
4. Operations mutate explicit application state.
5. The render layer turns current state into a virtual screen and flushes changed cells.

Editing flow:

1. The workspace opens a file.
2. The buffer owns text storage and dirty state.
3. The editor owns cursor, viewport, search state, and undo/redo transactions.
4. The syntax module reads buffer content or dirty ranges and produces highlight spans.
5. The renderer combines editor state, buffer text, syntax spans, gutters, overlays, and status bars.

Async flow:

1. Tasks and LSP own subprocess pipes.
2. The event loop polls terminal input, timers, and child-process output.
3. Build and LSP messages become internal events.
4. Internal events update diagnostics, completions, symbols, output panels, and status messages.

## Error Handling

- Restore terminal state on normal exit, failed startup after raw mode begins, fatal errors, and interrupt paths.
- Return explicit status codes from allocation and syscall-heavy paths.
- Surface recoverable errors in the status area or a temporary message panel.
- Fatal errors restore the terminal, print a concise message, and exit nonzero.
- File saves for local regular files should use a same-directory temporary file, flush, and rename. Unsupported save targets should fail cleanly without modifying the original file.
- Child-process failures should not crash the app.
- Malformed terminal input, build output, JSON, or LSP messages should degrade into recoverable errors.

## Testing And Verification

The project should be testable without relying only on manual terminal sessions.

Required test areas:

- Input escape parsing.
- Buffer edits and line indexing.
- Undo/redo transaction behavior.
- Search behavior.
- C syntax tokenization and highlighting spans.
- Render snapshots for deterministic layouts.
- File save failure paths.
- Build diagnostic parsing.
- JSON parsing and serialization.
- JSON-RPC framing.
- Command palette filtering and dispatch.

Verification should include normal builds, sanitizer builds, unit tests, render snapshot tests, and manual terminal smoke tests.

## Non-Goals For The First Complete Target

The first complete target is Milestone 2, not the full IDE. The first complete target will not include LSP, debugger hooks, project-wide symbols, full split-pane workflows, or a permanent build output panel. Its design must leave room for those features, but it should finish as a polished terminal editor first.
