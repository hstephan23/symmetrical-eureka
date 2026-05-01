#include "tide/session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TIDE_SESSION_HEADER "tide-session-v1"
#define TIDE_SESSION_LINE_CAPACITY 2048

static void trim_newline(char *line)
{
    size_t length = strlen(line);
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        line[--length] = '\0';
    }
}

static int parse_size(const char *text, size_t *value)
{
    char *end = NULL;
    unsigned long parsed = strtoul(text, &end, 10);
    if (end == text || *end != '\0') {
        return 0;
    }

    *value = (size_t)parsed;
    return 1;
}

TideStatus tide_session_save_workspace(const TideWorkspace *workspace, const char *path)
{
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return TIDE_ERR_IO;
    }

    if (fprintf(file, "%s\n", TIDE_SESSION_HEADER) < 0 ||
        fprintf(file, "current\t%zu\n", tide_workspace_current_index(workspace)) < 0) {
        fclose(file);
        return TIDE_ERR_IO;
    }

    for (size_t i = 0; i < workspace->count; ++i) {
        const char *buffer_path = workspace->entries[i].buffer.path;
        if (buffer_path != NULL && fprintf(file, "file\t%s\n", buffer_path) < 0) {
            fclose(file);
            return TIDE_ERR_IO;
        }
    }

    if (fclose(file) != 0) {
        return TIDE_ERR_IO;
    }

    return TIDE_OK;
}

TideStatus tide_session_load_workspace(TideWorkspace *workspace, const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return TIDE_ERR_IO;
    }

    char line[TIDE_SESSION_LINE_CAPACITY];
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return TIDE_ERR_INVALID;
    }
    trim_newline(line);
    if (strcmp(line, TIDE_SESSION_HEADER) != 0) {
        fclose(file);
        return TIDE_ERR_INVALID;
    }

    TideWorkspace replacement;
    TideStatus status = tide_workspace_init(&replacement);
    if (status != TIDE_OK) {
        fclose(file);
        return status;
    }

    size_t current = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        trim_newline(line);
        if (strncmp(line, "current\t", 8) == 0) {
            if (!parse_size(line + 8, &current)) {
                status = TIDE_ERR_INVALID;
                goto fail;
            }
            continue;
        }

        if (strncmp(line, "file\t", 5) == 0) {
            status = tide_workspace_open_file(&replacement, line + 5);
            if (status != TIDE_OK) {
                goto fail;
            }
            continue;
        }

        if (line[0] != '\0') {
            status = TIDE_ERR_INVALID;
            goto fail;
        }
    }

    if (ferror(file)) {
        status = TIDE_ERR_IO;
        goto fail;
    }
    fclose(file);

    if (replacement.count == 0) {
        tide_workspace_free(&replacement);
        return TIDE_ERR_INVALID;
    }

    if (current >= replacement.count) {
        current = replacement.count - 1;
    }
    status = tide_workspace_switch_to(&replacement, current);
    if (status != TIDE_OK) {
        tide_workspace_free(&replacement);
        return status;
    }

    TideWorkspace old = *workspace;
    *workspace = replacement;
    tide_workspace_free(&old);
    return TIDE_OK;

fail:
    fclose(file);
    tide_workspace_free(&replacement);
    return status;
}
