# Terminal C IDE Editor Core Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the next editor-core slice: open a single text file, edit it in-memory, render it in the terminal, move the cursor, insert/delete/newline text, save changes atomically, and keep the terminal cleanup behavior from Foundation.

**Architecture:** The editor core is split between a file-backed text buffer, an editor state object, and renderer/app integration. `buffer` owns text storage and file save/load, `editor` owns cursor/viewport/editing commands, and `app` translates normalized input events into editor operations before rendering through the existing virtual screen and ANSI renderer.

**Tech Stack:** C11, CMake, CTest, POSIX/macOS syscalls, standard C library only. No `ncurses`, editor libraries, filesystem helper libraries, or terminal UI frameworks.

---

## Scope

This plan implements an editor usable enough to open, edit, and save one file:

- `./build/tide path/to/file.c` opens a file.
- Printable input inserts text.
- Enter splits the current line.
- Backspace deletes before the cursor and joins lines at column 0.
- Arrow keys move the cursor.
- Ctrl-S saves.
- Ctrl-Q quits.
- The status line shows file name, dirty state, cursor position, and messages.

This plan intentionally does not implement undo/redo, search, syntax highlighting, command palette, multi-buffer workbench, diagnostics, LSP, or split panes. Those are separate follow-up plans inside Milestone 2 and 3. This slice still produces working editor software and protects the core boundaries those later features need.

## File Structure

- `include/tide/buffer.h`: text buffer data model, line operations, load/save APIs.
- `src/buffer.c`: dynamic line storage, file loading, same-directory temporary save with rename.
- `tests/test_buffer.c`: buffer editing and file I/O tests.
- `include/tide/editor.h`: editor state, cursor movement, text-editing commands, status messages.
- `src/editor.c`: editor behavior on top of `TideBuffer`.
- `tests/test_editor.c`: cursor/editing behavior tests.
- `include/tide/editor_render.h`: editor rendering API into `TideScreen`.
- `src/editor_render.c`: draw visible buffer lines and status line into screen.
- `tests/test_editor_render.c`: render snapshot tests using `TideScreen`.
- `include/tide/app.h`: extend app entry points to support optional path argument.
- `src/app.c`: replace demo-only loop with an editor loop when a path is provided.
- `main.c`: parse `--version`, `--render-demo`, and optional file path.
- `README.md`: document editor usage.

## Task 1: Text Buffer Data Model

**Files:**
- Create: `include/tide/buffer.h`
- Create: `src/buffer.c`
- Create: `tests/test_buffer.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing buffer tests**

Create `tests/test_buffer.c`:

```c
#include "tide/buffer.h"
#include "test_support.h"

static void test_buffer_starts_with_one_empty_line(void)
{
    TideBuffer buffer;
    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(buffer.line_count == 1);
    TIDE_ASSERT(buffer.lines[0].length == 0);
    TIDE_ASSERT(buffer.dirty == 0);
    tide_buffer_free(&buffer);
}

static void test_insert_char_and_newline(void)
{
    TideBuffer buffer;
    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 0, 0, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 0, 1, 'b') == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_newline(&buffer, 0, 1) == TIDE_OK);
    TIDE_ASSERT(buffer.line_count == 2);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "a");
    TIDE_ASSERT_STR_EQ(buffer.lines[1].data, "b");
    TIDE_ASSERT(buffer.dirty == 1);
    tide_buffer_free(&buffer);
}

static void test_delete_before_cursor_joins_lines(void)
{
    TideBuffer buffer;
    TideBufferPosition pos;
    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 0, 0, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_newline(&buffer, 0, 1) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 1, 0, 'b') == TIDE_OK);

    TIDE_ASSERT(tide_buffer_delete_before(&buffer, (TideBufferPosition){1, 0}, &pos) == TIDE_OK);

    TIDE_ASSERT(buffer.line_count == 1);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "ab");
    TIDE_ASSERT(pos.line == 0);
    TIDE_ASSERT(pos.column == 1);
    tide_buffer_free(&buffer);
}

