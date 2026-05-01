# ANSI Color Output v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Emit ANSI SGR escape codes for existing `TideCell` foreground, background, and style attributes so syntax highlighting is visible in a real terminal.

**Architecture:** Keep terminal encoding inside `src/ansi.c`. Track the currently emitted foreground, background, and style while rendering cells, emit a full reset plus active attributes when a cell differs from the current attributes, then append the cell character. This avoids changing the screen, editor, syntax, or string-builder APIs.

**Tech Stack:** C11, CMake, CTest, existing `TideScreen`, `TideCell`, and `TideStringBuilder`.

---

## File Structure

- `src/ansi.c`: add SGR attribute emission for full and dirty render paths.
- `tests/test_ansi.c`: add tests for foreground color, reverse style, default reset between cells, and dirty-cell style output.
- `README.md`: no change; syntax highlighting is already documented on this branch.

## Task 1: ANSI SGR Cell Attributes

**Files:**
- Modify: `src/ansi.c`
- Modify: `tests/test_ansi.c`

- [ ] **Step 1: Write failing ANSI color tests**

Append to `tests/test_ansi.c` before `main()`:

```c
static void test_full_render_emits_color_style_and_reset(void)
{
    TideScreen screen;
    TideStringBuilder out;

    TIDE_ASSERT(tide_screen_init(&screen, 2, 1) == TIDE_OK);
    TIDE_ASSERT(tide_string_builder_init(&out) == TIDE_OK);
    TIDE_ASSERT(tide_screen_set(&screen, 0, 0, tide_cell_make('X', TIDE_COLOR_GREEN, TIDE_COLOR_DEFAULT, TIDE_STYLE_BOLD)) == TIDE_OK);
    TIDE_ASSERT(tide_screen_set(&screen, 1, 0, tide_cell_make('Y', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE)) == TIDE_OK);

    TIDE_ASSERT(tide_ansi_render_full(&screen, &out) == TIDE_OK);

    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "\x1b[0;1;32mX") != NULL);
    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "\x1b[0mY") != NULL);
    TIDE_ASSERT(tide_screen_dirty_count(&screen) == 0);

    tide_string_builder_free(&out);
    tide_screen_free(&screen);
}

static void test_full_render_emits_reverse_style(void)
{
    TideScreen screen;
    TideStringBuilder out;

    TIDE_ASSERT(tide_screen_init(&screen, 1, 1) == TIDE_OK);
    TIDE_ASSERT(tide_string_builder_init(&out) == TIDE_OK);
    TIDE_ASSERT(tide_screen_set(&screen, 0, 0, tide_cell_make('R', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_REVERSE)) == TIDE_OK);

    TIDE_ASSERT(tide_ansi_render_full(&screen, &out) == TIDE_OK);

    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "\x1b[0;7mR") != NULL);

    tide_string_builder_free(&out);
    tide_screen_free(&screen);
}

static void test_dirty_render_emits_cursor_and_color(void)
{
    TideScreen screen;
    TideStringBuilder out;

    TIDE_ASSERT(tide_screen_init(&screen, 2, 1) == TIDE_OK);
    TIDE_ASSERT(tide_string_builder_init(&out) == TIDE_OK);
    tide_screen_mark_clean(&screen);
    TIDE_ASSERT(tide_screen_set(&screen, 1, 0, tide_cell_make('A', TIDE_COLOR_RED, TIDE_COLOR_DEFAULT, TIDE_STYLE_REVERSE)) == TIDE_OK);

    TIDE_ASSERT(tide_ansi_render_dirty(&screen, &out) == TIDE_OK);

    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "\x1b[1;2H\x1b[0;7;31mA") != NULL);
    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "\x1b[0m") != NULL);
    TIDE_ASSERT(tide_screen_dirty_count(&screen) == 0);

    tide_string_builder_free(&out);
    tide_screen_free(&screen);
}
```

Call all three tests from `main()`:

```c
    test_full_render_emits_color_style_and_reset();
    test_full_render_emits_reverse_style();
    test_dirty_render_emits_cursor_and_color();
```

- [ ] **Step 2: Verify the tests fail**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `test_ansi` fails because `src/ansi.c` still appends only cell characters and no SGR color/style sequences.

- [ ] **Step 3: Implement SGR attribute emission**

In `src/ansi.c`:

- Include `<stdio.h>` because the implementation uses `snprintf()`.
- Add an internal render-state struct:

```c
typedef struct AnsiState {
    TideColor fg;
    TideColor bg;
    unsigned style;
} AnsiState;
```

- Add helpers:

```c
static int color_code(TideColor color, int background);
static int attributes_equal(AnsiState state, TideCell cell);
static TideStatus append_sgr(TideStringBuilder *out, TideCell cell, AnsiState *state);
```

Implementation requirements:

- Initial `AnsiState` is `{TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE}`.
- `color_code()` returns `30`-`37` for foreground colors and `40`-`47` for background colors. Return `0` for `TIDE_COLOR_DEFAULT` and unsupported values.
- `append_sgr()` emits nothing when the cell attributes match the current state.
- When attributes differ, `append_sgr()` builds one sequence beginning with `"\x1b[0"`, appends `";1"` for `TIDE_STYLE_BOLD`, appends `";7"` for `TIDE_STYLE_REVERSE`, appends `";<fg>"` and `";<bg>"` when colors are not default, then appends `"m"`.
- After emitting, `append_sgr()` updates the state to the cell attributes.
- `append_cell()` calls `append_sgr()` before appending the printable character.
- Full render uses one `AnsiState` for the entire screen and still appends final `"\x1b[0m"`.
- Dirty render uses one `AnsiState` across all dirty cells, emits cursor movement before each dirty cell, calls `append_cell()`, and appends final `"\x1b[0m"` when the state is not default.
- Only mark the screen clean after all appends succeed.

- [ ] **Step 4: Verify green**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 5: Commit**

```bash
git add src/ansi.c tests/test_ansi.c
git commit -m "feat: emit ANSI cell colors and styles"
```

## Task 2: Full Verification

**Files:**
- No file edits expected.

- [ ] **Step 1: Run full verification**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
cmake -S . -B build-asan -DTIDE_ENABLE_SANITIZERS=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
./build/tide --version
./build/tide --render-demo
```

Expected: all commands succeed; both CTest runs report `100% tests passed`.

- [ ] **Step 2: Run manual terminal smoke**

Run:

```bash
./build/tide build/manual-syntax-visible.c
```

Type:

```text
#include <stdio.h>
int main(void) { return 42; }
Ctrl-P
q
Enter
```

Expected: the editor shows colored C tokens in a compatible terminal, the status line remains reverse-styled, and command-prompt quit exits cleanly.

## Coverage Checklist

- Foreground color and bold style are covered by `test_full_render_emits_color_style_and_reset()`.
- Reverse style is covered by `test_full_render_emits_reverse_style()`.
- Reset back to default before following default text is covered by `test_full_render_emits_color_style_and_reset()`.
- Dirty render cursor movement plus styled output is covered by `test_dirty_render_emits_cursor_and_color()`.
- Clean marking after render is covered by existing and new ANSI tests.
