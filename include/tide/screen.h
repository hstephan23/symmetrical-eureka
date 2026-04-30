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