int main(void)
{
    test_buffer_starts_with_one_empty_line();
    test_insert_char_and_newline();
    test_delete_before_cursor_joins_lines();
    return 0;
}
```

Modify `tests/CMakeLists.txt`:

```cmake
add_tide_test(test_buffer test_buffer.c)
```

- [ ] **Step 2: Run the test and verify it fails**

Run: `cmake --build build`

Expected: build fails because `tide/buffer.h` does not exist.

- [ ] **Step 3: Add buffer API and implementation**

Create `include/tide/buffer.h`:

```c
#ifndef TIDE_BUFFER_H
#define TIDE_BUFFER_H

#include <stddef.h>

#include "tide/status.h"

typedef struct TideLine {
    char *data;
    size_t length;
    size_t capacity;
} TideLine;

typedef struct TideBufferPosition {
    size_t line;
    size_t column;
} TideBufferPosition;

typedef struct TideBuffer {
    TideLine *lines;
    size_t line_count;
    size_t line_capacity;
    char *path;
    int dirty;
} TideBuffer;

TideStatus tide_buffer_init(TideBuffer *buffer);
void tide_buffer_free(TideBuffer *buffer);
TideStatus tide_buffer_insert_char(TideBuffer *buffer, size_t line, size_t column, char ch);
TideStatus tide_buffer_insert_newline(TideBuffer *buffer, size_t line, size_t column);
TideStatus tide_buffer_delete_before(TideBuffer *buffer, TideBufferPosition cursor, TideBufferPosition *next_cursor);
const char *tide_buffer_line_text(const TideBuffer *buffer, size_t line);
size_t tide_buffer_line_length(const TideBuffer *buffer, size_t line);

#endif
```

Implementation notes for `src/buffer.c`:

- Allocate one empty line in `tide_buffer_init()`.
- Store every line as a null-terminated dynamic string.
- Grow line capacity by doubling.
- Grow line array capacity by doubling.
- `tide_buffer_insert_char()` validates line and column, shifts bytes with `memmove`, writes the char, increments length, and marks dirty.
- `tide_buffer_insert_newline()` splits a line at `column`, inserts a new line after it, and marks dirty.
- `tide_buffer_delete_before()` deletes one char before `cursor`, or joins with previous line when `cursor.column == 0`, and returns the new cursor through `next_cursor`.
- `tide_buffer_line_text()` returns `""` for invalid lines.
- `tide_buffer_line_length()` returns `0` for invalid lines.

Modify `CMakeLists.txt` to add `src/buffer.c`.

- [ ] **Step 4: Run tests and verify they pass**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all existing tests plus `test_buffer` pass.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt include/tide/buffer.h src/buffer.c tests/CMakeLists.txt tests/test_buffer.c
git commit -m "feat: add text buffer editing primitives"
```

## Task 2: Buffer File Load And Atomic Save

**Files:**
- Modify: `include/tide/buffer.h`
- Modify: `src/buffer.c`
- Modify: `tests/test_buffer.c`

- [ ] **Step 1: Add failing file I/O tests**

Append to `tests/test_buffer.c`:

```c
static void test_load_file_splits_lines(void)
{
    TideBuffer buffer;
    const char *path = "build/test-buffer-load.txt";
    FILE *file = fopen(path, "w");
    TIDE_ASSERT(file != NULL);
    fputs("one\ntwo\n", file);
    fclose(file);

    TIDE_ASSERT(tide_buffer_load_file(&buffer, path) == TIDE_OK);
    TIDE_ASSERT(buffer.line_count == 2);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "one");
    TIDE_ASSERT_STR_EQ(buffer.lines[1].data, "two");
    TIDE_ASSERT(buffer.dirty == 0);
    TIDE_ASSERT_STR_EQ(buffer.path, path);
    tide_buffer_free(&buffer);
}

static void test_save_file_clears_dirty(void)
{
    TideBuffer buffer;
    const char *path = "build/test-buffer-save.txt";

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_set_path(&buffer, path) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 0, 0, 'x') == TIDE_OK);
    TIDE_ASSERT(tide_buffer_save(&buffer) == TIDE_OK);
    TIDE_ASSERT(buffer.dirty == 0);

    FILE *file = fopen(path, "r");
    char content[8] = {0};
    TIDE_ASSERT(file != NULL);
    TIDE_ASSERT(fread(content, 1, sizeof(content) - 1, file) == 1);
    fclose(file);
    TIDE_ASSERT_STR_EQ(content, "x");
    tide_buffer_free(&buffer);
}
```

