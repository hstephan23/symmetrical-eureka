#ifndef TIDE_EDITOR_H
#define TIDE_EDITOR_H

#include <stddef.h>

#include "tide/buffer.h"
#include "tide/status.h"

typedef enum TideEditorMove {
    TIDE_EDITOR_MOVE_UP,
    TIDE_EDITOR_MOVE_DOWN,
    TIDE_EDITOR_MOVE_LEFT,
    TIDE_EDITOR_MOVE_RIGHT
} TideEditorMove;

typedef struct TideEditor {
    TideBuffer *buffer;
    TideBufferPosition cursor;
    size_t viewport_line;
    size_t viewport_column;
    char status[128];
} TideEditor;

void tide_editor_init(TideEditor *editor, TideBuffer *buffer);
TideStatus tide_editor_insert_char(TideEditor *editor, char ch);
TideStatus tide_editor_insert_newline(TideEditor *editor);
TideStatus tide_editor_backspace(TideEditor *editor);
void tide_editor_move(TideEditor *editor, TideEditorMove move);
void tide_editor_set_status(TideEditor *editor, const char *message);
void tide_editor_ensure_cursor_visible(TideEditor *editor, size_t width, size_t height);

#endif
