# Terminal C IDE Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build Milestone 1 of the terminal C IDE: a tested C project skeleton with raw terminal support, normalized input events, virtual screen rendering, ANSI output, basic layout primitives, and a small executable demo.

**Architecture:** The first milestone creates the foundation modules that later editor and IDE features will use. Each module has a narrow public header under `include/tide/`, implementation under `src/`, and focused C tests under `tests/`.

**Tech Stack:** C11, CMake, CTest, POSIX/macOS syscalls, standard C library only. No `ncurses`, terminal UI libraries, JSON libraries, async libraries, editor libraries, or LSP client libraries.

---

## Scope

This plan implements only Foundation from the design spec. It intentionally does not implement file buffers, editing operations, syntax highlighting, workbench panels, build integration, LSP, debugger hooks, themes, or persistent sessions.

Completion target:

- `cmake -S . -B build`
- `cmake --build build`
- `ctest --test-dir build --output-on-failure`
- `cmake -S . -B build-asan -DTIDE_ENABLE_SANITIZERS=ON`
- `cmake --build build-asan`
- `ctest --test-dir build-asan --output-on-failure`
- `./build/tide --render-demo`
- Manual smoke: `./build/tide`, press `q`, terminal returns to normal.

## File Structure

Create this project structure:

- `CMakeLists.txt`: root CMake config, sanitizer option, `tide` executable, tests.
- `include/tide/status.h`: shared status enum and error strings.
- `include/tide/rect.h`: rectangular layout primitives.
- `include/tide/input.h`: terminal byte parser and normalized input events.
- `include/tide/screen.h`: virtual screen cells, dirty tracking, resize/clear/set operations.
- `include/tide/string_builder.h`: deterministic in-memory output builder for renderer tests.
- `include/tide/ansi.h`: ANSI renderer from virtual screen to byte stream.
- `include/tide/terminal.h`: terminal raw-mode lifecycle and pure termios transformation.
- `include/tide/app.h`: foundation application shell and render demo.
- `src/status.c`: status string implementation.
- `src/rect.c`: layout primitive implementation.
- `src/input.c`: escape sequence and key parser.
- `src/screen.c`: virtual screen implementation.
- `src/string_builder.c`: append-only dynamic string builder.
- `src/ansi.c`: ANSI renderer implementation.
- `src/terminal.c`: macOS/POSIX terminal wrapper.
- `src/app.c`: app shell, demo rendering, interactive quit loop.
- `main.c`: CLI entry point.
- `tests/CMakeLists.txt`: test target registration.
- `tests/test_support.h`: tiny assertion helpers.
- `tests/test_status.c`: status tests.
- `tests/test_rect.c`: rectangle/layout tests.
- `tests/test_input.c`: input parser tests.
- `tests/test_screen.c`: screen buffer tests.
- `tests/test_ansi.c`: ANSI renderer tests.
- `tests/test_terminal.c`: pure termios transformation tests.
- `tests/test_app.c`: demo rendering tests.

## Task 1: Build Skeleton And Shared Status

**Files:**
- Create: `CMakeLists.txt`
- Create: `include/tide/status.h`
- Create: `src/status.c`
- Create: `tests/CMakeLists.txt`
- Create: `tests/test_support.h`
- Create: `tests/test_status.c`

- [ ] **Step 1: Write the failing status test**

Create `tests/test_support.h`:

```c
#ifndef TIDE_TEST_SUPPORT_H
#define TIDE_TEST_SUPPORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TIDE_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "%s:%d: assertion failed: %s\n", __FILE__, __LINE__, #expr); \
            exit(1); \
        } \
    } while (0)

#define TIDE_ASSERT_STR_EQ(actual, expected) \
    do { \
        const char *actual_value = (actual); \
        const char *expected_value = (expected); \
        if (strcmp(actual_value, expected_value) != 0) { \
            fprintf(stderr, "%s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, expected_value, actual_value); \
            exit(1); \
        } \
    } while (0)

#endif
```

Create `tests/test_status.c`:

```c
#include "tide/status.h"
#include "test_support.h"

static void test_status_strings_are_stable(void)
{
    TIDE_ASSERT_STR_EQ(tide_status_string(TIDE_OK), "ok");
    TIDE_ASSERT_STR_EQ(tide_status_string(TIDE_ERR_ALLOC), "allocation failed");
    TIDE_ASSERT_STR_EQ(tide_status_string(TIDE_ERR_IO), "i/o failed");
    TIDE_ASSERT_STR_EQ(tide_status_string(TIDE_ERR_INVALID), "invalid input");
    TIDE_ASSERT_STR_EQ(tide_status_string(TIDE_ERR_UNSUPPORTED), "unsupported operation");
}

int main(void)
{
    test_status_strings_are_stable();
    return 0;
}
```

- [ ] **Step 2: Run the test and verify it fails to build**

Run:

```bash
cmake -S . -B build
cmake --build build
```

Expected: configure or build fails because there is no CMake project and `tide/status.h` does not exist.

