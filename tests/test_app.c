#include "tide/app.h"
#include "tide/buffer.h"
#include "tide/editor.h"
#include "tide/string_builder.h"
#include "tide/workspace.h"
#include "test_support.h"

#include <stdio.h>
#include <string.h>

static void write_text_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");
    TIDE_ASSERT(file != NULL);
    fputs(text, file);
    fclose(file);
}

static void test_demo_render_contains_title_and_status(void)
{
    TideStringBuilder out;

    TIDE_ASSERT(tide_string_builder_init(&out) == TIDE_OK);
    TIDE_ASSERT(tide_app_render_demo(20, 5, &out) == TIDE_OK);

    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "tide") != NULL);
    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "q: quit") != NULL);

    tide_string_builder_free(&out);
}

static void test_editor_render_demo_contains_file_text(void)
{
    TideStringBuilder out;
    const char *path = "test-app-open.txt";
    FILE *file = fopen(path, "w");
    TIDE_ASSERT(file != NULL);
    fputs("hello\n", file);
    fclose(file);

    TIDE_ASSERT(tide_string_builder_init(&out) == TIDE_OK);
    TIDE_ASSERT(tide_app_render_file_demo(path, 24, 5, &out) == TIDE_OK);
    TIDE_ASSERT(strstr(tide_string_builder_data(&out), "hello") != NULL);

    tide_string_builder_free(&out);
}

static void test_write_command_saves_buffer(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;
    const char *path = "test-app-command-write.txt";

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_set_path(&buffer, path) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'x') == TIDE_OK);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "write", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT(buffer.dirty == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "saved");
    tide_buffer_free(&buffer);
}

static void test_unknown_command_stays_open_and_sets_status(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "bogus", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "unknown command: bogus");
    tide_buffer_free(&buffer);
}

static void test_find_command_moves_cursor(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'b') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'a') == TIDE_OK);
    editor.cursor = (TideBufferPosition){0, 1};

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "find a", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 2);
    tide_buffer_free(&buffer);
}

static void test_next_without_search_sets_status(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "next", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "no active search");
    tide_buffer_free(&buffer);
}

static void test_undo_command_reverts_edit(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'x') == TIDE_OK);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "undo", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "");
    tide_buffer_free(&buffer);
}

static void test_redo_command_reapplies_edit(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'x') == TIDE_OK);
    TIDE_ASSERT(tide_editor_undo(&editor) == TIDE_OK);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "redo", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "x");
    tide_buffer_free(&buffer);
}

static void test_open_command_loads_file_and_resets_editor(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;
    const char *old_path = "test-app-open-command-old.txt";
    const char *new_path = "test-app-open-command-new.txt";

    write_text_file(old_path, "old");
    write_text_file(new_path, "one\ntwo");
    TIDE_ASSERT(tide_buffer_load_file(&buffer, old_path) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    editor.cursor = (TideBufferPosition){0, 2};
    editor.viewport_line = 4;
    editor.viewport_column = 3;

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "open test-app-open-command-new.txt", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(buffer.path, new_path);
    TIDE_ASSERT(buffer.line_count == 2);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "one");
    TIDE_ASSERT_STR_EQ(buffer.lines[1].data, "two");
    TIDE_ASSERT(buffer.dirty == 0);
    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 0);
    TIDE_ASSERT(editor.viewport_line == 0);
    TIDE_ASSERT(editor.viewport_column == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "opened: test-app-open-command-new.txt");
    tide_buffer_free(&buffer);
}

static void test_open_command_refuses_dirty_buffer(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'x') == TIDE_OK);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "open test-app-open-refuse.txt", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "x");
    TIDE_ASSERT_STR_EQ(editor.status, "unsaved changes; write first");
    tide_buffer_free(&buffer);
}

static void test_open_command_requires_path(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "open   ", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "path required");
    tide_buffer_free(&buffer);
}

static void test_reload_command_reloads_current_file_and_resets_editor(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;
    const char *path = "test-app-reload-command.txt";

    write_text_file(path, "old");
    TIDE_ASSERT(tide_buffer_load_file(&buffer, path) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    editor.cursor = (TideBufferPosition){0, 2};
    editor.viewport_line = 4;
    write_text_file(path, "new");

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "reload", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(buffer.path, path);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "new");
    TIDE_ASSERT(buffer.dirty == 0);
    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 0);
    TIDE_ASSERT(editor.viewport_line == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "reloaded");
    tide_buffer_free(&buffer);
}

static void test_reload_command_refuses_dirty_buffer(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;
    const char *path = "test-app-reload-refuse.txt";

    write_text_file(path, "old");
    TIDE_ASSERT(tide_buffer_load_file(&buffer, path) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'x') == TIDE_OK);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "reload", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "xold");
    TIDE_ASSERT_STR_EQ(editor.status, "unsaved changes; write first");
    tide_buffer_free(&buffer);
}

static void test_reload_command_requires_file_path(void)
{
    TideBuffer buffer;
    TideEditor editor;
    int quit = 1;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);

    TIDE_ASSERT(tide_app_execute_editor_command(&editor, "reload", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "no file to reload");
    tide_buffer_free(&buffer);
}

