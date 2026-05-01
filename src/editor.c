#include "tide/editor.h"

#include <stdio.h>
#include <string.h>

static size_t clamp_column(const TideBuffer *buffer, size_t line, size_t column)
{
    size_t length = tide_buffer_line_length(buffer, line);
    return column > length ? length : column;
}

static void clear_search(TideEditor *editor)
{
    editor->search_query[0] = '\0';
    editor->search_query_length = 0;
    editor->search_match = (TideBufferPosition){0, 0};
    editor->search_match_length = 0;
    editor->search_has_match = 0;
}

static void push_action(TideEditorEditAction *stack, size_t *count, TideEditorEditAction action)
{
    if (*count == TIDE_EDITOR_HISTORY_CAPACITY) {
        memmove(stack, stack + 1, (TIDE_EDITOR_HISTORY_CAPACITY - 1) * sizeof(*stack));
        *count = TIDE_EDITOR_HISTORY_CAPACITY - 1;
    }

    stack[*count] = action;
    (*count)++;
}

static int pop_action(TideEditorEditAction *stack, size_t *count, TideEditorEditAction *action)
{
    if (*count == 0) {
        return 0;
    }

    (*count)--;
    *action = stack[*count];
    return 1;
}

static void record_edit(TideEditor *editor, TideEditorEditAction undo_action)
{
    push_action(editor->undo_stack, &editor->undo_count, undo_action);
    editor->redo_count = 0;
    clear_search(editor);
}

static TideStatus apply_history_action(TideEditor *editor, TideEditorEditAction action, TideEditorEditAction *inverse)
{
    TideBufferPosition next_cursor;
    TideStatus status;

    switch (action.kind) {
    case TIDE_EDITOR_ACTION_INSERT_CHAR:
        status = tide_buffer_insert_char(editor->buffer, action.position.line, action.position.column, action.ch);
        if (status == TIDE_OK) {
            editor->cursor = (TideBufferPosition){action.position.line, action.position.column + 1};
            *inverse = (TideEditorEditAction){TIDE_EDITOR_ACTION_DELETE_CHAR, action.position, action.ch};
        }
        return status;

    case TIDE_EDITOR_ACTION_DELETE_CHAR:
        if (action.position.line >= editor->buffer->line_count ||
            action.position.column >= tide_buffer_line_length(editor->buffer, action.position.line)) {
            return TIDE_ERR_INVALID;
        }
        action.ch = tide_buffer_line_text(editor->buffer, action.position.line)[action.position.column];
        status = tide_buffer_delete_before(
            editor->buffer,
            (TideBufferPosition){action.position.line, action.position.column + 1},
            &next_cursor);
        if (status == TIDE_OK) {
            editor->cursor = next_cursor;
            *inverse = (TideEditorEditAction){TIDE_EDITOR_ACTION_INSERT_CHAR, action.position, action.ch};
        }
        return status;

    case TIDE_EDITOR_ACTION_INSERT_NEWLINE:
        status = tide_buffer_insert_newline(editor->buffer, action.position.line, action.position.column);
        if (status == TIDE_OK) {
            editor->cursor = (TideBufferPosition){action.position.line + 1, 0};
            *inverse = (TideEditorEditAction){
                TIDE_EDITOR_ACTION_DELETE_NEWLINE,
                (TideBufferPosition){action.position.line + 1, 0},
                0};
        }
        return status;

    case TIDE_EDITOR_ACTION_DELETE_NEWLINE:
        if (action.position.line == 0 || action.position.line >= editor->buffer->line_count || action.position.column != 0) {
            return TIDE_ERR_INVALID;
        }

        size_t join_column = tide_buffer_line_length(editor->buffer, action.position.line - 1);
        status = tide_buffer_delete_before(editor->buffer, action.position, &next_cursor);
        if (status == TIDE_OK) {
            editor->cursor = next_cursor;
            *inverse = (TideEditorEditAction){
                TIDE_EDITOR_ACTION_INSERT_NEWLINE,
                (TideBufferPosition){action.position.line - 1, join_column},
                0};
        }
        return status;
    }

    return TIDE_ERR_INVALID;
}

