# Run Command V1 Design

## Goal

Run Command V1 adds a command-palette `run [command]` workflow next to the existing `build [command]` workflow. It runs a shell command through the existing blocking task runner and opens the captured output in `*run-output*`.

## Scope

This slice stays synchronous. It does not add async process polling, terminal streaming, run configuration files, debugger integration, or interactive program support. Those are later Build IDE tasks.

## Behavior

- `run <command>` runs the command through `/bin/sh -c`.
- `run` without arguments reports `run command required`.
- Captured stdout and stderr are written to `*run-output*`.
- The output buffer starts with `$ <command>` and ends with `[exit N]`, matching build output formatting.
- Exit code `0` sets `run passed`; nonzero exits set `run failed: exit N`.
- Running programs does not parse or clear build diagnostics.

## Architecture

Reuse the existing `tasks` module and `tide_workspace_open_text()` helper. Keep command dispatch in `src/app.c`, mirroring the current build command implementation. Add `run` to the command palette catalog and README.

## Testing

App tests cover successful command output, nonzero exit output/status, and missing-argument status. Command palette tests cover exact lookup for `run`. Full verification uses the normal and sanitizer CTest suites.
