# Command Palette v1 Design

## Purpose

Command Palette v1 adds the first command-centric editing flow to `tide`. The goal is not a fuzzy finder or full command system yet; it is a small modal prompt that lets the user save and quit by typing commands instead of relying only on control-key shortcuts.

## Scope

The editor gains a prompt mode opened with `Ctrl-P`. While prompt mode is active:

- Printable characters append to the prompt.
- Backspace deletes one prompt character.
- Enter runs the typed command.
- Escape or `Ctrl-P` cancels the prompt and returns to normal editing.
- Buffer text and cursor movement are not changed by prompt typing.

Supported commands:

- `save` and `write`: save the file and keep editing.
- `w`: alias for `write`.
- `quit` and `q`: quit without saving.
- `wq`: save, then quit if save succeeds.

Unknown commands leave the editor open and show `unknown command: <command>` in the status line.

## Architecture

Prompt state belongs to `TideEditor` because rendering, input routing, and tests all need a single source of truth for whether normal editing or command entry is active. The editor owns a fixed-size command buffer, exposes small operations for opening/canceling/editing/submitting the prompt, and keeps command execution policy in the app layer where saving and quitting already live.

The renderer keeps the current single-line status area. When prompt mode is active, the status line renders `:<command>` instead of file metadata so the user can see what they are typing.

## Error Handling

Prompt text has a fixed maximum length. If the prompt is full, additional printable input is ignored and the status line remains stable. Save failures are reported through the existing editor status message using `tide_status_string()`. `wq` quits only after a successful save.

## Testing

Tests cover editor prompt state transitions, prompt rendering, command dispatch behavior, and app-level interactive smoke behavior where practical. Existing editor, renderer, and file editing tests must continue to pass.

## Out Of Scope

This slice does not implement fuzzy matching, command history, completions, arbitrary command registration, keyboard shortcut display, file opening from the prompt, or command-palette overlay UI.
