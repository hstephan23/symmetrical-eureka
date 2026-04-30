#include "tide/app.h"
#include "tide/buffer.h"
#include "tide/editor.h"
#include "tide/string_builder.h"
#include "test_support.h"

#include <stdio.h>
#include <string.h>

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

int main(void)
{
    test_demo_render_contains_title_and_status();
    test_editor_render_demo_contains_file_text();
    test_write_command_saves_buffer();
    test_unknown_command_stays_open_and_sets_status();
    return 0;
}
