# Command Palette v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a minimal command prompt opened with `Ctrl-P` that supports save and quit commands.

**Architecture:** Command prompt state lives in `TideEditor` so rendering and input routing use the same source of truth. The renderer swaps the status line for `:<command>` while prompt mode is active. App-level command dispatch keeps save/quit behavior near the existing terminal loop.

**Tech Stack:** C11, CMake, CTest, POSIX/macOS terminal input.

---

## File Structure

- `include/tide/input.h`: add `TIDE_KEY_CTRL_P`.
- `src/input.c`: parse byte `0x10` as `Ctrl-P`.
- `tests/test_input.c`: cover `Ctrl-P`.
- `include/tide/editor.h`: add command prompt state and prompt editing APIs.
- `src/editor.c`: implement command prompt state transitions and fixed-size command text editing.
- `tests/test_editor.c`: cover prompt open/type/backspace/cancel.
- `src/editor_render.c`: render `:<command>` on the status line while command prompt is active.
- `tests/test_editor_render.c`: cover prompt rendering.
- `include/tide/app.h`: expose app command execution for tests.
- `src/app.c`: route input to command prompt mode and execute `save`, `write`, `w`, `quit`, `q`, and `wq`.
- `tests/test_app.c`: cover app command execution without a real terminal.
- `README.md`: document `Ctrl-P` and commands.

## Task 1: Parse Ctrl-P

**Files:**
- Modify: `include/tide/input.h`
- Modify: `src/input.c`
- Modify: `tests/test_input.c`

- [ ] **Step 1: Write the failing input test**

Append to `tests/test_input.c`:

```c
static void test_ctrl_p_event(void)
{
    TideInputParser parser;
    TideInputEvent event;

    tide_input_parser_init(&parser);
    feed_one(&parser, 0x10, &event);

    TIDE_ASSERT(event.type == TIDE_INPUT_KEY);
    TIDE_ASSERT(event.key == TIDE_KEY_CTRL_P);
}
```

Call it from `main()`:

```c
int main(void)
{
    test_printable_text_event();
    test_ctrl_s_event();
    test_ctrl_p_event();
    test_arrow_left_event_can_arrive_across_reads();
    test_bare_escape_flushes_as_escape_key();
    return 0;
}
```

- [ ] **Step 2: Verify the test fails**

Run:

```bash
cmake --build build
```

Expected: build fails because `TIDE_KEY_CTRL_P` is not declared.

- [ ] **Step 3: Implement Ctrl-P parsing**

Add `TIDE_KEY_CTRL_P` to `TideKey` in `include/tide/input.h`, after `TIDE_KEY_CTRL_S`.

Add this case in `tide_input_feed()` in `src/input.c`:

```c
    case 0x10:
        return emit_key(event, TIDE_KEY_CTRL_P);
```

- [ ] **Step 4: Verify green**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 5: Commit**

```bash
git add include/tide/input.h src/input.c tests/test_input.c
git commit -m "feat: parse command prompt shortcut"
```

## Task 2: Editor Command Prompt State

**Files:**
- Modify: `include/tide/editor.h`
- Modify: `src/editor.c`
- Modify: `tests/test_editor.c`

- [ ] **Step 1: Write failing editor prompt tests**

Append to `tests/test_editor.c`:

```c
static void test_command_prompt_collects_text_without_editing_buffer(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    tide_editor_open_command_prompt(&editor);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'w') == TIDE_OK);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'q') == TIDE_OK);

    TIDE_ASSERT(tide_editor_command_active(&editor));
    TIDE_ASSERT_STR_EQ(tide_editor_command_text(&editor), "wq");
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "");
    tide_buffer_free(&buffer);
}

static void test_command_prompt_backspace_and_cancel(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    tide_editor_open_command_prompt(&editor);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'q') == TIDE_OK);
    tide_editor_command_backspace(&editor);
    TIDE_ASSERT_STR_EQ(tide_editor_command_text(&editor), "");
    tide_editor_cancel_command_prompt(&editor);

    TIDE_ASSERT(!tide_editor_command_active(&editor));
    tide_buffer_free(&buffer);
}
```

Call both tests from `main()`.

- [ ] **Step 2: Verify the tests fail**

Run:

```bash
cmake --build build
```

Expected: build fails because command prompt editor APIs are not declared.

- [ ] **Step 3: Add editor prompt API**

Add to `include/tide/editor.h`:

