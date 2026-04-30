#include "tide/buffer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static TideStatus insert_empty_line(TideBuffer *buffer, size_t index);

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

static char *copy_string(const char *value)
{
    size_t length = strlen(value);
    char *copy = malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, value, length + 1);
    return copy;
}

static TideStatus reset_to_empty(TideBuffer *buffer)
{
    for (size_t i = 0; i < buffer->line_count; ++i) {
        line_free(&buffer->lines[i]);
    }
    buffer->line_count = 0;
    return insert_empty_line(buffer, 0);
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

TideStatus tide_buffer_set_path(TideBuffer *buffer, const char *path)
{
    char *path_copy = copy_string(path);
    if (path_copy == NULL) {
        return TIDE_ERR_ALLOC;
    }

    free(buffer->path);
    buffer->path = path_copy;
    return TIDE_OK;
}

static TideStatus append_loaded_line(TideBuffer *buffer, const char *start, size_t length)
{
    TideStatus status;

    if (buffer->line_count == 0) {
        status = insert_empty_line(buffer, 0);
        if (status != TIDE_OK) {
            return status;
        }
    } else if (buffer->lines[buffer->line_count - 1].length > 0) {
        status = insert_empty_line(buffer, buffer->line_count);
        if (status != TIDE_OK) {
            return status;
        }
    }

    TideLine *line = &buffer->lines[buffer->line_count - 1];
    return line_append_bytes(line, start, length);
}

static TideStatus load_bytes(TideBuffer *buffer, const char *data, size_t length)
{
    TideStatus status = reset_to_empty(buffer);
    if (status != TIDE_OK) {
        return status;
    }
    buffer->lines[0].length = 0;
    buffer->lines[0].data[0] = '\0';

    size_t line_start = 0;
    for (size_t i = 0; i < length; ++i) {
        if (data[i] == '\n') {
            status = append_loaded_line(buffer, data + line_start, i - line_start);
            if (status != TIDE_OK) {
                return status;
            }
            line_start = i + 1;
        }
    }

    if (line_start < length) {
        status = append_loaded_line(buffer, data + line_start, length - line_start);
        if (status != TIDE_OK) {
            return status;
        }
    }

    if (buffer->line_count == 0) {
        return insert_empty_line(buffer, 0);
    }

    return TIDE_OK;
}

TideStatus tide_buffer_load_file(TideBuffer *buffer, const char *path)
{
    TideStatus status = tide_buffer_init(buffer);
    if (status != TIDE_OK) {
        return status;
    }

    status = tide_buffer_set_path(buffer, path);
    if (status != TIDE_OK) {
        tide_buffer_free(buffer);
        return status;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        if (errno == ENOENT) {
            buffer->dirty = 0;
            return TIDE_OK;
        }
        tide_buffer_free(buffer);
        return TIDE_ERR_IO;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        tide_buffer_free(buffer);
        return TIDE_ERR_IO;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        tide_buffer_free(buffer);
        return TIDE_ERR_IO;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        tide_buffer_free(buffer);
        return TIDE_ERR_IO;
    }

    char *data = malloc((size_t)file_size + 1);
    if (data == NULL) {
        fclose(file);
        tide_buffer_free(buffer);
        return TIDE_ERR_ALLOC;
    }

    size_t nread = fread(data, 1, (size_t)file_size, file);
    fclose(file);
    if (nread != (size_t)file_size) {
        free(data);
        tide_buffer_free(buffer);
        return TIDE_ERR_IO;
    }
    data[nread] = '\0';

    status = load_bytes(buffer, data, nread);
    free(data);
    if (status != TIDE_OK) {
        tide_buffer_free(buffer);
        return status;
    }

    buffer->dirty = 0;
    return TIDE_OK;
}

static TideStatus write_buffer(FILE *file, const TideBuffer *buffer)
{
    for (size_t i = 0; i < buffer->line_count; ++i) {
        TideLine line = buffer->lines[i];
        if (line.length > 0 && fwrite(line.data, 1, line.length, file) != line.length) {
            return TIDE_ERR_IO;
        }
        if (i + 1 < buffer->line_count && fputc('\n', file) == EOF) {
            return TIDE_ERR_IO;
        }
    }
    return TIDE_OK;
}

TideStatus tide_buffer_save(TideBuffer *buffer)
{
    if (buffer->path == NULL) {
        return TIDE_ERR_INVALID;
    }

    size_t path_length = strlen(buffer->path);
    char *temp_path = malloc(path_length + 5);
    if (temp_path == NULL) {
        return TIDE_ERR_ALLOC;
    }
    memcpy(temp_path, buffer->path, path_length);
    memcpy(temp_path + path_length, ".tmp", 5);

    FILE *file = fopen(temp_path, "wb");
    if (file == NULL) {
        free(temp_path);
        return TIDE_ERR_IO;
    }

    TideStatus status = write_buffer(file, buffer);
    if (status == TIDE_OK && fflush(file) != 0) {
        status = TIDE_ERR_IO;
    }
    if (fclose(file) != 0 && status == TIDE_OK) {
        status = TIDE_ERR_IO;
    }

    if (status == TIDE_OK && rename(temp_path, buffer->path) != 0) {
        status = TIDE_ERR_IO;
    }

    if (status != TIDE_OK) {
        unlink(temp_path);
        free(temp_path);
        return status;
    }

    free(temp_path);
    buffer->dirty = 0;
    return TIDE_OK;
}
