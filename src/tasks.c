#include "tide/tasks.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void result_init(TideTaskResult *result)
{
    result->output = NULL;
    result->output_length = 0;
    result->exit_code = -1;
}

static TideStatus append_output(char **output, size_t *length, size_t *capacity, const char *data, size_t data_length)
{
    size_t needed = *length + data_length + 1;
    size_t next_capacity = *capacity;

    while (next_capacity < needed) {
        if (next_capacity > ((size_t)-1) / 2) {
            return TIDE_ERR_ALLOC;
        }
        next_capacity *= 2;
    }

    if (next_capacity != *capacity) {
        char *next_output = realloc(*output, next_capacity);
        if (next_output == NULL) {
            return TIDE_ERR_ALLOC;
        }
        *output = next_output;
        *capacity = next_capacity;
    }

    memcpy(*output + *length, data, data_length);
    *length += data_length;
    (*output)[*length] = '\0';
    return TIDE_OK;
}

static int wait_for_child(pid_t child, int *exit_code)
{
    int status;

    for (;;) {
        if (waitpid(child, &status, 0) == child) {
            break;
        }
        if (errno != EINTR) {
            return 0;
        }
    }

    if (WIFEXITED(status)) {
        *exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        *exit_code = 128 + WTERMSIG(status);
    } else {
        *exit_code = -1;
    }

    return 1;
}

TideStatus tide_tasks_run_shell(const char *command, TideTaskResult *result)
{
    int fds[2];
    pid_t child;
    char buffer[4096];
    size_t capacity = 4096;
    TideStatus status = TIDE_OK;

    if (command == NULL || result == NULL) {
        return TIDE_ERR_INVALID;
    }

    result_init(result);
    result->output = malloc(capacity);
    if (result->output == NULL) {
        return TIDE_ERR_ALLOC;
    }
    result->output[0] = '\0';

    if (pipe(fds) != 0) {
        tide_task_result_free(result);
        return TIDE_ERR_IO;
    }

    child = fork();
    if (child == -1) {
        close(fds[0]);
        close(fds[1]);
        tide_task_result_free(result);
        return TIDE_ERR_IO;
    }

    if (child == 0) {
        close(fds[0]);
        if (dup2(fds[1], STDOUT_FILENO) == -1 || dup2(fds[1], STDERR_FILENO) == -1) {
            _exit(127);
        }
        close(fds[1]);
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
        _exit(127);
    }

    close(fds[1]);
    for (;;) {
        ssize_t nread = read(fds[0], buffer, sizeof(buffer));
        if (nread > 0) {
            status = append_output(&result->output, &result->output_length, &capacity, buffer, (size_t)nread);
            if (status != TIDE_OK) {
                break;
            }
            continue;
        }
        if (nread == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        status = TIDE_ERR_IO;
        break;
    }
    close(fds[0]);

    if (!wait_for_child(child, &result->exit_code) && status == TIDE_OK) {
        status = TIDE_ERR_IO;
    }

    if (status != TIDE_OK) {
        tide_task_result_free(result);
    }
    return status;
}

void tide_task_result_free(TideTaskResult *result)
{
    if (result == NULL) {
        return;
    }

    free(result->output);
    result_init(result);
}
