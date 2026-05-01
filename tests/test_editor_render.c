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

static void test_editor_render_highlights_current_search_match(void)
{
    TideBuffer buffer;
    TideEditor editor;
    TideScreen screen;
    TideCell cell;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'b') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'c') == TIDE_OK);
    editor.cursor = (TideBufferPosition){0, 0};
    TIDE_ASSERT(tide_editor_find(&editor, "b") == TIDE_OK);

    TIDE_ASSERT(tide_screen_init(&screen, 20, 4) == TIDE_OK);
    TIDE_ASSERT(tide_editor_render(&editor, &screen) == TIDE_OK);

    TIDE_ASSERT(tide_screen_get(&screen, 1, 0, &cell) == TIDE_OK);
    TIDE_ASSERT(cell.ch == 'b');
    TIDE_ASSERT((cell.style & TIDE_STYLE_REVERSE) != 0);

    tide_screen_free(&screen);
    tide_buffer_free(&buffer);
}

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

int main(void)
{
    test_editor_render_draws_text_and_status();
    test_editor_render_draws_command_prompt_on_status_line();
    test_editor_render_highlights_current_search_match();
    test_editor_render_applies_c_syntax_colors();
    test_editor_render_keeps_search_reverse_style_on_syntax_color();
    return 0;
}