- [ ] **Step 3: Add the minimal project skeleton**

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(tide C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(TIDE_ENABLE_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(tide_core
    src/status.c
)

target_include_directories(tide_core PUBLIC include)
target_compile_options(tide_core PRIVATE -Wall -Wextra -Werror -g -O0)

if(TIDE_ENABLE_SANITIZERS)
    target_compile_options(tide_core PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(tide_core PUBLIC -fsanitize=address,undefined)
endif()

add_executable(tide main.c)
target_link_libraries(tide PRIVATE tide_core)
target_compile_options(tide PRIVATE -Wall -Wextra -Werror -g -O0)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
```

Create `include/tide/status.h`:

```c
#ifndef TIDE_STATUS_H
#define TIDE_STATUS_H

typedef enum TideStatus {
    TIDE_OK = 0,
    TIDE_ERR_ALLOC,
    TIDE_ERR_IO,
    TIDE_ERR_INVALID,
    TIDE_ERR_UNSUPPORTED
} TideStatus;

const char *tide_status_string(TideStatus status);

#endif
```

Create `src/status.c`:

```c
#include "tide/status.h"

const char *tide_status_string(TideStatus status)
{
    switch (status) {
    case TIDE_OK:
        return "ok";
    case TIDE_ERR_ALLOC:
        return "allocation failed";
    case TIDE_ERR_IO:
        return "i/o failed";
    case TIDE_ERR_INVALID:
        return "invalid input";
    case TIDE_ERR_UNSUPPORTED:
        return "unsupported operation";
    }

    return "unknown error";
}
```

Create `main.c`:

```c
#include <stdio.h>

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("tide foundation");
    return 0;
}
```

Create `tests/CMakeLists.txt`:

```cmake
function(add_tide_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE tide_core)
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(${name} PRIVATE -Wall -Wextra -Werror -g -O0)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_tide_test(test_status test_status.c)
```

- [ ] **Step 4: Run tests and verify they pass**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `100% tests passed`.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt main.c include/tide/status.h src/status.c tests/CMakeLists.txt tests/test_support.h tests/test_status.c
git commit -m "feat: add tide project skeleton"
```

## Task 2: Rectangle Layout Primitives

**Files:**
- Create: `include/tide/rect.h`
- Create: `src/rect.c`
- Create: `tests/test_rect.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing rectangle tests**

Create `tests/test_rect.c`:

```c
#include "tide/rect.h"
#include "test_support.h"

static void test_empty_rect_detection(void)
{
    TIDE_ASSERT(tide_rect_is_empty((TideRect){0, 0, 0, 10}));
    TIDE_ASSERT(tide_rect_is_empty((TideRect){0, 0, 10, 0}));
    TIDE_ASSERT(!tide_rect_is_empty((TideRect){1, 2, 3, 4}));
}

static void test_clip_intersection(void)
{
    TideRect a = {2, 3, 10, 8};
    TideRect b = {5, 1, 6, 6};
    TideRect out = tide_rect_clip(a, b);

    TIDE_ASSERT(out.x == 5);
    TIDE_ASSERT(out.y == 3);
    TIDE_ASSERT(out.width == 6);
    TIDE_ASSERT(out.height == 4);
}

static void test_split_bottom(void)
{
    TideRect top = {0, 0, 0, 0};
    TideRect bottom = {0, 0, 0, 0};

    tide_rect_split_bottom((TideRect){0, 0, 80, 24}, 2, &top, &bottom);

    TIDE_ASSERT(top.x == 0);
    TIDE_ASSERT(top.y == 0);
    TIDE_ASSERT(top.width == 80);
    TIDE_ASSERT(top.height == 22);
    TIDE_ASSERT(bottom.x == 0);
    TIDE_ASSERT(bottom.y == 22);
    TIDE_ASSERT(bottom.width == 80);
    TIDE_ASSERT(bottom.height == 2);
}

int main(void)
{
    test_empty_rect_detection();
    test_clip_intersection();
    test_split_bottom();
    return 0;
}
```

Modify `tests/CMakeLists.txt`:

```cmake
add_tide_test(test_rect test_rect.c)
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```bash
cmake --build build
```

Expected: build fails because `tide/rect.h` does not exist.

- [ ] **Step 3: Add rectangle API and implementation**

Create `include/tide/rect.h`:

```c
#ifndef TIDE_RECT_H
#define TIDE_RECT_H

#include <stddef.h>

typedef struct TideRect {
    size_t x;
    size_t y;
    size_t width;
    size_t height;
} TideRect;

int tide_rect_is_empty(TideRect rect);
TideRect tide_rect_clip(TideRect a, TideRect b);
void tide_rect_split_bottom(TideRect rect, size_t bottom_height, TideRect *top, TideRect *bottom);

#endif
```

Create `src/rect.c`:

```c
#include "tide/rect.h"

static size_t max_size(size_t a, size_t b)
{
    return a > b ? a : b;
}

static size_t min_size(size_t a, size_t b)
{
    return a < b ? a : b;
}

int tide_rect_is_empty(TideRect rect)
{
    return rect.width == 0 || rect.height == 0;
}

TideRect tide_rect_clip(TideRect a, TideRect b)
{
    size_t left = max_size(a.x, b.x);
    size_t top = max_size(a.y, b.y);
    size_t right = min_size(a.x + a.width, b.x + b.width);
    size_t bottom = min_size(a.y + a.height, b.y + b.height);

    if (right <= left || bottom <= top) {
        return (TideRect){left, top, 0, 0};
    }

    return (TideRect){left, top, right - left, bottom - top};
}

void tide_rect_split_bottom(TideRect rect, size_t bottom_height, TideRect *top, TideRect *bottom)
{
    if (bottom_height > rect.height) {
        bottom_height = rect.height;
    }

    *top = (TideRect){rect.x, rect.y, rect.width, rect.height - bottom_height};
    *bottom = (TideRect){rect.x, rect.y + top->height, rect.width, bottom_height};
}
```

Modify `CMakeLists.txt`:

```cmake
add_library(tide_core
    src/status.c
    src/rect.c
)
```

- [ ] **Step 4: Run tests and verify they pass**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `test_status` and `test_rect` pass.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt include/tide/rect.h src/rect.c tests/CMakeLists.txt tests/test_rect.c
git commit -m "feat: add rectangle layout primitives"
```

## Task 3: Input Parser

**Files:**
- Create: `include/tide/input.h`
- Create: `src/input.c`
- Create: `tests/test_input.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing input parser tests**

Create `tests/test_input.c` with tests for printable text, Ctrl-S, arrow-left, split escape sequences, and bare Escape:

```c
#include "tide/input.h"
#include "test_support.h"

static void feed_one(TideInputParser *parser, unsigned char byte, TideInputEvent *event)
{
    TideInputResult result = tide_input_feed(parser, byte, event);
    TIDE_ASSERT(result == TIDE_INPUT_EVENT);
}

static void test_printable_text_event(void)
{
    TideInputParser parser;
    TideInputEvent event;

    tide_input_parser_init(&parser);
    feed_one(&parser, 'x', &event);

    TIDE_ASSERT(event.type == TIDE_INPUT_TEXT);
    TIDE_ASSERT(event.text == 'x');
}

static void test_ctrl_s_event(void)
{
    TideInputParser parser;
    TideInputEvent event;

    tide_input_parser_init(&parser);
    feed_one(&parser, 0x13, &event);

    TIDE_ASSERT(event.type == TIDE_INPUT_KEY);
    TIDE_ASSERT(event.key == TIDE_KEY_CTRL_S);
}

static void test_arrow_left_event_can_arrive_across_reads(void)
{
    TideInputParser parser;
    TideInputEvent event;

    tide_input_parser_init(&parser);
    TIDE_ASSERT(tide_input_feed(&parser, 0x1b, &event) == TIDE_INPUT_PENDING);
    TIDE_ASSERT(tide_input_feed(&parser, '[', &event) == TIDE_INPUT_PENDING);
    TIDE_ASSERT(tide_input_feed(&parser, 'D', &event) == TIDE_INPUT_EVENT);
    TIDE_ASSERT(event.type == TIDE_INPUT_KEY);
    TIDE_ASSERT(event.key == TIDE_KEY_ARROW_LEFT);
}

static void test_bare_escape_flushes_as_escape_key(void)
{
    TideInputParser parser;
    TideInputEvent event;

    tide_input_parser_init(&parser);
    TIDE_ASSERT(tide_input_feed(&parser, 0x1b, &event) == TIDE_INPUT_PENDING);
    TIDE_ASSERT(tide_input_flush(&parser, &event) == TIDE_INPUT_EVENT);
    TIDE_ASSERT(event.type == TIDE_INPUT_KEY);
    TIDE_ASSERT(event.key == TIDE_KEY_ESCAPE);
}

int main(void)
{
    test_printable_text_event();
    test_ctrl_s_event();
    test_arrow_left_event_can_arrive_across_reads();
    test_bare_escape_flushes_as_escape_key();
    return 0;
}
```

Modify `tests/CMakeLists.txt`:

```cmake
add_tide_test(test_input test_input.c)
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```bash
cmake --build build
```

Expected: build fails because `tide/input.h` does not exist.

- [ ] **Step 3: Add input parser API and implementation**

Create `include/tide/input.h`:

```c
#ifndef TIDE_INPUT_H
#define TIDE_INPUT_H

typedef enum TideInputResult {
    TIDE_INPUT_NONE = 0,
    TIDE_INPUT_PENDING,
    TIDE_INPUT_EVENT,
    TIDE_INPUT_INVALID
} TideInputResult;

typedef enum TideInputEventType {
    TIDE_INPUT_KEY = 1,
    TIDE_INPUT_TEXT
} TideInputEventType;

typedef enum TideKey {
    TIDE_KEY_UNKNOWN = 0,
    TIDE_KEY_ESCAPE,
    TIDE_KEY_ENTER,
    TIDE_KEY_BACKSPACE,
    TIDE_KEY_CTRL_C,
    TIDE_KEY_CTRL_Q,
    TIDE_KEY_CTRL_S,
    TIDE_KEY_ARROW_UP,
    TIDE_KEY_ARROW_DOWN,
    TIDE_KEY_ARROW_RIGHT,
    TIDE_KEY_ARROW_LEFT
} TideKey;

typedef struct TideInputEvent {
    TideInputEventType type;
    TideKey key;
    unsigned char text;
} TideInputEvent;

typedef enum TideInputState {
    TIDE_INPUT_STATE_NORMAL = 0,
    TIDE_INPUT_STATE_ESC,
    TIDE_INPUT_STATE_CSI
} TideInputState;

typedef struct TideInputParser {
    TideInputState state;
} TideInputParser;

void tide_input_parser_init(TideInputParser *parser);
TideInputResult tide_input_feed(TideInputParser *parser, unsigned char byte, TideInputEvent *event);
TideInputResult tide_input_flush(TideInputParser *parser, TideInputEvent *event);

#endif
```

Create `src/input.c`:

```c
#include "tide/input.h"

static TideInputResult emit_key(TideInputEvent *event, TideKey key)
{
    event->type = TIDE_INPUT_KEY;
    event->key = key;
    event->text = 0;
    return TIDE_INPUT_EVENT;
}

static TideInputResult emit_text(TideInputEvent *event, unsigned char text)
{
    event->type = TIDE_INPUT_TEXT;
    event->key = TIDE_KEY_UNKNOWN;
    event->text = text;
    return TIDE_INPUT_EVENT;
}

void tide_input_parser_init(TideInputParser *parser)
{
    parser->state = TIDE_INPUT_STATE_NORMAL;
}

TideInputResult tide_input_feed(TideInputParser *parser, unsigned char byte, TideInputEvent *event)
{
    if (parser->state == TIDE_INPUT_STATE_ESC) {
        if (byte == '[') {
            parser->state = TIDE_INPUT_STATE_CSI;
            return TIDE_INPUT_PENDING;
        }
        parser->state = TIDE_INPUT_STATE_NORMAL;
        return TIDE_INPUT_INVALID;
    }

    if (parser->state == TIDE_INPUT_STATE_CSI) {
        parser->state = TIDE_INPUT_STATE_NORMAL;
        switch (byte) {
        case 'A':
            return emit_key(event, TIDE_KEY_ARROW_UP);
        case 'B':
            return emit_key(event, TIDE_KEY_ARROW_DOWN);
        case 'C':
            return emit_key(event, TIDE_KEY_ARROW_RIGHT);
        case 'D':
            return emit_key(event, TIDE_KEY_ARROW_LEFT);
        default:
            return TIDE_INPUT_INVALID;
        }
    }

    switch (byte) {
    case 0x1b:
        parser->state = TIDE_INPUT_STATE_ESC;
        return TIDE_INPUT_PENDING;
    case '\r':
    case '\n':
        return emit_key(event, TIDE_KEY_ENTER);
    case 0x7f:
    case 0x08:
        return emit_key(event, TIDE_KEY_BACKSPACE);
    case 0x03:
        return emit_key(event, TIDE_KEY_CTRL_C);
    case 0x11:
        return emit_key(event, TIDE_KEY_CTRL_Q);
    case 0x13:
        return emit_key(event, TIDE_KEY_CTRL_S);
    default:
        if (byte >= 0x20 && byte != 0x7f) {
            return emit_text(event, byte);
        }
        return TIDE_INPUT_NONE;
    }
}

TideInputResult tide_input_flush(TideInputParser *parser, TideInputEvent *event)
{
    if (parser->state == TIDE_INPUT_STATE_ESC) {
        parser->state = TIDE_INPUT_STATE_NORMAL;
        return emit_key(event, TIDE_KEY_ESCAPE);
    }

    parser->state = TIDE_INPUT_STATE_NORMAL;
    return TIDE_INPUT_NONE;
}
```

Modify `CMakeLists.txt`:

```cmake
add_library(tide_core
    src/status.c
    src/rect.c
    src/input.c
)
```

- [ ] **Step 4: Run tests and verify they pass**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `test_input` passes with the existing tests.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt include/tide/input.h src/input.c tests/CMakeLists.txt tests/test_input.c
git commit -m "feat: add terminal input parser"
```

## Task 4: Virtual Screen Buffer

**Files:**
- Create: `include/tide/screen.h`
- Create: `src/screen.c`
- Create: `tests/test_screen.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing screen buffer tests**

Create `tests/test_screen.c`:

```c
#include "tide/screen.h"
#include "test_support.h"

static void test_screen_clear_and_set_cell(void)
{
    TideScreen screen;
    TideCell cell;

    TIDE_ASSERT(tide_screen_init(&screen, 4, 2) == TIDE_OK);
    tide_screen_clear(&screen, tide_cell_make(' ', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE));

    TIDE_ASSERT(tide_screen_set(&screen, 2, 1, tide_cell_make('A', TIDE_COLOR_GREEN, TIDE_COLOR_DEFAULT, TIDE_STYLE_BOLD)) == TIDE_OK);
    TIDE_ASSERT(tide_screen_get(&screen, 2, 1, &cell) == TIDE_OK);
    TIDE_ASSERT(cell.ch == 'A');
    TIDE_ASSERT(cell.fg == TIDE_COLOR_GREEN);
    TIDE_ASSERT(cell.style == TIDE_STYLE_BOLD);
    TIDE_ASSERT(tide_screen_dirty_count(&screen) == 8);

    tide_screen_free(&screen);
}

static void test_screen_rejects_out_of_bounds_write(void)
{
    TideScreen screen;

    TIDE_ASSERT(tide_screen_init(&screen, 3, 3) == TIDE_OK);
    TIDE_ASSERT(tide_screen_set(&screen, 3, 0, tide_cell_make('x', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE)) == TIDE_ERR_INVALID);
    tide_screen_free(&screen);
}

static void test_mark_clean_resets_damage(void)
{
    TideScreen screen;

    TIDE_ASSERT(tide_screen_init(&screen, 2, 2) == TIDE_OK);
    tide_screen_clear(&screen, tide_cell_make('.', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE));
    TIDE_ASSERT(tide_screen_dirty_count(&screen) == 4);
    tide_screen_mark_clean(&screen);
    TIDE_ASSERT(tide_screen_dirty_count(&screen) == 0);
    TIDE_ASSERT(tide_screen_set(&screen, 1, 1, tide_cell_make('z', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE)) == TIDE_OK);
    TIDE_ASSERT(tide_screen_dirty_count(&screen) == 1);
    tide_screen_free(&screen);
}

int main(void)
{
    test_screen_clear_and_set_cell();
    test_screen_rejects_out_of_bounds_write();
    test_mark_clean_resets_damage();
    return 0;
}
```

Modify `tests/CMakeLists.txt`:

```cmake
add_tide_test(test_screen test_screen.c)
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```bash
cmake --build build
```

Expected: build fails because `tide/screen.h` does not exist.

- [ ] **Step 3: Add virtual screen API and implementation**

Create `include/tide/screen.h`:

```c
#ifndef TIDE_SCREEN_H
#define TIDE_SCREEN_H

#include <stddef.h>

#include "tide/status.h"

typedef enum TideColor {
    TIDE_COLOR_DEFAULT = -1,
    TIDE_COLOR_BLACK = 0,
    TIDE_COLOR_RED,
    TIDE_COLOR_GREEN,
    TIDE_COLOR_YELLOW,
    TIDE_COLOR_BLUE,
    TIDE_COLOR_MAGENTA,
    TIDE_COLOR_CYAN,
    TIDE_COLOR_WHITE
} TideColor;

typedef enum TideStyle {
    TIDE_STYLE_NONE = 0,
    TIDE_STYLE_BOLD = 1 << 0,
    TIDE_STYLE_REVERSE = 1 << 1
} TideStyle;

typedef struct TideCell {
    char ch;
    TideColor fg;
    TideColor bg;
    unsigned style;
    int dirty;
} TideCell;

typedef struct TideScreen {
    size_t width;
    size_t height;
    TideCell *cells;
} TideScreen;

TideCell tide_cell_make(char ch, TideColor fg, TideColor bg, unsigned style);
TideStatus tide_screen_init(TideScreen *screen, size_t width, size_t height);
void tide_screen_free(TideScreen *screen);
void tide_screen_clear(TideScreen *screen, TideCell cell);
TideStatus tide_screen_set(TideScreen *screen, size_t x, size_t y, TideCell cell);
TideStatus tide_screen_get(const TideScreen *screen, size_t x, size_t y, TideCell *cell);
size_t tide_screen_dirty_count(const TideScreen *screen);
void tide_screen_mark_clean(TideScreen *screen);

#endif
```

Implementation requirements for `src/screen.c`:

```c
#include "tide/screen.h"

#include <stdlib.h>

static size_t cell_index(const TideScreen *screen, size_t x, size_t y)
{
    return y * screen->width + x;
}

TideCell tide_cell_make(char ch, TideColor fg, TideColor bg, unsigned style)
{
    TideCell cell;
    cell.ch = ch;
    cell.fg = fg;
    cell.bg = bg;
    cell.style = style;
    cell.dirty = 1;
    return cell;
}

TideStatus tide_screen_init(TideScreen *screen, size_t width, size_t height)
{
    screen->width = width;
    screen->height = height;
    screen->cells = NULL;

    if (width == 0 || height == 0) {
        return TIDE_ERR_INVALID;
    }

    screen->cells = calloc(width * height, sizeof(*screen->cells));
    if (screen->cells == NULL) {
        return TIDE_ERR_ALLOC;
    }

    tide_screen_clear(screen, tide_cell_make(' ', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE));
    return TIDE_OK;
}

void tide_screen_free(TideScreen *screen)
{
    free(screen->cells);
    screen->cells = NULL;
    screen->width = 0;
    screen->height = 0;
}

void tide_screen_clear(TideScreen *screen, TideCell cell)
{
    for (size_t i = 0; i < screen->width * screen->height; ++i) {
        cell.dirty = 1;
        screen->cells[i] = cell;
    }
}

TideStatus tide_screen_set(TideScreen *screen, size_t x, size_t y, TideCell cell)
{
    if (x >= screen->width || y >= screen->height) {
        return TIDE_ERR_INVALID;
    }

    cell.dirty = 1;
    screen->cells[cell_index(screen, x, y)] = cell;
    return TIDE_OK;
}

TideStatus tide_screen_get(const TideScreen *screen, size_t x, size_t y, TideCell *cell)
{
    if (x >= screen->width || y >= screen->height) {
        return TIDE_ERR_INVALID;
    }

    *cell = screen->cells[cell_index(screen, x, y)];
    return TIDE_OK;
}

size_t tide_screen_dirty_count(const TideScreen *screen)
{
    size_t count = 0;
    for (size_t i = 0; i < screen->width * screen->height; ++i) {
        if (screen->cells[i].dirty) {
            ++count;
        }
    }
    return count;
}

void tide_screen_mark_clean(TideScreen *screen)
{
    for (size_t i = 0; i < screen->width * screen->height; ++i) {
        screen->cells[i].dirty = 0;
    }
}
```

Modify `CMakeLists.txt` to add `src/screen.c`.

- [ ] **Step 4: Run tests and verify they pass**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `test_screen` passes with existing tests.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt include/tide/screen.h src/screen.c tests/CMakeLists.txt tests/test_screen.c
git commit -m "feat: add virtual screen buffer"
```

## Task 5: ANSI Renderer And String Builder

**Files:**
- Create: `include/tide/string_builder.h`
- Create: `src/string_builder.c`
- Create: `include/tide/ansi.h`
- Create: `src/ansi.c`
- Create: `tests/test_ansi.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing ANSI renderer tests**

Create `tests/test_ansi.c`:

```c
#include "tide/ansi.h"
#include "tide/screen.h"
#include "tide/string_builder.h"
#include "test_support.h"

#include <string.h>

static void test_full_render_contains_cursor_and_cell_text(void)
{
    TideScreen screen;
    TideStringBuilder out;

    TIDE_ASSERT(tide_screen_init(&screen, 3, 2) == TIDE_OK);
    TIDE_ASSERT(tide_string_builder_init(&out) == TIDE_OK);
    TIDE_ASSERT(tide_screen_set(&screen, 1, 0, tide_cell_make('X', TIDE_COLOR_GREEN, TIDE_COLOR_DEFAULT, TIDE_STYLE_BOLD)) == TIDE_OK);

    TIDE_ASSERT(tide_ansi_render_full(&screen, &out) == TIDE_OK);
    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "\x1b[?25l") != NULL);
    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "\x1b[H") != NULL);
    TIDE_ASSERT(strstr(tide_string_builder_data(&out), " X ") != NULL);

    tide_string_builder_free(&out);
    tide_screen_free(&screen);
}

static void test_dirty_render_marks_screen_clean(void)
{
    TideScreen screen;
    TideStringBuilder out;

    TIDE_ASSERT(tide_screen_init(&screen, 2, 1) == TIDE_OK);
    TIDE_ASSERT(tide_string_builder_init(&out) == TIDE_OK);
    tide_screen_mark_clean(&screen);
    TIDE_ASSERT(tide_screen_set(&screen, 1, 0, tide_cell_make('A', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE)) == TIDE_OK);

    TIDE_ASSERT(tide_ansi_render_dirty(&screen, &out) == TIDE_OK);
    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "\x1b[1;2H") != NULL);
    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "A") != NULL);
    TIDE_ASSERT(tide_screen_dirty_count(&screen) == 0);

    tide_string_builder_free(&out);
    tide_screen_free(&screen);
}

int main(void)
{
    test_full_render_contains_cursor_and_cell_text();
    test_dirty_render_marks_screen_clean();
    return 0;
}
```

Modify `tests/CMakeLists.txt`:

```cmake
add_tide_test(test_ansi test_ansi.c)
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```bash
cmake --build build
```

Expected: build fails because `tide/ansi.h` and `tide/string_builder.h` do not exist.

- [ ] **Step 3: Add string builder and renderer APIs**

Create `include/tide/string_builder.h`:

```c
#ifndef TIDE_STRING_BUILDER_H
#define TIDE_STRING_BUILDER_H

#include <stddef.h>

#include "tide/status.h"

typedef struct TideStringBuilder {
    char *data;
    size_t length;
    size_t capacity;
} TideStringBuilder;

TideStatus tide_string_builder_init(TideStringBuilder *builder);
void tide_string_builder_free(TideStringBuilder *builder);
TideStatus tide_string_builder_append(TideStringBuilder *builder, const char *text);
TideStatus tide_string_builder_append_char(TideStringBuilder *builder, char ch);
TideStatus tide_string_builder_append_format(TideStringBuilder *builder, const char *format, size_t a, size_t b);
const char *tide_string_builder_data(const TideStringBuilder *builder);
size_t tide_string_builder_length(const TideStringBuilder *builder);

#endif
```

Create `include/tide/ansi.h`:

```c
#ifndef TIDE_ANSI_H
#define TIDE_ANSI_H

#include "tide/screen.h"
#include "tide/status.h"
#include "tide/string_builder.h"

TideStatus tide_ansi_render_full(TideScreen *screen, TideStringBuilder *out);
TideStatus tide_ansi_render_dirty(TideScreen *screen, TideStringBuilder *out);
TideStatus tide_ansi_show_cursor(TideStringBuilder *out);
TideStatus tide_ansi_hide_cursor(TideStringBuilder *out);

#endif
```

Implementation requirements:

- `tide_string_builder_append_format()` only needs to support the renderer's `"\x1b[%zu;%zuH"` cursor-position format.
- `tide_ansi_render_full()` appends hide cursor, home cursor, screen contents row by row, newline between rows, reset, and marks the screen clean.
- `tide_ansi_render_dirty()` appends one cursor-position sequence and character per dirty cell, then marks the screen clean.
- Keep color/style implementation minimal for this milestone: reset at the end and emit plain cell characters. The `TideCell` style fields still stay in the data model for future renderer expansion.

Modify `CMakeLists.txt` to add:

```cmake
    src/string_builder.c
    src/ansi.c
```

- [ ] **Step 4: Run tests and verify they pass**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `test_ansi` passes with existing tests.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt include/tide/string_builder.h include/tide/ansi.h src/string_builder.c src/ansi.c tests/CMakeLists.txt tests/test_ansi.c
git commit -m "feat: add ANSI screen renderer"
```

## Task 6: Terminal Raw Mode

**Files:**
- Create: `include/tide/terminal.h`
- Create: `src/terminal.c`
- Create: `tests/test_terminal.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing terminal transformation tests**

Create `tests/test_terminal.c`:

```c
#include "tide/terminal.h"
#include "test_support.h"

#include <termios.h>

static void test_make_raw_clears_canonical_echo_and_signals(void)
{
    struct termios input = {0};
    struct termios output = {0};

    input.c_lflag = ECHO | ICANON | IEXTEN | ISIG;
    input.c_iflag = IXON | ICRNL | BRKINT | INPCK | ISTRIP;
    input.c_oflag = OPOST;
    input.c_cflag = CS7;

    tide_terminal_make_raw(&input, &output);

    TIDE_ASSERT((output.c_lflag & ECHO) == 0);
    TIDE_ASSERT((output.c_lflag & ICANON) == 0);
    TIDE_ASSERT((output.c_lflag & ISIG) == 0);
    TIDE_ASSERT((output.c_iflag & IXON) == 0);
    TIDE_ASSERT((output.c_iflag & ICRNL) == 0);
    TIDE_ASSERT((output.c_oflag & OPOST) == 0);
    TIDE_ASSERT((output.c_cflag & CS8) == CS8);
    TIDE_ASSERT(output.c_cc[VMIN] == 0);
    TIDE_ASSERT(output.c_cc[VTIME] == 1);
}

int main(void)
{
    test_make_raw_clears_canonical_echo_and_signals();
    return 0;
}
```

Modify `tests/CMakeLists.txt`:

```cmake
add_tide_test(test_terminal test_terminal.c)
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```bash
cmake --build build
```

Expected: build fails because `tide/terminal.h` does not exist.

- [ ] **Step 3: Add terminal API and implementation**

Create `include/tide/terminal.h`:

```c
#ifndef TIDE_TERMINAL_H
#define TIDE_TERMINAL_H

#include <termios.h>

#include "tide/status.h"

typedef struct TideTerminal {
    int fd;
    int raw_enabled;
    struct termios original;
} TideTerminal;

void tide_terminal_make_raw(const struct termios *input, struct termios *output);
TideStatus tide_terminal_enable_raw(TideTerminal *terminal, int fd);
void tide_terminal_disable_raw(TideTerminal *terminal);

#endif
```

Create `src/terminal.c`:

```c
#include "tide/terminal.h"

#include <unistd.h>

void tide_terminal_make_raw(const struct termios *input, struct termios *output)
{
    *output = *input;
    output->c_iflag &= (tcflag_t) ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    output->c_oflag &= (tcflag_t) ~(OPOST);
    output->c_cflag &= (tcflag_t) ~(CSIZE);
    output->c_cflag |= CS8;
    output->c_lflag &= (tcflag_t) ~(ECHO | ICANON | IEXTEN | ISIG);
    output->c_cc[VMIN] = 0;
    output->c_cc[VTIME] = 1;
}

TideStatus tide_terminal_enable_raw(TideTerminal *terminal, int fd)
{
    struct termios raw;

    terminal->fd = fd;
    terminal->raw_enabled = 0;

    if (tcgetattr(fd, &terminal->original) == -1) {
        return TIDE_ERR_IO;
    }

    tide_terminal_make_raw(&terminal->original, &raw);

    if (tcsetattr(fd, TCSAFLUSH, &raw) == -1) {
        return TIDE_ERR_IO;
    }

    terminal->raw_enabled = 1;
    return TIDE_OK;
}

void tide_terminal_disable_raw(TideTerminal *terminal)
{
    if (terminal->raw_enabled) {
        tcsetattr(terminal->fd, TCSAFLUSH, &terminal->original);
        terminal->raw_enabled = 0;
    }
}
```

Modify `CMakeLists.txt` to add `src/terminal.c`.

- [ ] **Step 4: Run tests and verify they pass**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `test_terminal` passes with existing tests.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt include/tide/terminal.h src/terminal.c tests/CMakeLists.txt tests/test_terminal.c
git commit -m "feat: add terminal raw mode support"
```

## Task 7: Foundation App Shell

**Files:**
- Create: `include/tide/app.h`
- Create: `src/app.c`
- Create: `tests/test_app.c`
- Modify: `CMakeLists.txt`
- Modify: `main.c`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing app render test**

Create `tests/test_app.c`:

```c
#include "tide/app.h"
#include "tide/string_builder.h"
#include "test_support.h"

#include <string.h>

static void test_demo_render_contains_title_and_status(void)
{
    TideStringBuilder out;

    TIDE_ASSERT(tide_string_builder_init(&out) == TIDE_OK);
    TIDE_ASSERT(tide_app_render_demo(20, 5, &out) == TIDE_OK);

    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "tide") != NULL);
    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "q: quit") != NULL);

    tide_string_builder_free(&out);
}