```c
#define TIDE_EDITOR_COMMAND_CAPACITY 128

typedef enum TideEditorPromptMode {
    TIDE_EDITOR_PROMPT_CLOSED = 0,
    TIDE_EDITOR_PROMPT_COMMAND
} TideEditorPromptMode;
```

Add these fields to `TideEditor`:

```c
    TideEditorPromptMode prompt_mode;
    char command[TIDE_EDITOR_COMMAND_CAPACITY];
    size_t command_length;
```

Add these declarations:

```c
void tide_editor_open_command_prompt(TideEditor *editor);
void tide_editor_cancel_command_prompt(TideEditor *editor);
int tide_editor_command_active(const TideEditor *editor);
TideStatus tide_editor_command_insert_char(TideEditor *editor, char ch);
void tide_editor_command_backspace(TideEditor *editor);
const char *tide_editor_command_text(const TideEditor *editor);
```

- [ ] **Step 4: Implement editor prompt behavior**

In `tide_editor_init()`, initialize prompt state:

```c
    editor->prompt_mode = TIDE_EDITOR_PROMPT_CLOSED;
    editor->command[0] = '\0';
    editor->command_length = 0;
```

Append to `src/editor.c`:

```c
void tide_editor_open_command_prompt(TideEditor *editor)
{
    editor->prompt_mode = TIDE_EDITOR_PROMPT_COMMAND;
    editor->command[0] = '\0';
    editor->command_length = 0;
    tide_editor_set_status(editor, "");
}

void tide_editor_cancel_command_prompt(TideEditor *editor)
{
    editor->prompt_mode = TIDE_EDITOR_PROMPT_CLOSED;
    editor->command[0] = '\0';
    editor->command_length = 0;
}

int tide_editor_command_active(const TideEditor *editor)
{
    return editor->prompt_mode == TIDE_EDITOR_PROMPT_COMMAND;
}

TideStatus tide_editor_command_insert_char(TideEditor *editor, char ch)
{
    if (!tide_editor_command_active(editor)) {
        return TIDE_ERR_INVALID;
    }
    if (editor->command_length + 1 >= sizeof(editor->command)) {
        return TIDE_OK;
    }

    editor->command[editor->command_length++] = ch;
    editor->command[editor->command_length] = '\0';
    return TIDE_OK;
}

void tide_editor_command_backspace(TideEditor *editor)
{
    if (!tide_editor_command_active(editor) || editor->command_length == 0) {
        return;
    }

    editor->command_length--;
    editor->command[editor->command_length] = '\0';
}

const char *tide_editor_command_text(const TideEditor *editor)
{
    return editor->command;
}
```

- [ ] **Step 5: Verify green**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 6: Commit**

```bash
git add include/tide/editor.h src/editor.c tests/test_editor.c
git commit -m "feat: add editor command prompt state"
```

## Task 3: Render Command Prompt

**Files:**
- Modify: `src/editor_render.c`
- Modify: `tests/test_editor_render.c`

- [ ] **Step 1: Write failing render test**

Append to `tests/test_editor_render.c`:

```c
static void test_editor_render_draws_command_prompt_on_status_line(void)
{
    TideBuffer buffer;
    TideEditor editor;
    TideScreen screen;
    TideCell cell;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    tide_editor_open_command_prompt(&editor);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'w') == TIDE_OK);

    TIDE_ASSERT(tide_screen_init(&screen, 20, 4) == TIDE_OK);
    TIDE_ASSERT(tide_editor_render(&editor, &screen) == TIDE_OK);

    TIDE_ASSERT(tide_screen_get(&screen, 0, 3, &cell) == TIDE_OK);
    TIDE_ASSERT(cell.ch == ':');
    TIDE_ASSERT(tide_screen_get(&screen, 1, 3, &cell) == TIDE_OK);
    TIDE_ASSERT(cell.ch == 'w');

    tide_screen_free(&screen);
    tide_buffer_free(&buffer);
}
```

Call it from `main()`.

- [ ] **Step 2: Verify the test fails**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `test_editor_render` fails because the status line still starts with `[`.

- [ ] **Step 3: Render prompt text**

In `draw_status()` in `src/editor_render.c`, after filling the status line and before drawing file metadata, add:

```c
    if (tide_editor_command_active(editor)) {
        char prompt[sizeof(editor->command) + 2];
        snprintf(prompt, sizeof(prompt), ":%s", tide_editor_command_text(editor));
        return draw_text(screen, 0, status_y, prompt, status_cell);
    }
```

- [ ] **Step 4: Verify green**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 5: Commit**

```bash
git add src/editor_render.c tests/test_editor_render.c
git commit -m "feat: render editor command prompt"
```

