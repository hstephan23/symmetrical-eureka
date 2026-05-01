# Multi-Buffer Workbench V1 Design

## Purpose

Add the first Workbench milestone slice: keep more than one file open at a time while preserving the command-centric editor model. This turns `open <path>` from "replace the current file" into "open another buffer and switch to it" when running through the workbench.

## Scope

- Add a `workspace` module that owns multiple `TideBuffer`/`TideEditor` pairs.
- Keep each buffer's cursor, viewport, undo/redo history, search state, dirty flag, and status independently.
- Support command prompt actions for buffer navigation:
  - `open <path>` opens a file in a new buffer and switches to it.
  - Opening an already-open path switches to the existing buffer instead of duplicating it.
  - `buffers` lists open buffers in the status line.
  - `bn` and `next-buffer` switch to the next buffer.
  - `bp` and `prev-buffer` switch to the previous buffer.
  - `buffer <n>` switches to a 1-based buffer index.
- Keep existing single-editor command behavior available for tests and low-level use.

## Non-Goals

- No fuzzy matching yet.
- No persistent session state yet.
- No project tree, diagnostics, output panel, or splits.
- No path canonicalization; V1 treats exact path strings as identities.

## Architecture

`TideWorkspace` owns a growable array of entries. Each entry contains one `TideBuffer` and one `TideEditor`. The editor points at the buffer inside the same entry, so the workspace refreshes editor buffer pointers after entry-array growth.

The app loop renders and edits only the current workspace editor. Workspace-aware command execution handles buffer commands first, then delegates existing editor-local commands such as `write`, `find`, `undo`, and `reload` to the current editor.

## Error Handling

- Empty `open` and `buffer` commands set status text and keep the current buffer.
- File load errors become status text rather than crashing the editor.
- Allocation failures return `TIDE_ERR_ALLOC` to the caller.
- Navigation commands are no-ops for an empty workspace, though the interactive app always creates at least one buffer.

## Testing

Add unit tests for workspace ownership and switching, plus app command tests for workbench commands. Existing editor, buffer, renderer, and command tests must continue to pass.
