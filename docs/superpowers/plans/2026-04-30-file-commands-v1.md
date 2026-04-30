# File Commands v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add command-prompt `open <path>` and `reload` support that safely replaces the current buffer without losing unsaved work.

**Architecture:** `TideBuffer` gets a safe replacement API that loads into a temporary buffer before swapping. `TideEditor` gets a reset API that clears cursor, viewport, command, search, and undo/redo state while keeping the same buffer pointer. `tide_app_execute_editor_command()` owns command parsing, dirty-buffer guards, status messages, and composing buffer replacement with editor reset.

**Tech Stack:** C11, CMake, CTest, existing `TideBuffer`, `TideEditor`, and command prompt dispatch.

---

## File Structure

- `include/tide/buffer.h`: declare `tide_buffer_replace_with_file()`.
- `src/buffer.c`: implement safe buffer replacement through a temporary `TideBuffer`.
- `tests/test_buffer.c`: cover successful replacement and failed replacement preserving the original buffer.
- `include/tide/editor.h`: declare `tide_editor_reset_view()`.
- `src/editor.c`: implement reset by reusing editor initialization semantics without replacing the buffer pointer.
- `tests/test_editor.c`: cover reset clearing cursor, viewport, search, and undo history.
- `src/app.c`: dispatch `open <path>` and `reload`, enforce dirty guards, and set statuses.
- `tests/test_app.c`: cover open/reload success and refusal paths.
- `README.md`: document `open <path>` and `reload`.

## Task 1: Safe Buffer Replacement

**Files:**
- Modify: `include/tide/buffer.h`
- Modify: `src/buffer.c`
- Modify: `tests/test_buffer.c`

- [ ] **Step 1: Write failing buffer replacement tests**

Append to `tests/test_buffer.c` before `main()`:

```c
static void test_replace_with_file_swaps_content_and_path(void)
{
    TideBuffer buffer;
    const char *old_path = "test-buffer-replace-old.txt";
    const char *new_path = "test-buffer-replace-new.txt";
    FILE *file = fopen(new_path, "w");
    TIDE_ASSERT(file != NULL);
    fputs("one\ntwo", file);
    fclose(file);

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_set_path(&buffer, old_path) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 0, 0, 'x') == TIDE_OK);

    TIDE_ASSERT(tide_buffer_replace_with_file(&buffer, new_path) == TIDE_OK);

    TIDE_ASSERT(buffer.line_count == 2);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "one");
    TIDE_ASSERT_STR_EQ(buffer.lines[1].data, "two");
    TIDE_ASSERT_STR_EQ(buffer.path, new_path);
    TIDE_ASSERT(buffer.dirty == 0);
    tide_buffer_free(&buffer);
}

static void test_replace_with_file_keeps_original_on_failure(void)
{
    TideBuffer buffer;
    const char *old_path = "test-buffer-replace-keep.txt";

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_set_path(&buffer, old_path) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 0, 0, 'x') == TIDE_OK);

    TIDE_ASSERT(tide_buffer_replace_with_file(&buffer, ".") == TIDE_ERR_IO);

    TIDE_ASSERT(buffer.line_count == 1);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "x");
    TIDE_ASSERT_STR_EQ(buffer.path, old_path);
    TIDE_ASSERT(buffer.dirty == 1);
    tide_buffer_free(&buffer);
}
```

Call both from `main()`:

```c
    test_replace_with_file_swaps_content_and_path();
    test_replace_with_file_keeps_original_on_failure();
```

- [ ] **Step 2: Verify the tests fail**

Run:

```bash
cmake --build build
```

Expected: build fails because `tide_buffer_replace_with_file()` is not declared.

- [ ] **Step 3: Add the buffer replacement API**

Add to `include/tide/buffer.h` after `tide_buffer_load_file()`:

```c
TideStatus tide_buffer_replace_with_file(TideBuffer *buffer, const char *path);
```

Add to `src/buffer.c` after `tide_buffer_load_file()`:

```c
TideStatus tide_buffer_replace_with_file(TideBuffer *buffer, const char *path)
{
    TideBuffer replacement;
    TideStatus status = tide_buffer_load_file(&replacement, path);
    if (status != TIDE_OK) {
        return status;
    }

    TideBuffer old = *buffer;
    *buffer = replacement;
    tide_buffer_free(&old);
    return TIDE_OK;
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
git add include/tide/buffer.h src/buffer.c tests/test_buffer.c
git commit -m "feat: add safe buffer file replacement"
```

## Task 2: Editor Reset State

**Files:**
- Modify: `include/tide/editor.h`
- Modify: `src/editor.c`
- Modify: `tests/test_editor.c`