int main(void)
{
    test_demo_render_contains_title_and_status();
    return 0;
}
```

Modify `tests/CMakeLists.txt`:

```cmake
add_tide_test(test_app test_app.c)
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```bash
cmake --build build
```

Expected: build fails because `tide/app.h` does not exist.

- [ ] **Step 3: Add app API and demo renderer**

Create `include/tide/app.h`:

```c
#ifndef TIDE_APP_H
#define TIDE_APP_H

#include <stddef.h>

#include "tide/status.h"
#include "tide/string_builder.h"

TideStatus tide_app_render_demo(size_t width, size_t height, TideStringBuilder *out);
int tide_app_run(void);
void tide_app_request_shutdown(void);

#endif
```

Create `src/app.c` with these responsibilities:

- Maintain a file-local shutdown flag for signal handling.
- Render a blank command-centric foundation screen:
  - first row contains `tide`
  - last row contains `q: quit`
  - middle rows are spaces
- `tide_app_render_demo()` renders into a `TideStringBuilder` using `TideScreen` and `tide_ansi_render_full()`.
- `tide_app_run()` enables raw mode on `STDIN_FILENO`, draws the demo screen to `STDOUT_FILENO`, reads bytes until `q`, Ctrl-Q, or Ctrl-C, then restores terminal mode and shows cursor.

