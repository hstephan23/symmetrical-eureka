# Session Persistence V1 Design

## Purpose

Persist the Workbench's open-file state explicitly, so a user can save the current set of open buffers and restore it later.

## Scope

- Add a `session` module that saves and loads workspace state.
- Persist open buffer file paths and the current buffer index.
- Add explicit commands:
  - `session-save [path]`
  - `session-load [path]`
- Use `.tide-session` when no path is provided.
- Refuse `session-load` when any open buffer is dirty.

## Non-Goals

- No autosave-on-exit yet.
- No unnamed buffer persistence.
- No cursor, viewport, split, or panel persistence yet.
- No escaping for newlines in paths; session files are line-based.

## Format

The session format is a small text file:

```text
tide-session-v1
current\t0
file\tpath/to/file.c
file\tinclude/file.h
```

## Testing

Unit tests cover save/load round-trips and replacing an existing workspace. App tests cover explicit commands and dirty-load refusal.