void tide_editor_init(TideEditor *editor, TideBuffer *buffer)
{
    editor->buffer = buffer;
    tide_editor_reset_view(editor);
}

void tide_editor_reset_view(TideEditor *editor)
{
    editor->cursor = (TideBufferPosition){0, 0};
    editor->viewport_line = 0;
    editor->viewport_column = 0;
    editor->status[0] = '\0';
    editor->prompt_mode = TIDE_EDITOR_PROMPT_CLOSED;
    editor->command[0] = '\0';
    editor->command_length = 0;
    editor->command_selection = 0;
    editor->search_query[0] = '\0';
    editor->search_query_length = 0;
    editor->search_match = (TideBufferPosition){0, 0};
    editor->search_match_length = 0;
    editor->search_has_match = 0;
    editor->undo_count = 0;
    editor->redo_count = 0;
}

TideStatus tide_editor_insert_char(TideEditor *editor, char ch)
{
    TideBufferPosition position = editor->cursor;
    TideStatus status = tide_buffer_insert_char(editor->buffer, editor->cursor.line, editor->cursor.column, ch);
    if (status == TIDE_OK) {
        editor->cursor.column++;
        record_edit(editor, (TideEditorEditAction){TIDE_EDITOR_ACTION_DELETE_CHAR, position, ch});
    }
    return status;
}

TideStatus tide_editor_insert_newline(TideEditor *editor)
{
    TideBufferPosition position = editor->cursor;
    TideStatus status = tide_buffer_insert_newline(editor->buffer, editor->cursor.line, editor->cursor.column);
    if (status == TIDE_OK) {
        editor->cursor.line++;
        editor->cursor.column = 0;
        record_edit(
            editor,
            (TideEditorEditAction){
                TIDE_EDITOR_ACTION_DELETE_NEWLINE,
                (TideBufferPosition){position.line + 1, 0},
                0});
    }
    return status;
}

TideStatus tide_editor_backspace(TideEditor *editor)
{
    TideBufferPosition next_cursor;
    TideEditorEditAction undo_action;
    if (editor->cursor.line >= editor->buffer->line_count ||
        editor->cursor.column > tide_buffer_line_length(editor->buffer, editor->cursor.line)) {
        return TIDE_ERR_INVALID;
    }

    if (editor->cursor.line == 0 && editor->cursor.column == 0) {
        return TIDE_OK;
    }

    if (editor->cursor.column > 0) {
        TideBufferPosition position = {editor->cursor.line, editor->cursor.column - 1};
        char deleted = tide_buffer_line_text(editor->buffer, position.line)[position.column];
        undo_action = (TideEditorEditAction){TIDE_EDITOR_ACTION_INSERT_CHAR, position, deleted};
    } else {
        undo_action = (TideEditorEditAction){
            TIDE_EDITOR_ACTION_INSERT_NEWLINE,
            (TideBufferPosition){editor->cursor.line - 1, tide_buffer_line_length(editor->buffer, editor->cursor.line - 1)},
            0};
    }

    TideStatus status = tide_buffer_delete_before(editor->buffer, editor->cursor, &next_cursor);
    if (status == TIDE_OK) {
        editor->cursor = next_cursor;
        record_edit(editor, undo_action);
    }
    return status;
}

TideStatus tide_editor_undo(TideEditor *editor)
{
    TideEditorEditAction action;
    TideEditorEditAction inverse;
    if (!pop_action(editor->undo_stack, &editor->undo_count, &action)) {
        tide_editor_set_status(editor, "nothing to undo");
        return TIDE_OK;
    }

    TideStatus status = apply_history_action(editor, action, &inverse);
    if (status != TIDE_OK) {
        push_action(editor->undo_stack, &editor->undo_count, action);
        return status;
    }

    push_action(editor->redo_stack, &editor->redo_count, inverse);
    clear_search(editor);
    return TIDE_OK;
}

