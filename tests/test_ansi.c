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
    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "X") != NULL);

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

int main(void)
{
    test_full_render_contains_cursor_and_cell_text();
    test_dirty_render_marks_screen_clean();
    test_full_render_emits_color_style_and_reset();
    test_full_render_emits_reverse_style();
    test_dirty_render_emits_cursor_and_color();
    return 0;
}
