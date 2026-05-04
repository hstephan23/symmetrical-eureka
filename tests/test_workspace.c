#include "tide/workspace.h"
#include "test_support.h"

#include <stdio.h>

static void write_text_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");
    TIDE_ASSERT(file != NULL);
    fputs(text, file);
    fclose(file);
}

static void test_open_files_switches_current_buffer(void)
{
    TideWorkspace workspace;
    TideEditor *editor;

    write_text_file("test-workspace-one.txt", "one");
    write_text_file("test-workspace-two.txt", "two");

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-workspace-one.txt") == TIDE_OK);
    editor = tide_workspace_current_editor(&workspace);
    TIDE_ASSERT(editor != NULL);
    editor->cursor = (TideBufferPosition){0, 2};

    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-workspace-two.txt") == TIDE_OK);
    TIDE_ASSERT(tide_workspace_count(&workspace) == 2);
    TIDE_ASSERT(tide_workspace_current_index(&workspace) == 1);
    TIDE_ASSERT_STR_EQ(tide_workspace_current_editor(&workspace)->buffer->lines[0].data, "two");

    TIDE_ASSERT(tide_workspace_switch_to(&workspace, 0) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_current_editor(&workspace)->cursor.column == 2);
    TIDE_ASSERT_STR_EQ(tide_workspace_current_editor(&workspace)->buffer->lines[0].data, "one");

    tide_workspace_free(&workspace);
}

static void test_open_existing_path_reuses_buffer(void)
{
    TideWorkspace workspace;

    write_text_file("test-workspace-existing.txt", "same");
    write_text_file("test-workspace-other.txt", "other");

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-workspace-existing.txt") == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-workspace-other.txt") == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-workspace-existing.txt") == TIDE_OK);

    TIDE_ASSERT(tide_workspace_count(&workspace) == 2);
    TIDE_ASSERT(tide_workspace_current_index(&workspace) == 0);

    tide_workspace_free(&workspace);
}

static void test_next_and_previous_wrap(void)
{
    TideWorkspace workspace;

    write_text_file("test-workspace-a.txt", "a");
    write_text_file("test-workspace-b.txt", "b");

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-workspace-a.txt") == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-workspace-b.txt") == TIDE_OK);

    TIDE_ASSERT(tide_workspace_next(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_current_index(&workspace) == 0);
    TIDE_ASSERT(tide_workspace_previous(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_current_index(&workspace) == 1);

    tide_workspace_free(&workspace);
}

static void test_growth_keeps_editor_buffer_pointers_valid(void)
{
    TideWorkspace workspace;
    char path[32];

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);

    for (size_t i = 0; i < 5; ++i) {
        snprintf(path, sizeof(path), "test-workspace-grow-%zu.txt", i);
        write_text_file(path, "x");
        TIDE_ASSERT(tide_workspace_open_file(&workspace, path) == TIDE_OK);
    }

    TIDE_ASSERT(tide_workspace_switch_to(&workspace, 0) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_current_editor(&workspace)->buffer == &workspace.entries[0].buffer);
    TIDE_ASSERT(tide_editor_insert_char(tide_workspace_current_editor(&workspace), 'a') == TIDE_OK);
    TIDE_ASSERT_STR_EQ(workspace.entries[0].buffer.lines[0].data, "ax");

    tide_workspace_free(&workspace);
}

static void test_open_text_adds_and_replaces_named_buffer(void)
{
    TideWorkspace workspace;

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_text(&workspace, "*build-output*", "one\ntwo") == TIDE_OK);

    TIDE_ASSERT(tide_workspace_count(&workspace) == 1);
    TIDE_ASSERT(tide_workspace_current_index(&workspace) == 0);
    TIDE_ASSERT_STR_EQ(tide_workspace_current_editor(&workspace)->buffer->path, "*build-output*");
    TIDE_ASSERT_STR_EQ(tide_workspace_current_editor(&workspace)->buffer->lines[0].data, "one");
    TIDE_ASSERT_STR_EQ(tide_workspace_current_editor(&workspace)->buffer->lines[1].data, "two");
    TIDE_ASSERT(tide_workspace_current_editor(&workspace)->buffer->dirty == 0);

    TIDE_ASSERT(tide_workspace_open_text(&workspace, "*build-output*", "fresh") == TIDE_OK);

    TIDE_ASSERT(tide_workspace_count(&workspace) == 1);
    TIDE_ASSERT_STR_EQ(tide_workspace_current_editor(&workspace)->buffer->lines[0].data, "fresh");
    TIDE_ASSERT(tide_workspace_current_editor(&workspace)->cursor.line == 0);
    TIDE_ASSERT(tide_workspace_current_editor(&workspace)->cursor.column == 0);

    tide_workspace_free(&workspace);
}

static void test_workspace_owns_and_clears_diagnostics(void)
{
    TideWorkspace workspace;

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_parse_diagnostics(
                    &workspace,
                    "test-workspace-diag.c:7:2: warning: be careful\n") == TIDE_OK);

    TIDE_ASSERT(tide_diagnostics_count(tide_workspace_diagnostics(&workspace)) == 1);
    TIDE_ASSERT_STR_EQ(tide_diagnostics_current(tide_workspace_diagnostics(&workspace))->path, "test-workspace-diag.c");

    TIDE_ASSERT(tide_workspace_parse_diagnostics(&workspace, "clean build\n") == TIDE_OK);
    TIDE_ASSERT(tide_diagnostics_count(tide_workspace_diagnostics(&workspace)) == 0);

    tide_workspace_free(&workspace);
}

int main(void)
{
    test_open_files_switches_current_buffer();
    test_open_existing_path_reuses_buffer();
    test_next_and_previous_wrap();
    test_growth_keeps_editor_buffer_pointers_valid();
    test_open_text_adds_and_replaces_named_buffer();
    test_workspace_owns_and_clears_diagnostics();
    return 0;
}
