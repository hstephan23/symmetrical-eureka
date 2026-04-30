#include "tide/editor.h"
#include "test_support.h"

static void test_insert_text_advances_cursor(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'b') == TIDE_OK);

    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 2);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "ab");
    tide_buffer_free(&buffer);
}

static void test_newline_and_backspace_update_cursor(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_newline(&editor) == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'b') == TIDE_OK);
    TIDE_ASSERT(tide_editor_backspace(&editor) == TIDE_OK);
    TIDE_ASSERT(tide_editor_backspace(&editor) == TIDE_OK);

    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 1);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "a");
    tide_buffer_free(&buffer);
}

static void test_arrow_movement_clamps_to_line_lengths(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_newline(&editor) == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'b') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'c') == TIDE_OK);
    tide_editor_move(&editor, TIDE_EDITOR_MOVE_UP);

    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 1);
    tide_buffer_free(&buffer);
}

int main(void)
{
    test_insert_text_advances_cursor();
    test_newline_and_backspace_update_cursor();
    test_arrow_movement_clamps_to_line_lengths();
    return 0;
}