static void test_workspace_open_command_adds_buffer_without_replacing_dirty_current(void)
{
    TideWorkspace workspace;
    TideEditor *editor;
    int quit = 1;

    write_text_file("test-app-workspace-one.txt", "one");
    write_text_file("test-app-workspace-two.txt", "two");

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-app-workspace-one.txt") == TIDE_OK);
    editor = tide_workspace_current_editor(&workspace);
    TIDE_ASSERT(editor != NULL);
    TIDE_ASSERT(tide_editor_insert_char(editor, 'x') == TIDE_OK);

    TIDE_ASSERT(tide_app_execute_workspace_command(&workspace, "open test-app-workspace-two.txt", &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT(tide_workspace_count(&workspace) == 2);
    TIDE_ASSERT(tide_workspace_current_index(&workspace) == 1);
    TIDE_ASSERT_STR_EQ(tide_workspace_current_editor(&workspace)->buffer->lines[0].data, "two");
    TIDE_ASSERT(tide_workspace_switch_to(&workspace, 0) == TIDE_OK);
    TIDE_ASSERT_STR_EQ(tide_workspace_current_editor(&workspace)->buffer->lines[0].data, "xone");
    TIDE_ASSERT(tide_workspace_current_editor(&workspace)->buffer->dirty == 1);

    tide_workspace_free(&workspace);
}

static void test_workspace_buffer_commands_switch_buffers(void)
{
    TideWorkspace workspace;
    int quit = 1;

    write_text_file("test-app-buffer-one.txt", "one");
    write_text_file("test-app-buffer-two.txt", "two");

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-app-buffer-one.txt") == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-app-buffer-two.txt") == TIDE_OK);

    TIDE_ASSERT(tide_app_execute_workspace_command(&workspace, "buffer 1", &quit) == TIDE_OK);
    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT(tide_workspace_current_index(&workspace) == 0);

    TIDE_ASSERT(tide_app_execute_workspace_command(&workspace, "bn", &quit) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_current_index(&workspace) == 1);

    TIDE_ASSERT(tide_app_execute_workspace_command(&workspace, "bp", &quit) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_current_index(&workspace) == 0);

    tide_workspace_free(&workspace);
}

static void test_workspace_buffers_command_lists_open_buffers(void)
{
    TideWorkspace workspace;
    TideEditor *editor;
    int quit = 1;

    write_text_file("test-app-list-one.txt", "one");
    write_text_file("test-app-list-two.txt", "two");

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-app-list-one.txt") == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-app-list-two.txt") == TIDE_OK);

    TIDE_ASSERT(tide_app_execute_workspace_command(&workspace, "buffers", &quit) == TIDE_OK);

    editor = tide_workspace_current_editor(&workspace);
    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT(editor != NULL);
    TIDE_ASSERT(strstr(editor->status, "1:test-app-list-one.txt") != NULL);
    TIDE_ASSERT(strstr(editor->status, "2:test-app-list-two.txt") != NULL);

    tide_workspace_free(&workspace);
}

static void test_prompt_resolution_executes_selected_fuzzy_command(void)
{
    TideWorkspace workspace;
    TideEditor *editor;
    char command[64];
    int quit = 1;

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-app-palette-undo.txt") == TIDE_OK);
    editor = tide_workspace_current_editor(&workspace);
    TIDE_ASSERT(editor != NULL);
    TIDE_ASSERT(tide_editor_insert_char(editor, 'x') == TIDE_OK);
    tide_editor_open_command_prompt(editor);
    TIDE_ASSERT(tide_editor_command_insert_char(editor, 'u') == TIDE_OK);
    TIDE_ASSERT(tide_editor_command_insert_char(editor, 'n') == TIDE_OK);

    TIDE_ASSERT(tide_app_resolve_prompt_command(editor, command, sizeof(command)) == TIDE_OK);
    TIDE_ASSERT_STR_EQ(command, "undo");
    tide_editor_cancel_command_prompt(editor);
    TIDE_ASSERT(tide_app_execute_workspace_command(&workspace, command, &quit) == TIDE_OK);

    TIDE_ASSERT(quit == 0);
    TIDE_ASSERT_STR_EQ(tide_workspace_current_editor(&workspace)->buffer->lines[0].data, "");

    tide_workspace_free(&workspace);
}

static void test_prompt_resolution_preserves_argument_command(void)
{
    TideBuffer buffer;
    TideEditor editor;
    char command[64];

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    tide_editor_open_command_prompt(&editor);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'f') == TIDE_OK);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'i') == TIDE_OK);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'n') == TIDE_OK);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'd') == TIDE_OK);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, ' ') == TIDE_OK);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'm') == TIDE_OK);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'i') == TIDE_OK);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'n') == TIDE_OK);

    TIDE_ASSERT(tide_app_resolve_prompt_command(&editor, command, sizeof(command)) == TIDE_OK);

    TIDE_ASSERT_STR_EQ(command, "find main");
    tide_buffer_free(&buffer);
}

int main(void)
{
    test_demo_render_contains_title_and_status();
    test_editor_render_demo_contains_file_text();
    test_write_command_saves_buffer();
    test_unknown_command_stays_open_and_sets_status();
    test_find_command_moves_cursor();
    test_next_without_search_sets_status();
    test_undo_command_reverts_edit();
    test_redo_command_reapplies_edit();
    test_open_command_loads_file_and_resets_editor();
    test_open_command_refuses_dirty_buffer();
    test_open_command_requires_path();
    test_reload_command_reloads_current_file_and_resets_editor();
    test_reload_command_refuses_dirty_buffer();
    test_reload_command_requires_file_path();
    test_workspace_open_command_adds_buffer_without_replacing_dirty_current();
    test_workspace_buffer_commands_switch_buffers();
    test_workspace_buffers_command_lists_open_buffers();
    test_prompt_resolution_executes_selected_fuzzy_command();
    test_prompt_resolution_preserves_argument_command();
    return 0;
}
