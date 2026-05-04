#include "tide/diagnostics.h"

#include <stdlib.h>
#include <string.h>

static int is_digit(int ch)
{
    return ch >= '0' && ch <= '9';
}

static const char *find_char(const char *start, const char *end, char ch)
{
    while (start < end) {
        if (*start == ch) {
            return start;
        }
        start++;
    }
    return NULL;
}

static void copy_span(char *out, size_t out_size, const char *start, size_t length)
{
    if (out_size == 0) {
        return;
    }

    if (length >= out_size) {
        length = out_size - 1;
    }

    memcpy(out, start, length);
    out[length] = '\0';
}

static TideStatus ensure_capacity(TideDiagnostics *diagnostics, size_t needed)
{
    size_t next_capacity = diagnostics->capacity == 0 ? 4 : diagnostics->capacity;

    if (needed <= diagnostics->capacity) {
        return TIDE_OK;
    }

    while (next_capacity < needed) {
        if (next_capacity > ((size_t)-1) / 2) {
            return TIDE_ERR_ALLOC;
        }
        next_capacity *= 2;
    }

    TideDiagnostic *next_items = realloc(diagnostics->items, next_capacity * sizeof(*diagnostics->items));
    if (next_items == NULL) {
        return TIDE_ERR_ALLOC;
    }

    diagnostics->items = next_items;
    diagnostics->capacity = next_capacity;
    return TIDE_OK;
}

static int parse_positive_number(const char **cursor, const char *end, size_t *value)
{
    size_t parsed = 0;
    const char *p = *cursor;

    if (p >= end || !is_digit((unsigned char)*p)) {
        return 0;
    }

    while (p < end && is_digit((unsigned char)*p)) {
        size_t digit = (size_t)(*p - '0');
        if (parsed > (((size_t)-1) - digit) / 10) {
            return 0;
        }
        parsed = parsed * 10 + digit;
        p++;
    }

    if (parsed == 0 || p >= end || *p != ':') {
        return 0;
    }

    *value = parsed;
    *cursor = p + 1;
    return 1;
}

static int parse_severity(const char *start, size_t length, TideDiagnosticSeverity *severity)
{
    while (length > 0 && start[length - 1] == ' ') {
        length--;
    }

    if (length == 5 && strncmp(start, "error", length) == 0) {
        *severity = TIDE_DIAGNOSTIC_ERROR;
        return 1;
    }
    if (length == 11 && strncmp(start, "fatal error", length) == 0) {
        *severity = TIDE_DIAGNOSTIC_ERROR;
        return 1;
    }
    if (length == 7 && strncmp(start, "warning", length) == 0) {
        *severity = TIDE_DIAGNOSTIC_WARNING;
        return 1;
    }
    if (length == 4 && strncmp(start, "note", length) == 0) {
        *severity = TIDE_DIAGNOSTIC_NOTE;
        return 1;
    }

    return 0;
}

static int parse_diagnostic_line(const char *line, size_t line_length, TideDiagnostic *diagnostic)
{
    const char *line_end = line + line_length;
    const char *path_end = find_char(line, line_end, ':');
    const char *p;
    const char *severity_end;
    size_t path_length;
    size_t message_length;

    if (path_end == NULL || path_end == line) {
        return 0;
    }

    path_length = (size_t)(path_end - line);
    p = path_end + 1;
    if (!parse_positive_number(&p, line_end, &diagnostic->line)) {
        return 0;
    }
    if (!parse_positive_number(&p, line_end, &diagnostic->column)) {
        return 0;
    }

    while (p < line_end && *p == ' ') {
        p++;
    }

    severity_end = find_char(p, line_end, ':');
    if (severity_end == NULL || !parse_severity(p, (size_t)(severity_end - p), &diagnostic->severity)) {
        return 0;
    }

    p = severity_end + 1;
    while (p < line_end && *p == ' ') {
        p++;
    }

    while (line_end > p && line_end[-1] == '\r') {
        line_end--;
    }

    message_length = (size_t)(line_end - p);
    copy_span(diagnostic->path, sizeof(diagnostic->path), line, path_length);
    copy_span(diagnostic->message, sizeof(diagnostic->message), p, message_length);
    return 1;
}