- [ ] **Step 1: Write failing editor reset test**

Append to `tests/test_editor.c` before `main()`:

```c
static void test_reset_view_clears_editor_state(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    insert_text(&editor, "abc");
    editor.cursor = (TideBufferPosition){0, 0};
    TIDE_ASSERT(tide_editor_find(&editor, "b") == TIDE_OK);
    TIDE_ASSERT(tide_editor_search_has_match(&editor));
    editor.viewport_line = 3;
    editor.viewport_column = 2;
    tide_editor_set_status(&editor, "status");

    tide_editor_reset_view(&editor);

    TIDE_ASSERT(editor.buffer == &buffer);
    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 0);
    TIDE_ASSERT(editor.viewport_line == 0);
    TIDE_ASSERT(editor.viewport_column == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "");
    TIDE_ASSERT(!tide_editor_search_has_match(&editor));
    TIDE_ASSERT(tide_editor_undo(&editor) == TIDE_OK);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "abc");
    TIDE_ASSERT_STR_EQ(editor.status, "nothing to undo");
    tide_buffer_free(&buffer);
}
```

Call it from `main()`:

```c
    test_reset_view_clears_editor_state();
```

- [ ] **Step 2: Verify the test fails**

Run:

```bash
cmake --build build
```

Expected: build fails because `tide_editor_reset_view()` is not declared.

- [ ] **Step 3: Add editor reset API**

Add to `include/tide/editor.h` after `tide_editor_init()`:

```c
void tide_editor_reset_view(TideEditor *editor);
```

In `src/editor.c`, replace the body of `tide_editor_init()` with:

```c
void tide_editor_init(TideEditor *editor, TideBuffer *buffer)
{
    editor->buffer = buffer;
    tide_editor_reset_view(editor);
}
```

Add this function after `tide_editor_init()`:

```c
void tide_editor_reset_view(TideEditor *editor)
{
    editor->cursor = (TideBufferPosition){0, 0};
    editor->viewport_line = 0;
    editor->viewport_column = 0;
    editor->status[0] = '\0';
    editor->prompt_mode = TIDE_EDITOR_PROMPT_CLOSED;
    editor->command[0] = '\0';
    editor->command_length = 0;
    editor->search_query[0] = '\0';
    editor->search_query_length = 0;
    editor->search_match = (TideBufferPosition){0, 0};
    editor->search_match_length = 0;
    editor->search_has_match = 0;
    editor->undo_count = 0;
    editor->redo_count = 0;
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
git add include/tide/editor.h src/editor.c tests/test_editor.c
git commit -m "feat: add editor view reset"
```

## Task 3: Open and Reload Commands

**Files:**
- Modify: `src/app.c`
- Modify: `tests/test_app.c`

- [ ] **Step 1: Write failing app command tests**

Add this helper near the top of `tests/test_app.c` after includes:

```c
static void write_text_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");
    TIDE_ASSERT(file != NULL);
    fputs(text, file);
    fclose(file);
}
```

Append to `tests/test_app.c` before `main()`:

```c
static void test_open_command_loads_file_and_resets_editor(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;
    const char *old_path = "test-app-open-command-old.txt";
    const char *new_path = "test-app-open-command-new.txt";

    write_text_file(old_path, "old");
    write_text_file(new_path, "one\ntwo");
    TIDE_ASSERT(tide_buffer_load_file(&buffer, old_path) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    editor.cursor = (TideBufferPosition){0, 2};
    editor.viewport_line = 4;
    editor.viewport_column = 3;

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "open test-app-open-command-new.txt", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(buffer.path, new_path);
    TIDE_ASSERT(buffer.line_count == 2);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "one");
    TIDE_ASSERT_STR_EQ(buffer.lines[1].data, "two");
    TIDE_ASSERT(buffer.dirty == 0);
    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 0);
    TIDE_ASSERT(editor.viewport_line == 0);
    TIDE_ASSERT(editor.viewport_column == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "opened: test-app-open-command-new.txt");
    tide_buffer_free(&buffer);
}

static void test_open_command_refuses_dirty_buffer(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'x') == TIDE_OK);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "open test-app-open-refuse.txt", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "x");
    TIDE_ASSERT_STR_EQ(editor.status, "unsaved changes; write first");
    tide_buffer_free(&buffer);
}

static void test_open_command_requires_path(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "open   ", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "path required");
    tide_buffer_free(&buffer);
}

static void test_reload_command_reloads_current_file_and_resets_editor(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;
    const char *path = "test-app-reload-command.txt";

    write_text_file(path, "old");
    TIDE_ASSERT(tide_buffer_load_file(&buffer, path) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    editor.cursor = (TideBufferPosition){0, 2};
    editor.viewport_line = 4;
    write_text_file(path, "new");

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "reload", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(buffer.path, path);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "new");
    TIDE_ASSERT(buffer.dirty == 0);
    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 0);
    TIDE_ASSERT(editor.viewport_line == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "reloaded");
    tide_buffer_free(&buffer);
}

static void test_reload_command_refuses_dirty_buffer(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;
    const char *path = "test-app-reload-refuse.txt";

    write_text_file(path, "old");
    TIDE_ASSERT(tide_buffer_load_file(&buffer, path) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'x') == TIDE_OK);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "reload", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "xold");
    TIDE_ASSERT_STR_EQ(editor.status, "unsaved changes; write first");
    tide_buffer_free(&buffer);
}

static void test_reload_command_requires_file_path(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "reload", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "no file to reload");
    tide_buffer_free(&buffer);
}
```

