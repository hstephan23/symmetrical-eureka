#include "tide/project_files.h"
#include "test_support.h"

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

static void make_dir(const char *path)
{
    if (mkdir(path, 0777) != 0 && errno != EEXIST) {
        TIDE_ASSERT(0);
    }
}

static void write_text_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");
    TIDE_ASSERT(file != NULL);
    fputs(text, file);
    fclose(file);
}

static void create_fixture(void)
{
    make_dir("test-project-files-root");
    make_dir("test-project-files-root/src");
    make_dir("test-project-files-root/include");
    make_dir("test-project-files-root/build");
    make_dir("test-project-files-root/.git");
    write_text_file("test-project-files-root/src/main.c", "int main(void) { return 0; }\n");
    write_text_file("test-project-files-root/src/editor.c", "void editor(void) {}\n");
    write_text_file("test-project-files-root/include/editor.h", "void editor(void);\n");
    write_text_file("test-project-files-root/build/generated.c", "ignored\n");
    write_text_file("test-project-files-root/.git/config", "ignored\n");
}

static int files_contain(const TideProjectFiles *files, const char *path)
{
    for (size_t i = 0; i < tide_project_files_count(files); ++i) {
        if (strcmp(tide_project_files_path(files, i), path) == 0) {
            return 1;
        }
    }

    return 0;
}

static void test_scan_discovers_relative_files_and_ignores_generated_dirs(void)
{
    TideProjectFiles files;

    create_fixture();
    TIDE_ASSERT(tide_project_files_init(&files) == TIDE_OK);
    TIDE_ASSERT(tide_project_files_scan(&files, "test-project-files-root") == TIDE_OK);

    TIDE_ASSERT(files_contain(&files, "src/main.c"));
    TIDE_ASSERT(files_contain(&files, "src/editor.c"));
    TIDE_ASSERT(files_contain(&files, "include/editor.h"));
    TIDE_ASSERT(!files_contain(&files, "build/generated.c"));
    TIDE_ASSERT(!files_contain(&files, ".git/config"));

    tide_project_files_free(&files);
}

static void test_filter_ranks_matching_files(void)
{
    TideProjectFiles files;
    TideProjectFileMatch matches[4];
    size_t count;

    create_fixture();
    TIDE_ASSERT(tide_project_files_init(&files) == TIDE_OK);
    TIDE_ASSERT(tide_project_files_scan(&files, "test-project-files-root") == TIDE_OK);

    count = tide_project_files_filter(&files, "main", matches, 4);

    TIDE_ASSERT(count >= 1);
    TIDE_ASSERT_STR_EQ(matches[0].path, "src/main.c");

    tide_project_files_free(&files);
}

static void test_filter_matches_hyphenated_file_queries(void)
{
    TideProjectFiles files;
    TideProjectFileMatch matches[2];
    size_t count;

    TIDE_ASSERT(tide_project_files_init(&files) == TIDE_OK);
    TIDE_ASSERT(tide_project_files_add(&files, "test-app-fuzzy-open-unique-target.c") == TIDE_OK);

    count = tide_project_files_filter(&files, "fuzzyuniquetarget", matches, 2);

    TIDE_ASSERT(count == 1);
    TIDE_ASSERT_STR_EQ(matches[0].path, "test-app-fuzzy-open-unique-target.c");

    tide_project_files_free(&files);
}

int main(void)
{
    test_scan_discovers_relative_files_and_ignores_generated_dirs();
    test_filter_ranks_matching_files();
    test_filter_matches_hyphenated_file_queries();
    return 0;
}
