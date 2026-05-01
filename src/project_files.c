#include "tide/project_files.h"

#include "tide/command_palette.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char *copy_string(const char *value)
{
    size_t length = strlen(value);
    char *copy = malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, value, length + 1);
    return copy;
}

static TideStatus join_path(const char *left, const char *right, char **out)
{
    size_t left_length = strlen(left);
    size_t right_length = strlen(right);
    int needs_slash = left_length > 0 && left[left_length - 1] != '/';
    char *path = malloc(left_length + (needs_slash ? 1 : 0) + right_length + 1);
    if (path == NULL) {
        return TIDE_ERR_ALLOC;
    }

    memcpy(path, left, left_length);
    if (needs_slash) {
        path[left_length] = '/';
    }
    memcpy(path + left_length + (needs_slash ? 1 : 0), right, right_length + 1);
    *out = path;
    return TIDE_OK;
}

static TideStatus ensure_capacity(TideProjectFiles *files, size_t needed)
{
    size_t next_capacity = files->capacity == 0 ? 16 : files->capacity;

    if (needed <= files->capacity) {
        return TIDE_OK;
    }

    while (next_capacity < needed) {
        if (next_capacity > ((size_t)-1) / 2) {
            return TIDE_ERR_ALLOC;
        }
        next_capacity *= 2;
    }

    char **next_paths = realloc(files->paths, next_capacity * sizeof(*files->paths));
    if (next_paths == NULL) {
        return TIDE_ERR_ALLOC;
    }

    files->paths = next_paths;
    files->capacity = next_capacity;
    return TIDE_OK;
}

TideStatus tide_project_files_add(TideProjectFiles *files, const char *path)
{
    TideStatus status = ensure_capacity(files, files->count + 1);
    if (status != TIDE_OK) {
        return status;
    }

    char *copy = copy_string(path);
    if (copy == NULL) {
        return TIDE_ERR_ALLOC;
    }

    files->paths[files->count++] = copy;
    return TIDE_OK;
}

static int should_ignore_dir(const char *name)
{
    return strcmp(name, ".git") == 0 ||
           strcmp(name, "build") == 0 ||
           strcmp(name, "build-asan") == 0 ||
           strcmp(name, "cmake-build-debug") == 0 ||
           strcmp(name, ".idea") == 0 ||
           strcmp(name, ".superpowers") == 0;
}

static TideStatus scan_dir(TideProjectFiles *files, const char *root, const char *relative)
{
    char *directory_path = NULL;
    DIR *directory;
    struct dirent *entry;
    TideStatus status;

    if (relative[0] == '\0') {
        directory_path = copy_string(root);
        if (directory_path == NULL) {
            return TIDE_ERR_ALLOC;
        }
    } else {
        status = join_path(root, relative, &directory_path);
        if (status != TIDE_OK) {
            return status;
        }
    }

    directory = opendir(directory_path);
    if (directory == NULL) {
        free(directory_path);
        return TIDE_ERR_IO;
    }

    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char *child_relative = NULL;
        char *child_full = NULL;
        if (relative[0] == '\0') {
            child_relative = copy_string(entry->d_name);
            if (child_relative == NULL) {
                status = TIDE_ERR_ALLOC;
                goto fail;
            }
        } else {
            status = join_path(relative, entry->d_name, &child_relative);
            if (status != TIDE_OK) {
                goto fail;
            }
        }

        status = join_path(root, child_relative, &child_full);
        if (status != TIDE_OK) {
            free(child_relative);
            goto fail;
        }

        struct stat info;
        if (lstat(child_full, &info) != 0) {
            free(child_full);
            free(child_relative);
            status = TIDE_ERR_IO;
            goto fail;
        }

        if (S_ISDIR(info.st_mode)) {
            if (!should_ignore_dir(entry->d_name)) {
                status = scan_dir(files, root, child_relative);
                if (status != TIDE_OK) {
                    free(child_full);
                    free(child_relative);
                    goto fail;
                }
            }
        } else if (S_ISREG(info.st_mode)) {
            status = tide_project_files_add(files, child_relative);
            if (status != TIDE_OK) {
                free(child_full);
                free(child_relative);
                goto fail;
            }
        }

        free(child_full);
        free(child_relative);
    }

    closedir(directory);
    free(directory_path);
    return TIDE_OK;

fail:
    closedir(directory);
    free(directory_path);
    return status;
}

static void insert_match(TideProjectFileMatch *matches, size_t *count, size_t capacity, TideProjectFileMatch candidate)
{
    size_t insert_at = *count;

    while (insert_at > 0 && candidate.score > matches[insert_at - 1].score) {
        if (insert_at < capacity) {
            matches[insert_at] = matches[insert_at - 1];
        }
        insert_at--;
    }

    if (insert_at >= capacity) {
        return;
    }

    if (*count < capacity) {
        (*count)++;
    }

    for (size_t i = *count - 1; i > insert_at; --i) {
        matches[i] = matches[i - 1];
    }

    matches[insert_at] = candidate;
}

TideStatus tide_project_files_init(TideProjectFiles *files)
{
    files->paths = NULL;
    files->count = 0;
    files->capacity = 0;
    return TIDE_OK;
}

void tide_project_files_free(TideProjectFiles *files)
{
    for (size_t i = 0; i < files->count; ++i) {
        free(files->paths[i]);
    }
    free(files->paths);
    files->paths = NULL;
    files->count = 0;
    files->capacity = 0;
}

TideStatus tide_project_files_scan(TideProjectFiles *files, const char *root)
{
    return scan_dir(files, root, "");
}

size_t tide_project_files_count(const TideProjectFiles *files)
{
    return files->count;
}

const char *tide_project_files_path(const TideProjectFiles *files, size_t index)
{
    if (index >= files->count) {
        return "";
    }

    return files->paths[index];
}

size_t tide_project_files_filter(
    const TideProjectFiles *files,
    const char *query,
    TideProjectFileMatch *matches,
    size_t capacity)
{
    size_t count = 0;
    if (capacity == 0 || query == NULL || query[0] == '\0') {
        return 0;
    }

    for (size_t i = 0; i < files->count; ++i) {
        int score = tide_command_palette_match_score(files->paths[i], query);
        if (score > 0) {
            insert_match(matches, &count, capacity, (TideProjectFileMatch){files->paths[i], score});
        }
    }

    return count;
}
