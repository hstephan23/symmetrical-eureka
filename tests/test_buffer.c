#include "tide/buffer.h"
#include "test_support.h"

#include <stdio.h>

static void test_buffer_starts_with_one_empty_line(void)
{
    TideBuffer buffer;
    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(buffer.line_count == 1);
    TIDE_ASSERT(buffer.lines[0].length == 0);
    TIDE_ASSERT(buffer.dirty == 0);
    tide_buffer_free(&buffer);
}

static void test_insert_char_and_newline(void)
{
    TideBuffer buffer;
    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 0, 0, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 0, 1, 'b') == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_newline(&buffer, 0, 1) == TIDE_OK);
    TIDE_ASSERT(buffer.line_count == 2);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "a");
    TIDE_ASSERT_STR_EQ(buffer.lines[1].data, "b");
    TIDE_ASSERT(buffer.dirty == 1);
    tide_buffer_free(&buffer);
}

static void test_delete_before_cursor_joins_lines(void)
{
    TideBuffer buffer;
    TideBufferPosition pos;
    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 0, 0, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_newline(&buffer, 0, 1) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 1, 0, 'b') == TIDE_OK);

    TIDE_ASSERT(tide_buffer_delete_before(&buffer, (TideBufferPosition){1, 0}, &pos) == TIDE_OK);

    TIDE_ASSERT(buffer.line_count == 1);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "ab");
    TIDE_ASSERT(pos.line == 0);
    TIDE_ASSERT(pos.column == 1);
    tide_buffer_free(&buffer);
}

static void test_load_file_splits_lines(void)
{
    TideBuffer buffer;
    const char *path = "test-buffer-load.txt";
    FILE *file = fopen(path, "w");
    TIDE_ASSERT(file != NULL);
    fputs("one\ntwo\n", file);
    fclose(file);

    TIDE_ASSERT(tide_buffer_load_file(&buffer, path) == TIDE_OK);
    TIDE_ASSERT(buffer.line_count == 2);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "one");
    TIDE_ASSERT_STR_EQ(buffer.lines[1].data, "two");
    TIDE_ASSERT(buffer.dirty == 0);
    TIDE_ASSERT_STR_EQ(buffer.path, path);
    tide_buffer_free(&buffer);
}

static void test_save_file_clears_dirty(void)
{
    TideBuffer buffer;
    const char *path = "test-buffer-save.txt";

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_set_path(&buffer, path) == TIDE_OK);
    TIDE_ASSERT(tide_buffer_insert_char(&buffer, 0, 0, 'x') == TIDE_OK);
    TIDE_ASSERT(tide_buffer_save(&buffer) == TIDE_OK);
    TIDE_ASSERT(buffer.dirty == 0);

    FILE *file = fopen(path, "r");
    char content[8] = {0};
    TIDE_ASSERT(file != NULL);
    TIDE_ASSERT(fread(content, 1, sizeof(content) - 1, file) == 1);
    fclose(file);
    TIDE_ASSERT_STR_EQ(content, "x");
    tide_buffer_free(&buffer);
}

int main(void)
{
    test_buffer_starts_with_one_empty_line();
    test_insert_char_and_newline();
    test_delete_before_cursor_joins_lines();
    test_load_file_splits_lines();
    test_save_file_clears_dirty();
    return 0;
}
