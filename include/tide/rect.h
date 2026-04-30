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
