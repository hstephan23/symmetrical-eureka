#ifndef TIDE_APP_H
#define TIDE_APP_H

#include <stddef.h>

#include "tide/status.h"
#include "tide/string_builder.h"

TideStatus tide_app_render_demo(size_t width, size_t height, TideStringBuilder *out);
TideStatus tide_app_render_file_demo(const char *path, size_t width, size_t height, TideStringBuilder *out);
int tide_app_run(void);
int tide_app_run_file(const char *path);
void tide_app_request_shutdown(void);

#endif
