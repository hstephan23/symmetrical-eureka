#ifndef TIDE_EDITOR_H
#define TIDE_EDITOR_H

#include <stddef.h>

#include "tide/buffer.h"
#include "tide/status.h"

#define TIDE_EDITOR_COMMAND_CAPACITY 128
#define TIDE_EDITOR_SEARCH_CAPACITY 128
#define TIDE_EDITOR_HISTORY_CAPACITY 256

typedef enum TideEditorPromptMode {
    TIDE_EDITOR_PROMPT_CLOSED = 0,
    TIDE_EDITOR_PROMPT_COMMAND
} TideEditorPromptMode;

typedef enum TideEditorMove {
    TIDE_EDITOR_MOVE_UP,
    TIDE_EDITOR_MOVE_DOWN,
    TIDE_EDITOR_MOVE_LEFT,
    TIDE_EDITOR_MOVE_RIGHT
} TideEditorMove;

typedef enum TideEditorEditActionKind {
    TIDE_EDITOR_ACTION_INSERT_CHAR = 1,
    TIDE_EDITOR_ACTION_DELETE_CHAR,
    TIDE_EDITOR_ACTION_INSERT_NEWLINE,
    TIDE_EDITOR_ACTION_DELETE_NEWLINE
} TideEditorEditActionKind;

typedef struct TideEditorEditAction {
    TideEditorEditActionKind kind;
    TideBufferPosition position;
    char ch;
} TideEditorEditAction;

typedef struct TideEditor {
    TideBuffer *buffer;
    TideBufferPosition cursor;
    size_t viewport_line;
    size_t viewport_column;
    char status[128];
    TideEditorPromptMode prompt_mode;
    char command[TIDE_EDITOR_COMMAND_CAPACITY];
    size_t command_length;
    char search_query[TIDE_EDITOR_SEARCH_CAPACITY];
    size_t search_query_length;
    TideBufferPosition search_match;
    size_t search_match_length;
    int search_has_match;
    TideEditorEditAction undo_stack[TIDE_EDITOR_HISTORY_CAPACITY];
    size_t undo_count;
    TideEditorEditAction redo_stack[TIDE_EDITOR_HISTORY_CAPACITY];
    size_t redo_count;
} TideEditor;

void tide_editor_init(TideEditor *editor, TideBuffer *buffer);
void tide_editor_reset_view(TideEditor *editor);
TideStatus tide_editor_insert_char(TideEditor *editor, char ch);
TideStatus tide_editor_insert_newline(TideEditor *editor);
TideStatus tide_editor_backspace(TideEditor *editor);
TideStatus tide_editor_undo(TideEditor *editor);
TideStatus tide_editor_redo(TideEditor *editor);
void tide_editor_move(TideEditor *editor, TideEditorMove move);
void tide_editor_set_status(TideEditor *editor, const char *message);
void tide_editor_ensure_cursor_visible(TideEditor *editor, size_t width, size_t height);
void tide_editor_open_command_prompt(TideEditor *editor);
void tide_editor_cancel_command_prompt(TideEditor *editor);
int tide_editor_command_active(const TideEditor *editor);
TideStatus tide_editor_command_insert_char(TideEditor *editor, char ch);
void tide_editor_command_backspace(TideEditor *editor);
const char *tide_editor_command_text(const TideEditor *editor);
TideStatus tide_editor_find(TideEditor *editor, const char *query);
TideStatus tide_editor_find_next(TideEditor *editor);
TideStatus tide_editor_find_previous(TideEditor *editor);
int tide_editor_search_has_match(const TideEditor *editor);
TideBufferPosition tide_editor_search_match(const TideEditor *editor);
size_t tide_editor_search_match_length(const TideEditor *editor);
const char *tide_editor_search_query(const TideEditor *editor);

#endif
