# Command Palette Suggestions V1 Design

## Purpose

Turn the existing Ctrl-P command prompt into the first real command palette. The editor should show available commands while the prompt is open, filter them as the user types, and support lightweight keyboard selection without changing the command-centric model.

## Scope

- Add a `command_palette` module with a static command catalog.
- Support fuzzy subsequence matching for command names.
- Render up to four matching commands above the status line while the prompt is open.
- Let Up/Down move the selected suggestion in command mode.
- Let Enter run the selected command when the typed prompt is empty or only a fuzzy command prefix.
- Keep argument commands such as `open path/to/file.c` and `find text` running as typed.

## Non-Goals

- No project file finder yet.
- No command registration system yet.
- No dynamic extension API yet.
- No command argument completion yet.

## Architecture

`command_palette` owns command metadata and matching. `editor` owns prompt selection state because that state is purely UI/session state. `editor_render` asks the command palette for matches and draws a compact overlay. `app` keeps command execution as the authority and only uses the palette to resolve selected command names in prompt mode.

## Error Handling

The palette never returns hard errors for unmatched input. Empty input returns the catalog in command order. Unknown typed commands still flow to the existing command dispatcher and produce the existing status message.

## Testing

Add unit tests for fuzzy matching and filtering order. Add renderer tests for visible suggestions and selected-row styling. Add app tests for executing a fuzzy selected command and preserving raw argument commands.
