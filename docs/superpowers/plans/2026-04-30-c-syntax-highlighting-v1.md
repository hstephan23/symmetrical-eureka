# C Syntax Highlighting v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add lexical C syntax highlighting to visible editor text.

**Architecture:** A new `syntax` module classifies one C source line at a time into fixed-capacity token spans without allocation. The editor renderer tokenizes each visible line, maps token kinds to existing `TideColor` values, and keeps search-match reverse styling layered on top of syntax foreground colors. This slice does not add parsing, themes, LSP, or multiline block-comment state.

**Tech Stack:** C11, CMake, CTest, existing `TideScreen`, `TideEditor`, and renderer cell color model.

---

## File Structure

- Create: `include/tide/syntax.h` for token kinds, token spans, line token array, tokenizer API, and lookup helper.
- Create: `src/syntax.c` for C lexical line tokenization.
- Create: `tests/test_syntax.c` for direct tokenizer tests.
- Modify: `CMakeLists.txt` to add `src/syntax.c` to `tide_core`.
- Modify: `tests/CMakeLists.txt` to add `test_syntax`.
- Modify: `src/editor_render.c` to apply token colors while drawing buffer text.
- Modify: `tests/test_editor_render.c` to verify syntax colors and search style layering.
- Modify: `README.md` to document C syntax highlighting.

## Task 1: Syntax Module

**Files:**
- Create: `include/tide/syntax.h`
- Create: `src/syntax.c`
- Create: `tests/test_syntax.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing syntax tests**

Create `tests/test_syntax.c`:

```c
#include "tide/syntax.h"
#include "test_support.h"

#include <string.h>

static TideSyntaxKind kind_at_text(const char *line, const char *needle)
{
    TideSyntaxLine syntax;
    const char *match = strstr(line, needle);
    TIDE_ASSERT(match != NULL);
    tide_syntax_tokenize_c_line(line, strlen(line), &syntax);
    return tide_syntax_kind_at(&syntax, (size_t)(match - line));
}

static void test_keywords_types_and_identifiers(void)
{
    const char *line = "int integer = return_value + return;";
    TideSyntaxLine syntax;

    tide_syntax_tokenize_c_line(line, strlen(line), &syntax);

    TIDE_ASSERT(tide_syntax_kind_at(&syntax, 0) == TIDE_SYNTAX_TYPE);
    TIDE_ASSERT(tide_syntax_kind_at(&syntax, 4) == TIDE_SYNTAX_TEXT);
    TIDE_ASSERT(tide_syntax_kind_at(&syntax, 14) == TIDE_SYNTAX_TEXT);
    TIDE_ASSERT(tide_syntax_kind_at(&syntax, 29) == TIDE_SYNTAX_KEYWORD);
}

static void test_literals_numbers_comments_and_preprocessor(void)
{
    TIDE_ASSERT(kind_at_text("#include <stdio.h>", "#") == TIDE_SYNTAX_PREPROCESSOR);
    TIDE_ASSERT(kind_at_text("int n = 42;", "42") == TIDE_SYNTAX_NUMBER);
    TIDE_ASSERT(kind_at_text("char *s = \"hi\\\"\";", "\"hi") == TIDE_SYNTAX_STRING);
    TIDE_ASSERT(kind_at_text("char c = '\\n';", "'\\n'") == TIDE_SYNTAX_CHAR);
    TIDE_ASSERT(kind_at_text("x++; // comment", "//") == TIDE_SYNTAX_COMMENT);
    TIDE_ASSERT(kind_at_text("x = /* comment */ 1;", "/*") == TIDE_SYNTAX_COMMENT);
}

int main(void)
{
    test_keywords_types_and_identifiers();
    test_literals_numbers_comments_and_preprocessor();
    return 0;
}
```

Add to `tests/CMakeLists.txt` after `test_screen`:

```cmake
add_tide_test(test_syntax test_syntax.c)
```

- [ ] **Step 2: Verify the syntax tests fail**

Run:

```bash
cmake -S . -B build
cmake --build build
```

Expected: build fails because `tide/syntax.h` does not exist.

- [ ] **Step 3: Add syntax API and implementation**

Create `include/tide/syntax.h`:

```c
#ifndef TIDE_SYNTAX_H
#define TIDE_SYNTAX_H

