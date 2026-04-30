#include "tide/string_builder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TideStatus ensure_capacity(TideStringBuilder *builder, size_t additional)
{
    size_t needed = builder->length + additional + 1;
    size_t next_capacity = builder->capacity;

    if (needed <= builder->capacity) {
        return TIDE_OK;
    }

    while (next_capacity < needed) {
        if (next_capacity > ((size_t)-1) / 2) {
            return TIDE_ERR_ALLOC;
        }
        next_capacity *= 2;
    }

    char *next_data = realloc(builder->data, next_capacity);
    if (next_data == NULL) {
        return TIDE_ERR_ALLOC;
    }

    builder->data = next_data;
    builder->capacity = next_capacity;
    return TIDE_OK;
}

TideStatus tide_string_builder_init(TideStringBuilder *builder)
{
    builder->capacity = 64;
    builder->length = 0;
    builder->data = malloc(builder->capacity);
    if (builder->data == NULL) {
        builder->capacity = 0;
        return TIDE_ERR_ALLOC;
    }

    builder->data[0] = '\0';
    return TIDE_OK;
}

void tide_string_builder_free(TideStringBuilder *builder)
{
    free(builder->data);
    builder->data = NULL;
    builder->length = 0;
    builder->capacity = 0;
}

TideStatus tide_string_builder_append(TideStringBuilder *builder, const char *text)
{
    size_t length = strlen(text);
    TideStatus status = ensure_capacity(builder, length);
    if (status != TIDE_OK) {
        return status;
    }

    memcpy(builder->data + builder->length, text, length + 1);
    builder->length += length;
    return TIDE_OK;
}

TideStatus tide_string_builder_append_char(TideStringBuilder *builder, char ch)
{
    TideStatus status = ensure_capacity(builder, 1);
    if (status != TIDE_OK) {
        return status;
    }

    builder->data[builder->length++] = ch;
    builder->data[builder->length] = '\0';
    return TIDE_OK;
}

TideStatus tide_string_builder_append_format(TideStringBuilder *builder, const char *format, size_t a, size_t b)
{
    char stack_buffer[64];
    int written = snprintf(stack_buffer, sizeof(stack_buffer), format, a, b);
    if (written < 0) {
        return TIDE_ERR_INVALID;
    }

    if ((size_t)written < sizeof(stack_buffer)) {
        return tide_string_builder_append(builder, stack_buffer);
    }

    char *heap_buffer = malloc((size_t)written + 1);
    if (heap_buffer == NULL) {
        return TIDE_ERR_ALLOC;
    }

    snprintf(heap_buffer, (size_t)written + 1, format, a, b);
    TideStatus status = tide_string_builder_append(builder, heap_buffer);
    free(heap_buffer);
    return status;
}

const char *tide_string_builder_data(const TideStringBuilder *builder)
{
    return builder->data;
}

size_t tide_string_builder_length(const TideStringBuilder *builder)
{
    return builder->length;
}
