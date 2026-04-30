# File Commands v1 Design

## Goal

File Commands v1 lets users change or refresh the active file from inside `tide` without restarting the program. It keeps the project editor-first and command-centric while avoiding the complexity of multiple buffers.

## User Experience

The command prompt gains two commands:

- `open <path>` loads `<path>` into the current editor buffer.
- `reload` reloads the current buffer path from disk.

Both commands protect unsaved changes. If the current buffer is dirty, the command does not replace the buffer and sets status to `unsaved changes; write first`. The user can run `write` first, then run `open <path>` or `reload`.

Successful `open <path>` resets the active editor to the loaded file and sets status to `opened: <path>`. Successful `reload` resets the active editor to the current path and sets status to `reloaded`.

## Architecture

The buffer layer gets a reset-and-load API so app commands can replace the contents of an existing `TideBuffer` safely. This keeps file I/O ownership in `TideBuffer` and leaves `TideEditor` responsible for cursor, viewport, search, and history state.

The editor layer gets a reset API that clears cursor position, viewport offset, status, command text, search state, and undo/redo history while keeping the same `TideBuffer *`. App command dispatch composes these APIs: validate the dirty guard, load or reload the buffer, reset the editor view state, then set the user-facing status message.

## Command Semantics

`open <path>`:

- Requires a non-empty path after optional spaces.
- Refuses if `editor->buffer->dirty` is true.
- Loads existing files or creates an empty buffer for missing paths, matching startup file-open behavior.
- On success, the buffer path becomes `<path>`, dirty is cleared, cursor moves to line 1 column 1, viewport returns to the top-left, search state is cleared, and undo/redo history is empty.
- On load failure, keeps the existing buffer unchanged and sets status to the error string.

`reload`:

- Requires the current buffer to have a path.
- Refuses if `editor->buffer->dirty` is true.
- Reloads the current path from disk using the same behavior as startup file-open.
- On success, cursor/view/search/history reset and status becomes `reloaded`.
- On load failure, keeps the existing buffer unchanged and sets status to the error string.

## Error Handling

The implementation must not destroy the current buffer until replacement content has loaded successfully. A temporary buffer can be loaded first, then swapped into the editor's existing buffer on success. If allocation or I/O fails, the original buffer remains intact.

`open` without a path sets status to `path required`. `reload` with no current path sets status to `no file to reload`. Dirty-buffer refusals return `TIDE_OK` because the command was understood and safely rejected.

## Tests

Tests cover app-level command behavior and buffer-level replacement safety:

- `open <path>` loads file contents, changes path, resets cursor, and clears dirty state.
- `open <path>` refuses when the current buffer has unsaved changes.
- `open` without a path sets `path required`.
- `reload` reloads from disk and resets editor view state.
- `reload` refuses when dirty.
- A failed load keeps the previous buffer contents and path.
- Editor reset clears search and undo/redo history.

## Out Of Scope

File Commands v1 does not add multiple buffers, tabs, recent files, fuzzy file picking, file completion, force-open, force-reload, save-as, directory browsing, or reload prompts.