#include <stddef.h>

#define TIDE_SYNTAX_MAX_TOKENS 128

typedef enum TideSyntaxKind {
    TIDE_SYNTAX_TEXT = 0,
    TIDE_SYNTAX_KEYWORD,
    TIDE_SYNTAX_TYPE,
    TIDE_SYNTAX_NUMBER,
    TIDE_SYNTAX_STRING,
    TIDE_SYNTAX_CHAR,
    TIDE_SYNTAX_COMMENT,
    TIDE_SYNTAX_PREPROCESSOR
} TideSyntaxKind;

typedef struct TideSyntaxToken {
    size_t start;
    size_t length;
    TideSyntaxKind kind;
} TideSyntaxToken;

typedef struct TideSyntaxLine {
    TideSyntaxToken tokens[TIDE_SYNTAX_MAX_TOKENS];
    size_t count;
} TideSyntaxLine;

void tide_syntax_tokenize_c_line(const char *line, size_t length, TideSyntaxLine *out);
TideSyntaxKind tide_syntax_kind_at(const TideSyntaxLine *line, size_t column);

#endif
```

Create `src/syntax.c` with fixed-capacity token helpers, C identifier scanning, keyword/type tables, string/char escape scanning, line and same-line block comment scanning, preprocessor line detection, and number scanning. The public functions must match the header:

```c
void tide_syntax_tokenize_c_line(const char *line, size_t length, TideSyntaxLine *out);
TideSyntaxKind tide_syntax_kind_at(const TideSyntaxLine *line, size_t column);
```

Implementation requirements:

- `out->count = 0` at the start.
- Add tokens only for non-default syntax kinds.
- Return `TIDE_SYNTAX_TEXT` from `tide_syntax_kind_at()` when no token covers the column.
- Use `unsigned char` casts for all `<ctype.h>` calls.
- Treat a line as preprocessor when its first non-space character is `#`; the token starts at that `#` and runs to the end of the line.
- Treat unterminated strings, chars, and block comments as running to the end of the line.

Modify `CMakeLists.txt` to include `src/syntax.c` in `tide_core` after `src/string_builder.c`:

```cmake
    src/syntax.c
```

- [ ] **Step 4: Verify syntax module green**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt tests/CMakeLists.txt include/tide/syntax.h src/syntax.c tests/test_syntax.c
git commit -m "feat: add C syntax tokenizer"
```

## Task 2: Renderer Syntax Colors

**Files:**
- Modify: `src/editor_render.c`
- Modify: `tests/test_editor_render.c`

- [ ] **Step 1: Write failing renderer color tests**

Append to `tests/test_editor_render.c` before `main()`:

```c
static void insert_render_text(TideEditor *editor, const char *text)
{
    for (size_t i = 0; text[i] != '\0'; ++i) {
        TIDE_ASSERT(tide_editor_insert_char(editor, text[i]) == TIDE_OK);
    }
}

static void test_editor_render_applies_c_syntax_colors(void)
{
    TideBuffer buffer;
    TideEditor editor;
    TideScreen screen;
    TideCell cell;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    insert_render_text(&editor, "int main(void) { return 42; }");

    TIDE_ASSERT(tide_screen_init(&screen, 40, 4) == TIDE_OK);
    TIDE_ASSERT(tide_editor_render(&editor, &screen) == TIDE_OK);

    TIDE_ASSERT(tide_screen_get(&screen, 0, 0, &cell) == TIDE_OK);
    TIDE_ASSERT(cell.fg == TIDE_COLOR_GREEN);
    TIDE_ASSERT(tide_screen_get(&screen, 4, 0, &cell) == TIDE_OK);
    TIDE_ASSERT(cell.fg == TIDE_COLOR_DEFAULT);
    TIDE_ASSERT(tide_screen_get(&screen, 9, 0, &cell) == TIDE_OK);
    TIDE_ASSERT(cell.fg == TIDE_COLOR_GREEN);
    TIDE_ASSERT(tide_screen_get(&screen, 17, 0, &cell) == TIDE_OK);
    TIDE_ASSERT(cell.fg == TIDE_COLOR_CYAN);
    TIDE_ASSERT(tide_screen_get(&screen, 24, 0, &cell) == TIDE_OK);
    TIDE_ASSERT(cell.fg == TIDE_COLOR_YELLOW);

    tide_screen_free(&screen);
    tide_buffer_free(&buffer);
}

