#include "tide/editor.h"

#include <string.h>

static size_t clamp_column(const TideBuffer *buffer, size_t line, size_t column)
{
    size_t length = tide_buffer_line_length(buffer, line);
    return column > length ? length : column;
}

void tide_editor_init(TideEditor *editor, TideBuffer *buffer)
{
    editor->buffer = buffer;
    editor->cursor = (TideBufferPosition){0, 0};
    editor->viewport_line = 0;
    editor->viewport_column = 0;
    editor->status[0] = '\0';
    editor->prompt_mode = TIDE_EDITOR_PROMPT_CLOSED;
    editor->command[0] = '\0';
    editor->command_length = 0;
}

TideStatus tide_editor_insert_char(TideEditor *editor, char ch)
{
    TideStatus status = tide_buffer_insert_char(editor->buffer, editor->cursor.line, editor->cursor.column, ch);
    if (status == TIDE_OK) {
        editor->cursor.column++;
    }
    return status;
}

TideStatus tide_editor_insert_newline(TideEditor *editor)
{
    TideStatus status = tide_buffer_insert_newline(editor->buffer, editor->cursor.line, editor->cursor.column);
    if (status == TIDE_OK) {
        editor->cursor.line++;
        editor->cursor.column = 0;
    }
    return status;
}

TideStatus tide_editor_backspace(TideEditor *editor)
{
    TideBufferPosition next_cursor;
    TideStatus status = tide_buffer_delete_before(editor->buffer, editor->cursor, &next_cursor);
    if (status == TIDE_OK) {
        editor->cursor = next_cursor;
    }
    return status;
}

void tide_editor_move(TideEditor *editor, TideEditorMove move)
{
    switch (move) {
    case TIDE_EDITOR_MOVE_UP:
        if (editor->cursor.line > 0) {
            editor->cursor.line--;
            editor->cursor.column = clamp_column(editor->buffer, editor->cursor.line, editor->cursor.column);
        }
        break;
    case TIDE_EDITOR_MOVE_DOWN:
        if (editor->cursor.line + 1 < editor->buffer->line_count) {
            editor->cursor.line++;
            editor->cursor.column = clamp_column(editor->buffer, editor->cursor.line, editor->cursor.column);
        }
        break;
    case TIDE_EDITOR_MOVE_LEFT:
        if (editor->cursor.column > 0) {
            editor->cursor.column--;
        } else if (editor->cursor.line > 0) {
            editor->cursor.line--;
            editor->cursor.column = tide_buffer_line_length(editor->buffer, editor->cursor.line);
        }
        break;
    case TIDE_EDITOR_MOVE_RIGHT:
        if (editor->cursor.column < tide_buffer_line_length(editor->buffer, editor->cursor.line)) {
            editor->cursor.column++;
        } else if (editor->cursor.line + 1 < editor->buffer->line_count) {
            editor->cursor.line++;
            editor->cursor.column = 0;
        }
        break;
    }
}

void tide_editor_set_status(TideEditor *editor, const char *message)
{
    strncpy(editor->status, message, sizeof(editor->status) - 1);
    editor->status[sizeof(editor->status) - 1] = '\0';
}

void tide_editor_ensure_cursor_visible(TideEditor *editor, size_t width, size_t height)
{
    size_t editable_height = height > 1 ? height - 1 : 1;

    if (editor->cursor.line < editor->viewport_line) {
        editor->viewport_line = editor->cursor.line;
    } else if (editor->cursor.line >= editor->viewport_line + editable_height) {
        editor->viewport_line = editor->cursor.line - editable_height + 1;
    }

    if (editor->cursor.column < editor->viewport_column) {
        editor->viewport_column = editor->cursor.column;
    } else if (width > 0 && editor->cursor.column >= editor->viewport_column + width) {
        editor->viewport_column = editor->cursor.column - width + 1;
    }
}

void tide_editor_open_command_prompt(TideEditor *editor)
{
    editor->prompt_mode = TIDE_EDITOR_PROMPT_COMMAND;
    editor->command[0] = '\0';
    editor->command_length = 0;
    tide_editor_set_status(editor, "");
}

void tide_editor_cancel_command_prompt(TideEditor *editor)
{
    editor->prompt_mode = TIDE_EDITOR_PROMPT_CLOSED;
    editor->command[0] = '\0';
    editor->command_length = 0;
}

int tide_editor_command_active(const TideEditor *editor)
{
    return editor->prompt_mode == TIDE_EDITOR_PROMPT_COMMAND;
}

TideStatus tide_editor_command_insert_char(TideEditor *editor, char ch)
{
    if (!tide_editor_command_active(editor)) {
        return TIDE_ERR_INVALID;
    }
    if (editor->command_length + 1 >= sizeof(editor->command)) {
        return TIDE_OK;
    }

    editor->command[editor->command_length++] = ch;
    editor->command[editor->command_length] = '\0';
    return TIDE_OK;
}

void tide_editor_command_backspace(TideEditor *editor)
{
    if (!tide_editor_command_active(editor) || editor->command_length == 0) {
        return;
    }

    editor->command_length--;
    editor->command[editor->command_length] = '\0';
}

const char *tide_editor_command_text(const TideEditor *editor)
{
    return editor->command;
}
