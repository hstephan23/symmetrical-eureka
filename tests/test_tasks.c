#include "tide/tasks.h"
#include "test_support.h"

#include <string.h>

static void test_task_captures_stdout_and_stderr(void)
{
    TideTaskResult result;

    TIDE_ASSERT(tide_tasks_run_shell("printf stdout; printf stderr >&2", &result) == TIDE_OK);

    TIDE_ASSERT(result.exit_code == 0);
    TIDE_ASSERT(strstr(result.output, "stdout") != NULL);
    TIDE_ASSERT(strstr(result.output, "stderr") != NULL);

    tide_task_result_free(&result);
}

static void test_task_reports_nonzero_exit_code(void)
{
    TideTaskResult result;

    TIDE_ASSERT(tide_tasks_run_shell("printf fail; exit 7", &result) == TIDE_OK);

    TIDE_ASSERT(result.exit_code == 7);
    TIDE_ASSERT_STR_EQ(result.output, "fail");

    tide_task_result_free(&result);
}

int main(void)
{
    test_task_captures_stdout_and_stderr();
    test_task_reports_nonzero_exit_code();
    return 0;
}
