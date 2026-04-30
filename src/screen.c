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