TideStatus tide_editor_redo(TideEditor *editor)
{
    TideEditorEditAction action;
    TideEditorEditAction inverse;
    if (!pop_action(editor->redo_stack, &editor->redo_count, &action)) {
        tide_editor_set_status(editor, "nothing to redo");
        return TIDE_OK;
    }

    TideStatus status = apply_history_action(editor, action, &inverse);
    if (status != TIDE_OK) {
        push_action(editor->redo_stack, &editor->redo_count, action);
        return status;
    }

    push_action(editor->undo_stack, &editor->undo_count, inverse);
    clear_search(editor);
    return TIDE_OK;
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
    editor->command_selection = 0;
    tide_editor_set_status(editor, "");
}

void tide_editor_cancel_command_prompt(TideEditor *editor)
{
    editor->prompt_mode = TIDE_EDITOR_PROMPT_CLOSED;
    editor->command[0] = '\0';
    editor->command_length = 0;
    editor->command_selection = 0;
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
    editor->command_selection = 0;
    return TIDE_OK;
}

void tide_editor_command_backspace(TideEditor *editor)
{
    if (!tide_editor_command_active(editor) || editor->command_length == 0) {
        return;
    }

    editor->command_length--;
    editor->command[editor->command_length] = '\0';
    editor->command_selection = 0;
}

const char *tide_editor_command_text(const TideEditor *editor)
{
    return editor->command;
}

size_t tide_editor_command_selection(const TideEditor *editor)
{
    return editor->command_selection;
}

void tide_editor_command_move_selection(TideEditor *editor, TideEditorMove move, size_t match_count)
{
    if (!tide_editor_command_active(editor) || match_count == 0) {
        editor->command_selection = 0;
        return;
    }

    if (editor->command_selection >= match_count) {
        editor->command_selection = 0;
    }

    if (move == TIDE_EDITOR_MOVE_DOWN) {
        editor->command_selection = (editor->command_selection + 1) % match_count;
    } else if (move == TIDE_EDITOR_MOVE_UP) {
        editor->command_selection = editor->command_selection == 0 ? match_count - 1 : editor->command_selection - 1;
    }
}

static int line_find_between(const char *line, size_t line_length, const char *query, size_t query_length, size_t start_column, size_t end_column, size_t *match_column)
{
    if (query_length == 0 || query_length > line_length || start_column > line_length) {
        return 0;
    }
    if (end_column > line_length) {
        end_column = line_length;
    }
    if (end_column < query_length) {
        return 0;
    }

    size_t last_start = end_column - query_length;
    if (start_column > last_start) {
        return 0;
    }

    for (size_t column = start_column; column <= last_start; ++column) {
        if (memcmp(line + column, query, query_length) == 0) {
            *match_column = column;
            return 1;
        }
    }

    return 0;
}

static int line_find_previous_between(const char *line, size_t line_length, const char *query, size_t query_length, size_t end_column, size_t *match_column)
{
    if (query_length == 0 || query_length > line_length || end_column == 0) {
        return 0;
    }
    if (end_column > line_length + 1) {
        end_column = line_length + 1;
    }

    size_t last_start = line_length - query_length;
    if (end_column - 1 < last_start) {
        last_start = end_column - 1;
    }

    for (size_t column = last_start + 1; column > 0; --column) {
        size_t candidate = column - 1;
        if (memcmp(line + candidate, query, query_length) == 0) {
            *match_column = candidate;
            return 1;
        }
    }

    return 0;
}

static int find_forward_from(TideEditor *editor, const char *query, size_t query_length, TideBufferPosition start, TideBufferPosition *match)
{
    for (size_t line = start.line; line < editor->buffer->line_count; ++line) {
        size_t column;
        size_t line_length = tide_buffer_line_length(editor->buffer, line);
        size_t start_column = line == start.line ? start.column : 0;
        if (line_find_between(tide_buffer_line_text(editor->buffer, line), line_length, query, query_length, start_column, line_length, &column)) {
            *match = (TideBufferPosition){line, column};
            return 1;
        }
    }

    for (size_t line = 0; line <= start.line && line < editor->buffer->line_count; ++line) {
        size_t column;
        size_t line_length = tide_buffer_line_length(editor->buffer, line);
        size_t end_column = line == start.line ? start.column : line_length;
        if (line_find_between(tide_buffer_line_text(editor->buffer, line), line_length, query, query_length, 0, end_column, &column)) {
            *match = (TideBufferPosition){line, column};
            return 1;
        }
    }

    return 0;
}