static void test_editor_render_keeps_search_reverse_style_on_syntax_color(void)
{
    TideBuffer buffer;
    TideEditor editor;
    TideScreen screen;
    TideCell cell;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    insert_render_text(&editor, "return");
    editor.cursor = (TideBufferPosition){0, 0};
    TIDE_ASSERT(tide_editor_find(&editor, "return") == TIDE_OK);

    TIDE_ASSERT(tide_screen_init(&screen, 20, 4) == TIDE_OK);
    TIDE_ASSERT(tide_editor_render(&editor, &screen) == TIDE_OK);

    TIDE_ASSERT(tide_screen_get(&screen, 0, 0, &cell) == TIDE_OK);
    TIDE_ASSERT(cell.fg == TIDE_COLOR_CYAN);
    TIDE_ASSERT((cell.style & TIDE_STYLE_REVERSE) != 0);

    tide_screen_free(&screen);
    tide_buffer_free(&buffer);
}
```

Call both tests from `main()`:

```c
    test_editor_render_applies_c_syntax_colors();
    test_editor_render_keeps_search_reverse_style_on_syntax_color();
```

- [ ] **Step 2: Verify renderer tests fail**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `test_editor_render` fails because text cells still use `TIDE_COLOR_DEFAULT`.

- [ ] **Step 3: Apply syntax colors in renderer**

Modify `src/editor_render.c`:

- Include `tide/syntax.h`.
- Add a private `syntax_color()` mapper:

```c
static TideColor syntax_color(TideSyntaxKind kind)
{
    switch (kind) {
    case TIDE_SYNTAX_KEYWORD:
        return TIDE_COLOR_CYAN;
    case TIDE_SYNTAX_TYPE:
        return TIDE_COLOR_GREEN;
    case TIDE_SYNTAX_NUMBER:
        return TIDE_COLOR_YELLOW;
    case TIDE_SYNTAX_STRING:
    case TIDE_SYNTAX_CHAR:
        return TIDE_COLOR_MAGENTA;
    case TIDE_SYNTAX_COMMENT:
        return TIDE_COLOR_BLUE;
    case TIDE_SYNTAX_PREPROCESSOR:
        return TIDE_COLOR_RED;
    case TIDE_SYNTAX_TEXT:
        return TIDE_COLOR_DEFAULT;
    }
    return TIDE_COLOR_DEFAULT;
}
```

- In `draw_buffer_lines()`, tokenize each visible line before the column loop:

```c
        TideSyntaxLine syntax;
        tide_syntax_tokenize_c_line(line, line_length, &syntax);
```

- Inside the column loop, set `text_cell.fg` from the token kind before applying search style:

```c
            text_cell.fg = syntax_color(tide_syntax_kind_at(&syntax, buffer_column));
```

- Keep `text_cell.bg = TIDE_COLOR_DEFAULT` and `text_cell.style = TIDE_STYLE_NONE` for each character before search-match style is applied.

- [ ] **Step 4: Verify renderer integration green**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 5: Commit**

```bash
git add src/editor_render.c tests/test_editor_render.c
git commit -m "feat: color C syntax in editor renderer"
```

## Task 3: Documentation and Full Verification

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Document syntax highlighting**

In `README.md`, add to the interactive keys section:

```markdown
- C keywords, literals, comments, and preprocessor lines are syntax highlighted.
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

- [ ] **Step 3: Run manual render smoke**

Run:

```bash
./build/tide build/manual-syntax.c
```

Type:

```text
#include <stdio.h>
int main(void) { return 42; }
Ctrl-P
q
Enter
```

Expected: the editor renders the C text without corrupting status or command-prompt behavior and exits cleanly.

- [ ] **Step 4: Commit**

```bash
git add README.md
git commit -m "docs: describe C syntax highlighting"
```

## Coverage Checklist

- Token classification for keywords, types, numbers, strings, chars, comments, preprocessor directives, and identifiers is covered by Task 1.
- Renderer color mapping for type, keyword, number, and default text is covered by Task 2.
- Search reverse style layered over syntax color is covered by Task 2.
- README user-facing documentation is covered by Task 3.
