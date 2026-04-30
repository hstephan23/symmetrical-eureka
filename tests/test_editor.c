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

static void test_undo_and_redo_insert_char(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'x') == TIDE_OK);
    TIDE_ASSERT(tide_editor_undo(&editor) == TIDE_OK);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "");
    TIDE_ASSERT(editor.cursor.column == 0);
    TIDE_ASSERT(tide_editor_redo(&editor) == TIDE_OK);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "x");
    TIDE_ASSERT(editor.cursor.column == 1);
    tide_buffer_free(&buffer);
}

static void test_undo_backspace_restores_deleted_char(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'b') == TIDE_OK);
    TIDE_ASSERT(tide_editor_backspace(&editor) == TIDE_OK);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "a");
    TIDE_ASSERT(tide_editor_undo(&editor) == TIDE_OK);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "ab");
    TIDE_ASSERT(editor.cursor.column == 2);
    tide_buffer_free(&buffer);
}

static void test_undo_newline_and_join(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_newline(&editor) == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'b') == TIDE_OK);
    TIDE_ASSERT(buffer.line_count == 2);
    TIDE_ASSERT(tide_editor_undo(&editor) == TIDE_OK);
    TIDE_ASSERT(tide_editor_undo(&editor) == TIDE_OK);
    TIDE_ASSERT(buffer.line_count == 1);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "a");
    TIDE_ASSERT(tide_editor_redo(&editor) == TIDE_OK);
    TIDE_ASSERT(buffer.line_count == 2);
    tide_buffer_free(&buffer);
}

static void test_new_edit_after_undo_clears_redo(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'a') == TIDE_OK);
    TIDE_ASSERT(tide_editor_undo(&editor) == TIDE_OK);
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'b') == TIDE_OK);
    TIDE_ASSERT(tide_editor_redo(&editor) == TIDE_OK);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "b");
    TIDE_ASSERT_STR_EQ(editor.status, "nothing to redo");
    tide_buffer_free(&buffer);
}

static void test_edit_clears_search_match(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    insert_text(&editor, "abc");
    editor.cursor = (TideBufferPosition){0, 0};
    TIDE_ASSERT(tide_editor_find(&editor, "b") == TIDE_OK);
    TIDE_ASSERT(tide_editor_search_has_match(&editor));
    TIDE_ASSERT(tide_editor_insert_char(&editor, 'x') == TIDE_OK);
    TIDE_ASSERT(!tide_editor_search_has_match(&editor));
    tide_buffer_free(&buffer);
}

static void test_reset_view_clears_editor_state(void)
{
    TideBuffer buffer;
    TideEditor editor;

    TIDE_ASSERT(tide_buffer_init(&buffer) == TIDE_OK);
    tide_editor_init(&editor, &buffer);
    insert_text(&editor, "abc");
    editor.cursor = (TideBufferPosition){0, 0};
    TIDE_ASSERT(tide_editor_find(&editor, "b") == TIDE_OK);
    TIDE_ASSERT(tide_editor_search_has_match(&editor));
    editor.viewport_line = 3;
    editor.viewport_column = 2;
    tide_editor_set_status(&editor, "status");

    tide_editor_reset_view(&editor);

    TIDE_ASSERT(editor.buffer == &buffer);
    TIDE_ASSERT(editor.cursor.line == 0);
    TIDE_ASSERT(editor.cursor.column == 0);
    TIDE_ASSERT(editor.viewport_line == 0);
    TIDE_ASSERT(editor.viewport_column == 0);
    TIDE_ASSERT_STR_EQ(editor.status, "");
    TIDE_ASSERT(!tide_editor_search_has_match(&editor));
    TIDE_ASSERT(tide_editor_undo(&editor) == TIDE_OK);
    TIDE_ASSERT_STR_EQ(buffer.lines[0].data, "abc");
    TIDE_ASSERT_STR_EQ(editor.status, "nothing to undo");
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
    test_undo_and_redo_insert_char();
    test_undo_backspace_restores_deleted_char();
    test_undo_newline_and_join();
    test_new_edit_after_undo_clears_redo();
    test_edit_clears_search_match();
    test_reset_view_clears_editor_state();
    return 0;
}
