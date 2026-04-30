#ifndef TIDE_STRING_BUILDER_H
#define TIDE_STRING_BUILDER_H

#include <stddef.h>

#include "tide/status.h"

typedef struct TideStringBuilder {
    char *data;
    size_t length;
    size_t capacity;
} TideStringBuilder;

TideStatus tide_string_builder_init(TideStringBuilder *builder);
void tide_string_builder_free(TideStringBuilder *builder);
TideStatus tide_string_builder_append(TideStringBuilder *builder, const char *text);
TideStatus tide_string_builder_append_char(TideStringBuilder *builder, char ch);
TideStatus tide_string_builder_append_format(TideStringBuilder *builder, const char *format, size_t a, size_t b);
const char *tide_string_builder_data(const TideStringBuilder *builder);
size_t tide_string_builder_length(const TideStringBuilder *builder);

#endif