static TideStatus append_diagnostic(TideDiagnostics *diagnostics, const TideDiagnostic *diagnostic)
{
    TideStatus status = ensure_capacity(diagnostics, diagnostics->count + 1);
    if (status != TIDE_OK) {
        return status;
    }

    diagnostics->items[diagnostics->count++] = *diagnostic;
    return TIDE_OK;
}

TideStatus tide_diagnostics_init(TideDiagnostics *diagnostics)
{
    diagnostics->items = NULL;
    diagnostics->count = 0;
    diagnostics->capacity = 0;
    diagnostics->current = 0;
    return TIDE_OK;
}

void tide_diagnostics_free(TideDiagnostics *diagnostics)
{
    free(diagnostics->items);
    diagnostics->items = NULL;
    diagnostics->count = 0;
    diagnostics->capacity = 0;
    diagnostics->current = 0;
}

void tide_diagnostics_clear(TideDiagnostics *diagnostics)
{
    diagnostics->count = 0;
    diagnostics->current = 0;
}

TideStatus tide_diagnostics_parse_output(TideDiagnostics *diagnostics, const char *output)
{
    const char *line_start;

    if (diagnostics == NULL || output == NULL) {
        return TIDE_ERR_INVALID;
    }

    tide_diagnostics_clear(diagnostics);
    line_start = output;
    for (;;) {
        const char *line_end = strchr(line_start, '\n');
        TideDiagnostic diagnostic;
        size_t line_length;

        if (line_end == NULL) {
            line_end = line_start + strlen(line_start);
        }
        line_length = (size_t)(line_end - line_start);

        if (line_length > 0 && parse_diagnostic_line(line_start, line_length, &diagnostic)) {
            TideStatus status = append_diagnostic(diagnostics, &diagnostic);
            if (status != TIDE_OK) {
                return status;
            }
        }

        if (*line_end == '\0') {
            break;
        }
        line_start = line_end + 1;
    }

    diagnostics->current = 0;
    return TIDE_OK;
}

size_t tide_diagnostics_count(const TideDiagnostics *diagnostics)
{
    return diagnostics->count;
}

size_t tide_diagnostics_current_index(const TideDiagnostics *diagnostics)
{
    return diagnostics->current;
}

const TideDiagnostic *tide_diagnostics_at(const TideDiagnostics *diagnostics, size_t index)
{
    if (index >= diagnostics->count) {
        return NULL;
    }
    return &diagnostics->items[index];
}

const TideDiagnostic *tide_diagnostics_current(const TideDiagnostics *diagnostics)
{
    return tide_diagnostics_at(diagnostics, diagnostics->current);
}

TideStatus tide_diagnostics_next(TideDiagnostics *diagnostics)
{
    if (diagnostics->count == 0) {
        return TIDE_ERR_INVALID;
    }

    diagnostics->current = (diagnostics->current + 1) % diagnostics->count;
    return TIDE_OK;
}

TideStatus tide_diagnostics_previous(TideDiagnostics *diagnostics)
{
    if (diagnostics->count == 0) {
        return TIDE_ERR_INVALID;
    }

    diagnostics->current = diagnostics->current == 0 ? diagnostics->count - 1 : diagnostics->current - 1;
    return TIDE_OK;
}

const char *tide_diagnostic_severity_label(TideDiagnosticSeverity severity)
{
    switch (severity) {
    case TIDE_DIAGNOSTIC_ERROR:
        return "error";
    case TIDE_DIAGNOSTIC_WARNING:
        return "warning";
    case TIDE_DIAGNOSTIC_NOTE:
        return "note";
    default:
        return "diagnostic";
    }
}