Use this public behavior in `main.c`:

```c
#include "tide/app.h"
#include "tide/string_builder.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        puts("tide 0.1.0");
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "--render-demo") == 0) {
        TideStringBuilder out;
        if (tide_string_builder_init(&out) != TIDE_OK) {
            fputs("tide: allocation failed\n", stderr);
            return 1;
        }
        if (tide_app_render_demo(40, 8, &out) != TIDE_OK) {
            tide_string_builder_free(&out);
            fputs("tide: render failed\n", stderr);
            return 1;
        }
        fputs(tide_string_builder_data(&out), stdout);
        tide_string_builder_free(&out);
        return 0;
    }

    return tide_app_run();
}
```

Modify `CMakeLists.txt` to add `src/app.c`.

- [ ] **Step 4: Run tests and CLI checks**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
./build/tide --version
./build/tide --render-demo
```

Expected:

- CTest reports all tests pass.
- `./build/tide --version` prints `tide 0.1.0`.
- `./build/tide --render-demo` prints ANSI output containing `tide` and `q: quit`.

- [ ] **Step 5: Manual terminal smoke**

Run:

```bash
./build/tide
```

Expected:

- Terminal switches to raw mode.
- A minimal `tide` foundation screen appears.
- Pressing `q` exits.
- Cursor returns and terminal input behaves normally after exit.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt main.c include/tide/app.h src/app.c tests/CMakeLists.txt tests/test_app.c
git commit -m "feat: add foundation app shell"
```

