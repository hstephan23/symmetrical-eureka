#ifndef TIDE_APP_H
#define TIDE_APP_H

#include <stddef.h>

#include "tide/editor.h"
#include "tide/status.h"
#include "tide/string_builder.h"
#include "tide/workspace.h"

TideStatus tide_app_render_demo(size_t width, size_t height, TideStringBuilder *out);
TideStatus tide_app_render_file_demo(const char *path, size_t width, size_t height, TideStringBuilder *out);
TideStatus tide_app_execute_editor_command(TideEditor *editor, const char *command, int *quit);
TideStatus tide_app_execute_workspace_command(TideWorkspace *workspace, const char *command, int *quit);
int tide_app_run(void);
int tide_app_run_file(const char *path);
void tide_app_request_shutdown(void);

#endif
