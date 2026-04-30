#ifndef TIDE_BUFFER_H
#define TIDE_BUFFER_H

#include <stddef.h>

#include "tide/status.h"

typedef struct TideLine {
    char *data;
    size_t length;
    size_t capacity;
} TideLine;

typedef struct TideBufferPosition {
    size_t line;
    size_t column;
} TideBufferPosition;

typedef struct TideBuffer {
    TideLine *lines;
    size_t line_count;
    size_t line_capacity;
    char *path;
    int dirty;
} TideBuffer;

TideStatus tide_buffer_init(TideBuffer *buffer);
void tide_buffer_free(TideBuffer *buffer);
TideStatus tide_buffer_insert_char(TideBuffer *buffer, size_t line, size_t column, char ch);
TideStatus tide_buffer_insert_newline(TideBuffer *buffer, size_t line, size_t column);
TideStatus tide_buffer_delete_before(TideBuffer *buffer, TideBufferPosition cursor, TideBufferPosition *next_cursor);
const char *tide_buffer_line_text(const TideBuffer *buffer, size_t line);
size_t tide_buffer_line_length(const TideBuffer *buffer, size_t line);
TideStatus tide_buffer_set_path(TideBuffer *buffer, const char *path);
TideStatus tide_buffer_load_file(TideBuffer *buffer, const char *path);
TideStatus tide_buffer_replace_with_file(TideBuffer *buffer, const char *path);
TideStatus tide_buffer_save(TideBuffer *buffer);

#endif
