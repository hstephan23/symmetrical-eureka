#include "tide/workspace.h"

#include <stdlib.h>
#include <string.h>

static void refresh_editor_buffers(TideWorkspace *workspace)
{
    for (size_t i = 0; i < workspace->count; ++i) {
        workspace->entries[i].editor.buffer = &workspace->entries[i].buffer;
    }
}

static TideStatus ensure_capacity(TideWorkspace *workspace, size_t needed)
{
    size_t next_capacity = workspace->capacity == 0 ? 4 : workspace->capacity;

    if (needed <= workspace->capacity) {
        return TIDE_OK;
    }

    while (next_capacity < needed) {
        if (next_capacity > ((size_t)-1) / 2) {
            return TIDE_ERR_ALLOC;
        }
        next_capacity *= 2;
    }

    TideWorkspaceEntry *next_entries = realloc(workspace->entries, next_capacity * sizeof(*workspace->entries));
    if (next_entries == NULL) {
        return TIDE_ERR_ALLOC;
    }

    workspace->entries = next_entries;
    workspace->capacity = next_capacity;
    refresh_editor_buffers(workspace);
    return TIDE_OK;
}

static int find_open_path(const TideWorkspace *workspace, const char *path, size_t *index)
{
    for (size_t i = 0; i < workspace->count; ++i) {
        const char *entry_path = workspace->entries[i].buffer.path;
        if (entry_path != NULL && strcmp(entry_path, path) == 0) {
            *index = i;
            return 1;
        }
    }

    return 0;
}

TideStatus tide_workspace_init(TideWorkspace *workspace)
{
    workspace->entries = NULL;
    workspace->count = 0;
    workspace->capacity = 0;
    workspace->current = 0;
    return TIDE_OK;
}

void tide_workspace_free(TideWorkspace *workspace)
{
    for (size_t i = 0; i < workspace->count; ++i) {
        tide_buffer_free(&workspace->entries[i].buffer);
    }

    free(workspace->entries);
    workspace->entries = NULL;
    workspace->count = 0;
    workspace->capacity = 0;
    workspace->current = 0;
}

TideStatus tide_workspace_open_file(TideWorkspace *workspace, const char *path)
{
    size_t existing;
    if (path == NULL || path[0] == '\0') {
        return TIDE_ERR_INVALID;
    }

    if (find_open_path(workspace, path, &existing)) {
        workspace->current = existing;
        return TIDE_OK;
    }

    TideStatus status = ensure_capacity(workspace, workspace->count + 1);
    if (status != TIDE_OK) {
        return status;
    }

    TideWorkspaceEntry *entry = &workspace->entries[workspace->count];
    status = tide_buffer_load_file(&entry->buffer, path);
    if (status != TIDE_OK) {
        return status;
    }

    tide_editor_init(&entry->editor, &entry->buffer);
    workspace->current = workspace->count;
    workspace->count++;
    return TIDE_OK;
}

TideStatus tide_workspace_switch_to(TideWorkspace *workspace, size_t index)
{
    if (index >= workspace->count) {
        return TIDE_ERR_INVALID;
    }

    workspace->current = index;
    return TIDE_OK;
}

TideStatus tide_workspace_next(TideWorkspace *workspace)
{
    if (workspace->count == 0) {
        return TIDE_OK;
    }

    workspace->current = (workspace->current + 1) % workspace->count;
    return TIDE_OK;
}

TideStatus tide_workspace_previous(TideWorkspace *workspace)
{
    if (workspace->count == 0) {
        return TIDE_OK;
    }

    workspace->current = workspace->current == 0 ? workspace->count - 1 : workspace->current - 1;
    return TIDE_OK;
}

TideEditor *tide_workspace_current_editor(TideWorkspace *workspace)
{
    if (workspace->count == 0) {
        return NULL;
    }

    return &workspace->entries[workspace->current].editor;
}

const TideEditor *tide_workspace_current_editor_const(const TideWorkspace *workspace)
{
    if (workspace->count == 0) {
        return NULL;
    }

    return &workspace->entries[workspace->current].editor;
}

size_t tide_workspace_count(const TideWorkspace *workspace)
{
    return workspace->count;
}

size_t tide_workspace_current_index(const TideWorkspace *workspace)
{
    return workspace->current;
}
