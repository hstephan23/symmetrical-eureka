#ifndef TIDE_PROJECT_FILES_H
#define TIDE_PROJECT_FILES_H

#include <stddef.h>

#include "tide/status.h"

typedef struct TideProjectFiles {
    char **paths;
    size_t count;
    size_t capacity;
} TideProjectFiles;

typedef struct TideProjectFileMatch {
    const char *path;
    int score;
} TideProjectFileMatch;

TideStatus tide_project_files_init(TideProjectFiles *files);
void tide_project_files_free(TideProjectFiles *files);
TideStatus tide_project_files_scan(TideProjectFiles *files, const char *root);
size_t tide_project_files_count(const TideProjectFiles *files);
const char *tide_project_files_path(const TideProjectFiles *files, size_t index);
TideStatus tide_project_files_add(TideProjectFiles *files, const char *path);
size_t tide_project_files_filter(
    const TideProjectFiles *files,
    const char *query,
    TideProjectFileMatch *matches,
    size_t capacity);

#endif