Call both tests from `main()`.

- [ ] **Step 2: Run the test and verify it fails**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: build fails because `tide_buffer_load_file`, `tide_buffer_set_path`, and `tide_buffer_save` are not declared.

- [ ] **Step 3: Add file APIs**

Add to `include/tide/buffer.h`:

```c
TideStatus tide_buffer_set_path(TideBuffer *buffer, const char *path);
TideStatus tide_buffer_load_file(TideBuffer *buffer, const char *path);
TideStatus tide_buffer_save(TideBuffer *buffer);
```

Implementation notes:

- `tide_buffer_load_file()` reads the entire file with `fopen`, `fseek`, `ftell`, `fread`; missing files create one empty line and remember the path.
- Split file content on `\n`; do not create an extra blank line for a trailing newline.
- `tide_buffer_set_path()` owns a copied path string.
- `tide_buffer_save()` requires `buffer->path`; write to `<path>.tmp`, flush, close, rename to `path`, remove temp on failure, and clear dirty only on success.

- [ ] **Step 4: Run tests and verify they pass**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `test_buffer` passes with load and save coverage.

- [ ] **Step 5: Commit**

```bash
git add include/tide/buffer.h src/buffer.c tests/test_buffer.c
git commit -m "feat: add buffer file load and save"
```

## Task 3: Editor Cursor And Editing Operations

**Files:**
- Create: `include/tide/editor.h`
- Create: `src/editor.c`
- Create: `tests/test_editor.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing editor tests**

Create `tests/test_editor.c`:

```c
#include "tide/editor.h"
#include "test_support.h"

static void test_insert_text_advances_cursor(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'b') == TIDE_OK);

    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 2);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "ab");
    tide_buffer_free(&buffer);
}

static void test_newline_and_backspace_update_cursor(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_newline(&editor) == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'b') == TIDE_OK);
    TIDE_ASSERT(tide_editor_backspace(&editor) == TIDE_OK);
    TIDE_ASSERT(tide_editor_backspace(&editor) == TIDE_OK);

    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 1);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "a");
    tide_buffer_free(&buffer);
}

static void test_arrow_movement_clamps_to_line_lengths(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_newline(&editor) == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'b') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'c') == TIDE_OK);
    tide_editor_move(&editor, TIDE_EDITOR_MOVE_UP);

    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 1);
    tide_buffer_free(&buffer);
}

int main(void)
{
    test_insert_text_advances_cursor();
    test_newline_and_backspace_update_cursor();
    test_arrow_movement_clamps_to_line_lengths();
    return 0;
}
```

Modify `tests/CMakeLists.txt`:

```cmake
add_tide_test(test_editor test_editor.c)
```

- [ ] **Step 2: Run the test and verify it fails**

Run: `cmake --build build`

Expected: build fails because `tide/editor.h` does not exist.

- [ ] **Step 3: Add editor API and implementation**

Create `include/tide/editor.h`:

```c
#ifndef TIDE_EDITOR_H
#define TIDE_EDITOR_H

#include <stddef.h>

#include "tide/buffer.h"
#include "tide/status.h"

typedef enum TideEditorMove {
    TIDE_EDITOR_MOVE_UP,
    TIDE_EDITOR_MOVE_DOWN,
    TIDE_EDITOR_MOVE_LEFT,
    TIDE_EDITOR_MOVE_RIGHT
} TideEditorMove;

typedef struct TideEditor {
    TideBuffer *buffer;
    TideBufferPosition cursor;
    size_t viewport_line;
    size_t viewport_column;
    char status[128];
} TideEditor;

