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
