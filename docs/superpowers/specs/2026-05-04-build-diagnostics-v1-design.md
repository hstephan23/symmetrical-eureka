# Build Diagnostics V1 Design

## Goal

Build Diagnostics V1 turns captured build output into actionable diagnostics. After `build [command]`, Tide should keep the raw `*build-output*` buffer, parse Clang-style diagnostic lines, open a compact `*diagnostics*` buffer when diagnostics exist, and let users jump between diagnostic source locations.

## Scope

This slice handles synchronous output that is already captured by the existing `build [command]` command. It does not add asynchronous task execution, incremental output streaming, or permanent panes. Those remain part of the later Build IDE milestone.

## Diagnostic Format

The parser recognizes Clang/GCC-style lines:

```text
path/to/file.c:12:5: error: expected ';' after expression
path/to/file.c:18:9: warning: unused variable 'x'
path/to/file.c:20:3: note: expanded from macro 'X'
path/to/file.c:4:1: fatal error: 'missing.h' file not found
```

Each parsed diagnostic stores path, 1-based line, 1-based column, severity, and message. Unrecognized build output is ignored.

## Architecture

Add a focused `diagnostics` module for parsing, storing, freeing, and navigating diagnostic records. The workspace owns a `TideDiagnostics` collection because diagnostics are project/workbench state, not editor state.

The app remains the command dispatcher. `build [command]` parses the captured output into workspace diagnostics, refreshes `*build-output*`, and opens `*diagnostics*` when one or more diagnostics are found. Navigation commands use the current diagnostic index to open the diagnostic file, move the editor cursor to the source location, and update the status line.

## Commands

- `diagnostics` opens the current diagnostics list, or sets `no diagnostics` when none exist.
- `dn` and `diagnostic-next` advance to the next diagnostic, wrapping around.
- `dp` and `diagnostic-prev` move to the previous diagnostic, wrapping around.

## Error Handling

Malformed lines are skipped. Allocation failures return `TIDE_ERR_ALLOC`. Missing or unopenable diagnostic files surface the existing workspace open status in the editor status line. Running a build with no diagnostics clears any previous diagnostics.

## Testing

Unit tests cover diagnostic parsing, ignored non-diagnostic output, fatal errors, and navigation wraparound. Workspace/app tests cover build output parsing, diagnostics buffer rendering, clearing stale diagnostics, and diagnostic navigation to file/line/column.