void tide_editor_init(TideEditor *editor, TideBuffer *buffer);
TideStatus tide_editor_insert_char(TideEditor *editor, char ch);
TideStatus tide_editor_insert_newline(TideEditor *editor);
TideStatus tide_editor_backspace(TideEditor *editor);
void tide_editor_move(TideEditor *editor, TideEditorMove move);
void tide_editor_set_status(TideEditor *editor, const char *message);
void tide_editor_ensure_cursor_visible(TideEditor *editor, size_t width, size_t height);

#endif
```

Implementation notes:

- Editing calls delegate to `TideBuffer`.
- Movement clamps to valid line and column.
- `tide_editor_ensure_cursor_visible()` keeps cursor inside the visible region. Treat `height` as total editor rows including status line; editable rows are `height - 1`.
- `tide_editor_set_status()` copies into fixed storage and always null-terminates.

Modify `CMakeLists.txt` to add `src/editor.c`.

- [ ] **Step 4: Run tests and verify they pass**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all tests through `test_editor` pass.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt include/tide/editor.h src/editor.c tests/CMakeLists.txt tests/test_editor.c
git commit -m "feat: add editor cursor operations"
```

## Task 4: Editor Renderer

**Files:**
- Create: `include/tide/editor_render.h`
- Create: `src/editor_render.c`
- Create: `tests/test_editor_render.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing renderer tests**

Create `tests/test_editor_render.c`:

```c
#include "tide/editor.h"
#include "tide/editor_render.h"
#include "tide/screen.h"
#include "test_support.h"

static void test_editor_render_draws_text_and_status(void)
{
    TideBuffer buffer;
    TideEditor editor;
    TideScreen screen;
    TideCell cell;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 0, 0, 'x') == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    tide_editor_set_status(&editor, "saved");

    TIDE_ASSERT(tide_screen_init(&screen, 20, 4) == TIDE_OK);
    TIDE_ASSERT(tide_editor_render(&editor, &screen) == TIDE_OK);

    TIDE_ASSERT(tide_screen_get(&screen, 0, 0, &cell) == TIDE_OK);
    TIDE_ASSERT(cell.ch == 'x');
    TIDE_ASSERT(tide_screen_get(&screen, 0, 3, &cell) == TIDE_OK);
    TIDE_ASSERT(cell.ch == '[');

    tide_screen_free(&screen);
    tide_buffer_free(&buffer);
}

int main(void)
{
    test_editor_render_draws_text_and_status();
    return 0;
}
```

Modify `tests/CMakeLists.txt`:

```cmake
add_tide_test(test_editor_render test_editor_render.c)
```

- [ ] **Step 2: Run the test and verify it fails**

Run: `cmake --build build`

Expected: build fails because `tide/editor_render.h` does not exist.

- [ ] **Step 3: Add renderer API and implementation**

Create `include/tide/editor_render.h`:

```c
#ifndef TIDE_EDITOR_RENDER_H
#define TIDE_EDITOR_RENDER_H

#include "tide/editor.h"
#include "tide/screen.h"
#include "tide/status.h"

TideStatus tide_editor_render(TideEditor *editor, TideScreen *screen);

#endif
```

Implementation notes:

- Clear screen first.
- Render visible lines from `editor->viewport_line` to `screen->height - 2`.
- Render printable chars only; substitute tabs with a single space in this slice.
- Last row is reverse-style status: `[dirty|clean] path-or-[No Name] Ln X, Col Y message`.
- Call `tide_editor_ensure_cursor_visible()` before drawing.

Modify `CMakeLists.txt` to add `src/editor_render.c`.

- [ ] **Step 4: Run tests and verify they pass**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all tests through `test_editor_render` pass.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt include/tide/editor_render.h src/editor_render.c tests/CMakeLists.txt tests/test_editor_render.c
git commit -m "feat: render editor buffer and status"
```

## Task 5: App Integration For Single-File Editing

