#include "tide/status.h"
#include "test_support.h"

static void test_status_strings_are_stable(void)
{
    TIDE_ASSERT_STR_EQ(tide_status_string(TIDE_OK), "ok");
    TIDE_ASSERT_STR_EQ(tide_status_string(TIDE_ERR_ALLOC), "allocation failed");
    TIDE_ASSERT_STR_EQ(tide_status_string(TIDE_ERR_IO), "i/o failed");
    TIDE_ASSERT_STR_EQ(tide_status_string(TIDE_ERR_INVALID), "invalid input");
    TIDE_ASSERT_STR_EQ(tide_status_string(TIDE_ERR_UNSUPPORTED), "unsupported operation");
}

int main(void)
{
    test_status_strings_are_stable();
    return 0;
}
