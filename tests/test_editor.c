#include "tide/editor.h"
#include "test_support.h"

static void insert_text(TideEditor *editor, const char *text)
{
    for (size_t i = 0; text[i] != '\0'; ++i) {
        if (text[i] == '\n') {
            TIDE_ASSERT(tide_editor_insert_newline(editor) == TIDE_OK);
        } else {
            TIDE_ASSERT(tide_editor_insert_char(editor, text[i]) == TIDE_OK);
        }
    }
}

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

static void test_command_prompt_collects_text_without_editing_buffer(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    tide_editor_open_command_prompt(&editor);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'w') == TIDE_OK);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'q') == TIDE_OK);

    TIDE_ASSERT(tide_editor_command_active(&editor));
    TIDE_ASSERT_STR_EQ(tide_editor_command_text(&editor), "wq");
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "");
    tide_buffer_free(&buffer);
}

static void test_command_prompt_backspace_and_cancel(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    tide_editor_open_command_prompt(&editor);
    TIDE_ASSERT(tide_editor_command_insert_char(&editor, 'q') == TIDE_OK);
    tide_editor_command_backspace(&editor);
    TIDE_ASSERT_STR_EQ(tide_editor_command_text(&editor), "");
    tide_editor_cancel_command_prompt(&editor);

    TIDE_ASSERT(!tide_editor_command_active(&editor));
    tide_buffer_free(&buffer);
}

static void test_find_moves_cursor_to_first_match_at_or_after_cursor(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    insert_text(&editor, "one two one");
    editor.cursor = (TideBufferPosition){0, 1};

    TIDE_ASSERT(tide_editor_find(&editor, "one") == TIDE_OK);

    TIDE_ASSERT(tide_editor_search_has_match(&editor));
    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 8);
    TIDE_ASSERT(tide_editor_search_match_length(&editor) == 3);
    tide_buffer_free(&buffer);
}

static void test_find_next_and_previous_wrap(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    insert_text(&editor, "one two one");
    editor.cursor = (TideBufferPosition){0, 0};

    TIDE_ASSERT(tide_editor_find(&editor, "one") == TIDE_OK);
    TIDE_ASSERT(editor.cursor.column == 0);
    TIDE_ASSERT(tide_editor_find_next(&editor) == TIDE_OK);
    TIDE_ASSERT(editor.cursor.column == 8);
    TIDE_ASSERT(tide_editor_find_next(&editor) == TIDE_OK);
    TIDE_ASSERT(editor.cursor.column == 0);
    TIDE_ASSERT(tide_editor_find_previous(&editor) == TIDE_OK);
    TIDE_ASSERT(editor.cursor.column == 8);
    tide_buffer_free(&buffer);
}

static void test_find_no_match_sets_status_and_keeps_cursor(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    insert_text(&editor, "alpha");
    editor.cursor = (TideBufferPosition){0, 2};

    TIDE_ASSERT(tide_editor_find(&editor, "zzz") == TIDE_OK);

    TIDE_ASSERT(!tide_editor_search_has_match(&editor));
    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 2);
    TIDE_ASSERT_STR_EQ(editor.status, "no match: zzz");
    tide_buffer_free(&buffer);
}

int main(void)
{
    test_insert_text_advances_cursor();
    test_newline_and_backspace_update_cursor();
    test_arrow_movement_clamps_to_line_lengths();
    test_command_prompt_collects_text_without_editing_buffer();
    test_command_prompt_backspace_and_cancel();
    test_find_moves_cursor_to_first_match_at_or_after_cursor();
    test_find_next_and_previous_wrap();
    test_find_no_match_sets_status_and_keeps_cursor();
    return 0;
}
