# ANSI Color Output v1 Design

## Goal

ANSI Color Output v1 makes existing `TideCell` foreground, background, and style attributes visible in real terminal output. This completes the practical path from syntax token classification to colored text in `./build/tide file.c`.

## User Experience

The editor should render colored source text in terminals that support standard ANSI SGR escape sequences. C syntax highlighting already assigns foreground colors to text cells; this slice makes those colors appear on screen.

Status lines and command prompts keep their existing reverse style. Search highlighting remains visible through reverse style while preserving the syntax foreground color already assigned to the cell.

## Architecture

The ANSI renderer remains the only layer that knows how terminal escape sequences are encoded. `TideScreen`, `TideEditor`, and the syntax module continue to work with abstract `TideCell` values.

`src/ansi.c` tracks the currently emitted foreground color, background color, and style while appending cells. Before writing a cell character, it emits an SGR sequence only if the cell attributes differ from the current emitted attributes. Full render starts from reset/default state and ends with `\x1b[0m`. Dirty render also tracks attributes across dirty cells and resets at the end if it emitted styled output.

## Supported Attributes

Foreground colors use standard 30-37 SGR codes:

- black: `30`
- red: `31`
- green: `32`
- yellow: `33`
- blue: `34`
- magenta: `35`
- cyan: `36`
- white: `37`

Background colors use standard 40-47 SGR codes. `TIDE_COLOR_DEFAULT` maps to default foreground/background through reset logic rather than a separate partial reset in v1.

Styles:

- `TIDE_STYLE_BOLD` maps to `1`
- `TIDE_STYLE_REVERSE` maps to `7`
- combined styles emit both codes in one SGR sequence.

To keep v1 simple and correct, any attribute change emits a full reset plus the needed active attributes. This is slightly verbose but avoids partial-reset edge cases.

## Error Handling

ANSI rendering still returns `TideStatus` from string-builder append operations. If emitting an SGR sequence fails, rendering returns that status and does not mark the screen clean.

Unsupported enum values fall back to default color. Existing character rendering behavior is unchanged: `'\0'` renders as a space.

## Tests

Tests cover both full and dirty render paths:

- Full render emits foreground color and bold style before a styled cell.
- Full render emits reverse style for status-style cells.
- Full render resets to default before later default cells, so color does not bleed across cells.
- Dirty render emits cursor movement plus style/color before dirty styled cells.
- Existing screen clean marking still occurs after successful full and dirty renders.

## Out Of Scope

ANSI Color Output v1 does not add 256-color support, RGB truecolor, themes, terminal capability detection, partial SGR resets, underline/italic styles, or output-size optimization.