## Task 4: App Command Dispatch

**Files:**
- Modify: `include/tide/app.h`
- Modify: `src/app.c`
- Modify: `tests/test_app.c`

- [ ] **Step 1: Write failing app command tests**

Append to `tests/test_app.c`:

```c
static void test_write_command_saves_buffer(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;
    const char *path = "test-app-command-write.txt";

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_set_path(&buffer, path) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'x') == TIDE_OK);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "write", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT(buffer.dirty == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "saved");
    tide_buffer_free(&buffer);
}

static void test_unknown_command_stays_open_and_sets_status(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "bogus", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "unknown command: bogus");
    tide_buffer_free(&buffer);
}
```

Add `#include "tide/buffer.h"` and `#include "tide/editor.h"` to `tests/test_app.c`, then call both tests from `main()`.

- [ ] **Step 2: Verify the tests fail**

Run:

```bash
cmake --build build
```

Expected: build fails because `tide_app_execute_editor_command` is not declared.

- [ ] **Step 3: Expose command execution API**

Add `#include "tide/editor.h"` to `include/tide/app.h`.

Add this declaration:

```c
TideStatus tide_app_execute_editor_command(TideEditor *editor, const char *command, int *quit);
```

- [ ] **Step 4: Implement command execution**

Add this helper and function to `src/app.c`:

```c
static int command_matches(const char *command, const char *a, const char *b)
{
    return strcmp(command, a) == 0 || strcmp(command, b) == 0;
}

TideStatus tide_app_execute_editor_command(TideEditor *editor, const char *command, int *quit)
{
    *quit = 0;

    if (command_matches(command, "save", "write") || strcmp(command, "w") == 0) {
        TideStatus status = tide_buffer_save(editor->buffer);
        tide_editor_set_status(editor, status == TIDE_OK ? "saved" : tide_status_string(status));
        return TIDE_OK;
    }

    if (command_matches(command, "quit", "q")) {
        *quit = 1;
        return TIDE_OK;
    }

    if (strcmp(command, "wq") == 0) {
        TideStatus status = tide_buffer_save(editor->buffer);
        tide_editor_set_status(editor, status == TIDE_OK ? "saved" : tide_status_string(status));
        if (status == TIDE_OK) {
            *quit = 1;
        }
        return TIDE_OK;
    }

    char message[sizeof(editor->status)];
    snprintf(message, sizeof(message), "unknown command: %s", command);
    tide_editor_set_status(editor, message);
    return TIDE_OK;
}
```

- [ ] **Step 5: Route prompt input in the app loop**

In `handle_editor_event()`:

- If command prompt is active:
  - text inserts prompt characters.
  - Backspace edits prompt text.
  - Enter copies `tide_editor_command_text()`, cancels prompt mode, then calls `tide_app_execute_editor_command()`.
  - Escape or `Ctrl-P` cancels prompt mode.
  - Other keys are ignored.
- If command prompt is closed:
  - `Ctrl-P` opens prompt mode.
  - existing editing, save, movement, and quit behavior remains.

In `tide_app_run_file()`, when `read()` returns `0`, call `tide_input_flush()` and handle any emitted event. This lets a bare Escape cancel command mode after the terminal timeout.

- [ ] **Step 6: Verify green**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 7: Manual PTY smoke**

Run:

```bash
./build/tide build/manual-command-palette.txt
```

Then feed:

- `Ctrl-P`
- `wq`
- `Enter`

Expected: the editor saves and exits. The file exists and contains the current buffer content.

- [ ] **Step 8: Commit**

```bash
git add include/tide/app.h src/app.c tests/test_app.c
git commit -m "feat: execute editor commands from prompt"
```

## Task 5: Documentation And Full Verification

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Update README**

Add under interactive keys:

```markdown
- Ctrl-P opens the command prompt.

Command prompt:

- `write`, `save`, or `w` saves.
- `quit` or `q` quits.
- `wq` saves and quits.
- Escape or Ctrl-P closes the prompt.
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

Expected: all commands succeed; both CTest runs report `100% tests passed`.

- [ ] **Step 3: Commit**

```bash
git add README.md
git commit -m "docs: describe command prompt usage"
```

## Self-Review Notes

- `Ctrl-P` parsing is covered by Task 1.
- Prompt state and prompt text editing are covered by Task 2.
- Prompt status-line rendering is covered by Task 3.
- Save/quit command dispatch and input routing are covered by Task 4.
- Usage documentation and complete verification are covered by Task 5.
