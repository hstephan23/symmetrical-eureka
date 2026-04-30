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