## Task 8: Documentation And Verification

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Update README with current build/run/test instructions**

Replace `README.md` with:

```markdown
# Terminal C IDE

`tide` is a macOS-first terminal IDE written in C. The project is intentionally built from scratch: no `ncurses`, terminal UI library, JSON library, async library, editor library, or LSP client library.

The current implementation target is Foundation: terminal raw mode, input parsing, virtual screen rendering, ANSI output, layout primitives, and a minimal command-centric app shell.

## Design

- [Terminal C IDE Design](docs/superpowers/specs/2026-04-30-terminal-c-ide-design.md)
- [Foundation Implementation Plan](docs/superpowers/plans/2026-04-30-terminal-c-ide-foundation.md)

## Requirements

- macOS
- CMake 3.20+
- Apple Clang or compatible C11 compiler

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

## Sanitizer Build

```bash
cmake -S . -B build-asan -DTIDE_ENABLE_SANITIZERS=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

## Run

```bash
./build/tide --version
./build/tide --render-demo
./build/tide
```

In interactive mode, press `q` to exit.
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
git commit -m "docs: describe foundation build workflow"
```

## Self-Review Notes

Spec coverage:

- Foundation raw terminal mode: Task 6 and Task 7.
- Input decoding: Task 3.
- ANSI output: Task 5.
- Virtual screen rendering and damage tracking: Task 4 and Task 5.
- Layout primitives: Task 2.
- Error cleanup: Task 6 and Task 7.
- Test infrastructure: Task 1 through Task 8.

Known scope boundaries:

- Editor Core starts after this plan. It will add buffers, file I/O, undo/redo, search, C syntax highlighting, and command palette behavior.
- Workbench, Build IDE, C Intelligence, and Advanced IDE are separate future plans after the editor core is usable.
