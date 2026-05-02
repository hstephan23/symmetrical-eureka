#ifndef TIDE_TASKS_H
#define TIDE_TASKS_H

#include <stddef.h>

#include "tide/status.h"

typedef struct TideTaskResult {
    char *output;
    size_t output_length;
    int exit_code;
} TideTaskResult;

TideStatus tide_tasks_run_shell(const char *command, TideTaskResult *result);
void tide_task_result_free(TideTaskResult *result);

#endif
