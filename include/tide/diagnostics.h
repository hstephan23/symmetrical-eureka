#ifndef TIDE_DIAGNOSTICS_H
#define TIDE_DIAGNOSTICS_H

#include <stddef.h>

#include "tide/status.h"

#define TIDE_DIAGNOSTIC_PATH_CAPACITY 256
#define TIDE_DIAGNOSTIC_MESSAGE_CAPACITY 256

typedef enum TideDiagnosticSeverity {
    TIDE_DIAGNOSTIC_ERROR = 1,
    TIDE_DIAGNOSTIC_WARNING,
    TIDE_DIAGNOSTIC_NOTE
} TideDiagnosticSeverity;

typedef struct TideDiagnostic {
    char path[TIDE_DIAGNOSTIC_PATH_CAPACITY];
    size_t line;
    size_t column;
    TideDiagnosticSeverity severity;
    char message[TIDE_DIAGNOSTIC_MESSAGE_CAPACITY];
} TideDiagnostic;

typedef struct TideDiagnostics {
    TideDiagnostic *items;
    size_t count;
    size_t capacity;
    size_t current;
} TideDiagnostics;

TideStatus tide_diagnostics_init(TideDiagnostics *diagnostics);
void tide_diagnostics_free(TideDiagnostics *diagnostics);
void tide_diagnostics_clear(TideDiagnostics *diagnostics);
TideStatus tide_diagnostics_parse_output(TideDiagnostics *diagnostics, const char *output);
size_t tide_diagnostics_count(const TideDiagnostics *diagnostics);
size_t tide_diagnostics_current_index(const TideDiagnostics *diagnostics);
const TideDiagnostic *tide_diagnostics_at(const TideDiagnostics *diagnostics, size_t index);
const TideDiagnostic *tide_diagnostics_current(const TideDiagnostics *diagnostics);
TideStatus tide_diagnostics_next(TideDiagnostics *diagnostics);
TideStatus tide_diagnostics_previous(TideDiagnostics *diagnostics);
const char *tide_diagnostic_severity_label(TideDiagnosticSeverity severity);

#endif
