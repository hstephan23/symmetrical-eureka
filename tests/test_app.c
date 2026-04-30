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
