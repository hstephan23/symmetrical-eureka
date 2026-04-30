#include "tide/buffer.h"

#include <stdlib.h>
#include <string.h>

static TideStatus line_init(TideLine *line)
{
    line->capacity = 16;
    line->length = 0;
    line->data = malloc(line->capacity);
    if (line->data == NULL) {
        line->capacity = 0;
        return TIDE_ERR_ALLOC;
    }
    line->data[0] = '\0';
    return TIDE_OK;
}

static void line_free(TideLine *line)
{
    free(line->data);
    line->data = NULL;
    line->length = 0;
    line->capacity = 0;
}

static TideStatus line_reserve(TideLine *line, size_t needed)
{
    size_t next_capacity = line->capacity;

    if (needed <= line->capacity) {
        return TIDE_OK;
    }

    while (next_capacity < needed) {
        if (next_capacity > ((size_t)-1) / 2) {
            return TIDE_ERR_ALLOC;
        }
        next_capacity *= 2;
    }

    char *next_data = realloc(line->data, next_capacity);
    if (next_data == NULL) {
        return TIDE_ERR_ALLOC;
    }

    line->data = next_data;
    line->capacity = next_capacity;
    return TIDE_OK;
}

static TideStatus line_insert_char(TideLine *line, size_t column, char ch)
{
    if (column > line->length) {
        return TIDE_ERR_INVALID;
    }

    TideStatus status = line_reserve(line, line->length + 2);
    if (status != TIDE_OK) {
        return status;
    }

    memmove(line->data + column + 1, line->data + column, line->length - column + 1);
    line->data[column] = ch;
    line->length++;
    return TIDE_OK;
}

static TideStatus line_append_bytes(TideLine *line, const char *data, size_t length)
{
    TideStatus status = line_reserve(line, line->length + length + 1);
    if (status != TIDE_OK) {
        return status;
    }

    memcpy(line->data + line->length, data, length);
    line->length += length;
    line->data[line->length] = '\0';
    return TIDE_OK;
}

static TideStatus ensure_line_capacity(TideBuffer *buffer, size_t needed)
{
    size_t next_capacity = buffer->line_capacity;

    if (needed <= buffer->line_capacity) {
        return TIDE_OK;
    }

    while (next_capacity < needed) {
        if (next_capacity > ((size_t)-1) / 2) {
            return TIDE_ERR_ALLOC;
        }
        next_capacity *= 2;
    }

    TideLine *next_lines = realloc(buffer->lines, next_capacity * sizeof(*buffer->lines));
    if (next_lines == NULL) {
        return TIDE_ERR_ALLOC;
    }

    buffer->lines = next_lines;
    buffer->line_capacity = next_capacity;
    return TIDE_OK;
}

static TideStatus insert_empty_line(TideBuffer *buffer, size_t index)
{
    if (index > buffer->line_count) {
        return TIDE_ERR_INVALID;
    }

    TideStatus status = ensure_line_capacity(buffer, buffer->line_count + 1);
    if (status != TIDE_OK) {
        return status;
    }

    memmove(buffer->lines + index + 1, buffer->lines + index, (buffer->line_count - index) * sizeof(*buffer->lines));
    status = line_init(&buffer->lines[index]);
    if (status != TIDE_OK) {
        memmove(buffer->lines + index, buffer->lines + index + 1, (buffer->line_count - index) * sizeof(*buffer->lines));
        return status;
    }

    buffer->line_count++;
    return TIDE_OK;
}

TideStatus tide_buffer_init(TideBuffer *buffer)
{
    buffer->line_capacity = 8;
    buffer->line_count = 0;
    buffer->path = NULL;
    buffer->dirty = 0;
    buffer->lines = calloc(buffer->line_capacity, sizeof(*buffer->lines));
    if (buffer->lines == NULL) {
        buffer->line_capacity = 0;
        return TIDE_ERR_ALLOC;
    }

    TideStatus status = line_init(&buffer->lines[0]);
    if (status != TIDE_OK) {
        free(buffer->lines);
        buffer->lines = NULL;
        buffer->line_capacity = 0;
        return status;
    }

    buffer->line_count = 1;
    return TIDE_OK;
}

void tide_buffer_free(TideBuffer *buffer)
{
    for (size_t i = 0; i < buffer->line_count; ++i) {
        line_free(&buffer->lines[i]);
    }
    free(buffer->lines);
    free(buffer->path);
    buffer->lines = NULL;
    buffer->line_count = 0;
    buffer->line_capacity = 0;
    buffer->path = NULL;
    buffer->dirty = 0;
}

TideStatus tide_buffer_insert_char(TideBuffer *buffer, size_t line, size_t column, char ch)
{
    if (line >= buffer->line_count) {
        return TIDE_ERR_INVALID;
    }

    TideStatus status = line_insert_char(&buffer->lines[line], column, ch);
    if (status == TIDE_OK) {
        buffer->dirty = 1;
    }
    return status;
}

TideStatus tide_buffer_insert_newline(TideBuffer *buffer, size_t line, size_t column)
{
    if (line >= buffer->line_count || column > buffer->lines[line].length) {
        return TIDE_ERR_INVALID;
    }

    TideLine *current = &buffer->lines[line];
    size_t tail_length = current->length - column;

    TideStatus status = insert_empty_line(buffer, line + 1);
    if (status != TIDE_OK) {
        return status;
    }

    current = &buffer->lines[line];
    TideLine *next = &buffer->lines[line + 1];
    status = line_append_bytes(next, current->data + column, tail_length);
    if (status != TIDE_OK) {
        return status;
    }

    current->length = column;
    current->data[column] = '\0';
    buffer->dirty = 1;
    return TIDE_OK;
}

TideStatus tide_buffer_delete_before(TideBuffer *buffer, TideBufferPosition cursor, TideBufferPosition *next_cursor)
{
    if (cursor.line >= buffer->line_count || cursor.column > buffer->lines[cursor.line].length) {
        return TIDE_ERR_INVALID;
    }

    if (cursor.column > 0) {
        TideLine *line = &buffer->lines[cursor.line];
        memmove(line->data + cursor.column - 1, line->data + cursor.column, line->length - cursor.column + 1);
        line->length--;
        buffer->dirty = 1;
        *next_cursor = (TideBufferPosition){cursor.line, cursor.column - 1};
        return TIDE_OK;
    }

    if (cursor.line == 0) {
        *next_cursor = cursor;
        return TIDE_OK;
    }

    TideLine *previous = &buffer->lines[cursor.line - 1];
    TideLine *current = &buffer->lines[cursor.line];
    size_t join_column = previous->length;
    TideStatus status = line_append_bytes(previous, current->data, current->length);
    if (status != TIDE_OK) {
        return status;
    }

    line_free(current);
    memmove(buffer->lines + cursor.line, buffer->lines + cursor.line + 1, (buffer->line_count - cursor.line - 1) * sizeof(*buffer->lines));
    buffer->line_count--;
    buffer->dirty = 1;
    *next_cursor = (TideBufferPosition){cursor.line - 1, join_column};
    return TIDE_OK;
}

const char *tide_buffer_line_text(const TideBuffer *buffer, size_t line)
{
    if (line >= buffer->line_count) {
        return "";
    }
    return buffer->lines[line].data;
}

size_t tide_buffer_line_length(const TideBuffer *buffer, size_t line)
{
    if (line >= buffer->line_count) {
        return 0;
    }
    return buffer->lines[line].length;
}
