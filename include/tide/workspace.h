#ifndef TIDE_WORKSPACE_H
#define TIDE_WORKSPACE_H

#include <stddef.h>

#include "tide/buffer.h"
#include "tide/editor.h"
#include "tide/status.h"

typedef struct TideWorkspaceEntry {
    TideBuffer buffer;
    TideEditor editor;
} TideWorkspaceEntry;

typedef struct TideWorkspace {
    TideWorkspaceEntry *entries;
    size_t count;
    size_t capacity;
    size_t current;
} TideWorkspace;

TideStatus tide_workspace_init(TideWorkspace *workspace);
void tide_workspace_free(TideWorkspace *workspace);
TideStatus tide_workspace_open_file(TideWorkspace *workspace, const char *path);
TideStatus tide_workspace_switch_to(TideWorkspace *workspace, size_t index);
TideStatus tide_workspace_next(TideWorkspace *workspace);
TideStatus tide_workspace_previous(TideWorkspace *workspace);
TideEditor *tide_workspace_current_editor(TideWorkspace *workspace);
const TideEditor *tide_workspace_current_editor_const(const TideWorkspace *workspace);
size_t tide_workspace_count(const TideWorkspace *workspace);
size_t tide_workspace_current_index(const TideWorkspace *workspace);

#endif