static int find_previous_from(TideEditor *editor, const char *query, size_t query_length, TideBufferPosition start, TideBufferPosition *match)
{
    for (size_t line = start.line + 1; line > 0; --line) {
        size_t line_index = line - 1;
        size_t column;
        size_t line_length = tide_buffer_line_length(editor->buffer, line_index);
        size_t end_column = line_index == start.line ? start.column : line_length + 1;
        if (line_find_previous_between(tide_buffer_line_text(editor->buffer, line_index), line_length, query, query_length, end_column, &column)) {
            *match = (TideBufferPosition){line_index, column};
            return 1;
        }
    }

    for (size_t line = editor->buffer->line_count; line > start.line; --line) {
        size_t line_index = line - 1;
        size_t column;
        size_t line_length = tide_buffer_line_length(editor->buffer, line_index);
        if (line_find_previous_between(tide_buffer_line_text(editor->buffer, line_index), line_length, query, query_length, line_length + 1, &column)) {
            *match = (TideBufferPosition){line_index, column};
            return 1;
        }
    }

    return 0;
}

static void set_search_match(TideEditor *editor, const char *query, size_t query_length, TideBufferPosition match)
{
    size_t copy_length = query_length;
    if (copy_length >= sizeof(editor->search_query)) {
        copy_length = sizeof(editor->search_query) - 1;
    }

    memcpy(editor->search_query, query, copy_length);
    editor->search_query[copy_length] = '\0';
    editor->search_query_length = copy_length;
    editor->search_match = match;
    editor->search_match_length = copy_length;
    editor->search_has_match = 1;
    editor->cursor = match;
    tide_editor_set_status(editor, "");
}

TideStatus tide_editor_find(TideEditor *editor, const char *query)
{
    size_t query_length = strlen(query);
    if (query_length == 0) {
        clear_search(editor);
        tide_editor_set_status(editor, "search query required");
        return TIDE_OK;
    }

    TideBufferPosition original = editor->cursor;
    TideBufferPosition match;
    if (find_forward_from(editor, query, query_length, original, &match)) {
        set_search_match(editor, query, query_length, match);
        return TIDE_OK;
    }

    clear_search(editor);
    editor->cursor = original;
    char message[sizeof(editor->status)];
    snprintf(message, sizeof(message), "no match: %s", query);
    tide_editor_set_status(editor, message);
    return TIDE_OK;
}

TideStatus tide_editor_find_next(TideEditor *editor)
{
    if (editor->search_query_length == 0) {
        tide_editor_set_status(editor, "no active search");
        return TIDE_OK;
    }

    TideBufferPosition start = editor->search_match;
    start.column++;
    TideBufferPosition match;
    if (find_forward_from(editor, editor->search_query, editor->search_query_length, start, &match)) {
        set_search_match(editor, editor->search_query, editor->search_query_length, match);
    }
    return TIDE_OK;
}

TideStatus tide_editor_find_previous(TideEditor *editor)
{
    if (editor->search_query_length == 0) {
        tide_editor_set_status(editor, "no active search");
        return TIDE_OK;
    }

    TideBufferPosition match;
    if (find_previous_from(editor, editor->search_query, editor->search_query_length, editor->search_match, &match)) {
        set_search_match(editor, editor->search_query, editor->search_query_length, match);
    }
    return TIDE_OK;
}

int tide_editor_search_has_match(const TideEditor *editor)
{
    return editor->search_has_match;
}

TideBufferPosition tide_editor_search_match(const TideEditor *editor)
{
    return editor->search_match;
}

size_t tide_editor_search_match_length(const TideEditor *editor)
{
    return editor->search_match_length;
}

const char *tide_editor_search_query(const TideEditor *editor)
{
    return editor->search_query;
}
