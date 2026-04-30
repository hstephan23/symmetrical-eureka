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