Call all six tests from `main()`.

- [ ] **Step 2: Verify the tests fail**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `test_app` fails because `open` and `reload` are unknown commands.

- [ ] **Step 3: Implement command helpers and dispatch**

In `src/app.c`, add these static helpers after `skip_command_spaces()`:

```c
static TideStatus replace_editor_file(TideEditor *editor, const char *path, const char *success_status)
{
    if (editor->buffer->dirty) {
        tide_editor_set_status(editor, "unsaved changes; write first");
        return TIDE_OK;
    }

    TideStatus status = tide_buffer_replace_with_file(editor->buffer, path);
    if (status != TIDE_OK) {
        tide_editor_set_status(editor, tide_status_string(status));
        return TIDE_OK;
    }

    tide_editor_reset_view(editor);
    tide_editor_set_status(editor, success_status);
    return TIDE_OK;
}

static TideStatus open_editor_file(TideEditor *editor, const char *path)
{
    if (path[0] == '\0') {
        tide_editor_set_status(editor, "path required");
        return TIDE_OK;
    }

    if (editor->buffer->dirty) {
        tide_editor_set_status(editor, "unsaved changes; write first");
        return TIDE_OK;
    }

    TideStatus status = tide_buffer_replace_with_file(editor->buffer, path);
    if (status != TIDE_OK) {
        tide_editor_set_status(editor, tide_status_string(status));
        return TIDE_OK;
    }

    tide_editor_reset_view(editor);
    char message[sizeof(editor->status)];
    snprintf(message, sizeof(message), "opened: %s", path);
    tide_editor_set_status(editor, message);
    return TIDE_OK;
}

static TideStatus reload_editor_file(TideEditor *editor)
{
    if (editor->buffer->path == NULL) {
        tide_editor_set_status(editor, "no file to reload");
        return TIDE_OK;
    }

    return replace_editor_file(editor, editor->buffer->path, "reloaded");
}
```

In `tide_app_execute_editor_command()`, before the unknown-command branch, add:

```c
    if (strncmp(command, "open", 4) == 0 && (command[4] == '\0' || command[4] == ' ' || command[4] == '\t')) {
        const char *path = skip_command_spaces(command + 4);
        return open_editor_file(editor, path);
    }

    if (strcmp(command, "reload") == 0) {
        return reload_editor_file(editor);
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
git add src/app.c tests/test_app.c
git commit -m "feat: add open and reload commands"
```

## Task 4: Documentation and Full Verification

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Document file commands**

In `README.md`, add to the command prompt list:

```markdown
- `open <path>` opens a file in the current editor.
- `reload` reloads the current file from disk.
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

- [ ] **Step 3: Run manual PTY smoke**

Run:

```bash
./build/tide build/manual-file-commands-a.txt
```

Type:

```text
ab
Ctrl-P
open build/manual-file-commands-b.txt
Enter
Ctrl-P
write
Enter
Ctrl-P
open build/manual-file-commands-b.txt
Enter
Ctrl-P
reload
Enter
Ctrl-P
q
Enter
```

Expected: dirty `open` is refused until `write`; after the second `open`, the editor switches to `build/manual-file-commands-b.txt`; `reload` keeps that file open and exits cleanly.

- [ ] **Step 4: Commit**

```bash
git add README.md
git commit -m "docs: describe file commands"
```

## Coverage Checklist

- `open <path>` success is covered by Task 3.
- `open <path>` dirty refusal is covered by Task 3.
- `open` path validation is covered by Task 3.
- `reload` success is covered by Task 3.
- `reload` dirty refusal is covered by Task 3.
- `reload` path validation is covered by Task 3.
- Safe failed replacement is covered by Task 1.
- Editor reset of cursor, viewport, search, and undo/redo is covered by Task 2.