**Files:**
- Modify: `include/tide/app.h`
- Modify: `src/app.c`
- Modify: `main.c`
- Modify: `tests/test_app.c`

- [ ] **Step 1: Add failing app integration tests**

Append to `tests/test_app.c`:

```c
static void test_editor_render_demo_contains_file_text(void)
{
    TideStringBuilder out;
    const char *path = "build/test-app-open.txt";
    FILE *file = fopen(path, "w");
    TIDE_ASSERT(file != NULL);
    fputs("hello\n", file);
    fclose(file);

    TIDE_ASSERT(tide_string_builder_init(&out) == TIDE_OK);
    TIDE_ASSERT(tide_app_render_file_demo(path, 24, 5, &out) == TIDE_OK);
    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "hello") != NULL);
    tide_string_builder_free(&out);
}
```

Call it from `main()`.

- [ ] **Step 2: Run the test and verify it fails**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: build fails because `tide_app_render_file_demo` is not declared.

- [ ] **Step 3: Extend app API and loop**

Add to `include/tide/app.h`:

```c
TideStatus tide_app_render_file_demo(const char *path, size_t width, size_t height, TideStringBuilder *out);
int tide_app_run_file(const char *path);
```

Implementation notes:

- `tide_app_render_file_demo()` loads a buffer, initializes an editor, renders through `tide_editor_render()`, then ANSI-renders the screen into `out`.
- `tide_app_run_file()` is the interactive editor loop:
  - load missing or existing file
  - enable raw mode
  - render editor
  - handle `TIDE_INPUT_TEXT`: insert char
  - handle Enter: newline
  - handle Backspace: backspace
  - handle arrows: move
  - handle Ctrl-S: save and set status to `saved`
  - handle Ctrl-Q/Ctrl-C: quit
  - re-render after changes
  - restore cursor, raw mode, and signals through the existing cleanup path
- `main.c` calls `tide_app_run_file(argv[1])` for a single non-option argument.

- [ ] **Step 4: Run tests and CLI checks**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
./build/tide --version
./build/tide --render-demo
```

Expected: tests pass, existing demo commands still work.

- [ ] **Step 5: Manual terminal smoke**

Run:

```bash
printf 'one\n' > build/manual-editor.txt
./build/tide build/manual-editor.txt
```

Expected:

- The editor opens and displays `one`.
- Typing a printable character inserts it.
- Arrow keys move.
- Ctrl-S saves.
- Ctrl-Q exits.
- Reopen the file and confirm the saved text is present.

- [ ] **Step 6: Commit**

```bash
git add include/tide/app.h src/app.c main.c tests/test_app.c
git commit -m "feat: wire single-file editor into app"
```

## Task 6: Documentation And Full Verification

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Update README**

Add editor usage to `README.md`:

```markdown
## Edit A File

```bash
./build/tide path/to/file.c
```

Interactive keys:

- Printable characters insert text.
- Arrow keys move the cursor.
- Enter inserts a newline.
- Backspace deletes before the cursor.
- Ctrl-S saves.
- Ctrl-Q quits.
```

- [ ] **Step 2: Run full verification**

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

Expected:

- Normal build succeeds.
- Normal CTest run reports all tests pass.
- Sanitizer build succeeds.
- Sanitizer CTest run reports all tests pass.
- `--version` prints `tide 0.1.0`.
- `--render-demo` prints ANSI output containing `tide` and `q: quit`.

- [ ] **Step 3: Commit**

```bash
git add README.md
git commit -m "docs: describe editor core usage"
```

## Self-Review Notes

Spec coverage for this slice:

- Open files: Task 2 and Task 5.
- Edit files: Task 1, Task 3, and Task 5.
- Save local regular files through temporary file and rename: Task 2.
- Maintain cursor, viewport, dirty state, and status messages: Task 3 and Task 4.
- Render editor state through the existing terminal renderer: Task 4 and Task 5.

Known remaining Editor Core work after this plan:

- Undo/redo.
- Search.
- C syntax highlighting.
- Command palette actions.
- Reload behavior.
