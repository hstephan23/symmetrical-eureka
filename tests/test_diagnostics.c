#include "tide/diagnostics.h"
#include "test_support.h"

#include <string.h>

static void test_parses_clang_errors_warnings_and_notes(void)
{
    TideDiagnostics diagnostics;

    TIDE_ASSERT(tide_diagnostics_init(&diagnostics) == TIDE_OK);
    TIDE_ASSERT(tide_diagnostics_parse_output(
                    &diagnostics,
                    "src/main.c:12:5: error: expected ';' after expression\n"
                    "src/lib.c:8:3: warning: unused variable 'x'\n"
                    "include/lib.h:4:1: note: expanded from macro 'LIB'\n") == TIDE_OK);

    TIDE_ASSERT(tide_diagnostics_count(&diagnostics) == 3);
    TIDE_ASSERT_STR_EQ(tide_diagnostics_at(&diagnostics, 0)->path, "src/main.c");
    TIDE_ASSERT(tide_diagnostics_at(&diagnostics, 0)->line == 12);
    TIDE_ASSERT(tide_diagnostics_at(&diagnostics, 0)->column == 5);
    TIDE_ASSERT(tide_diagnostics_at(&diagnostics, 0)->severity == TIDE_DIAGNOSTIC_ERROR);
    TIDE_ASSERT_STR_EQ(tide_diagnostics_at(&diagnostics, 0)->message, "expected ';' after expression");
    TIDE_ASSERT(tide_diagnostics_at(&diagnostics, 1)->severity == TIDE_DIAGNOSTIC_WARNING);
    TIDE_ASSERT(tide_diagnostics_at(&diagnostics, 2)->severity == TIDE_DIAGNOSTIC_NOTE);

    tide_diagnostics_free(&diagnostics);
}

static void test_parses_fatal_error_as_error(void)
{
    TideDiagnostics diagnostics;

    TIDE_ASSERT(tide_diagnostics_init(&diagnostics) == TIDE_OK);
    TIDE_ASSERT(tide_diagnostics_parse_output(
                    &diagnostics,
                    "src/main.c:2:10: fatal error: 'missing.h' file not found\n") == TIDE_OK);

    TIDE_ASSERT(tide_diagnostics_count(&diagnostics) == 1);
    TIDE_ASSERT(tide_diagnostics_at(&diagnostics, 0)->severity == TIDE_DIAGNOSTIC_ERROR);
    TIDE_ASSERT_STR_EQ(tide_diagnostics_at(&diagnostics, 0)->message, "'missing.h' file not found");

    tide_diagnostics_free(&diagnostics);
}

static void test_ignores_non_diagnostic_output(void)
{
    TideDiagnostics diagnostics;

    TIDE_ASSERT(tide_diagnostics_init(&diagnostics) == TIDE_OK);
    TIDE_ASSERT(tide_diagnostics_parse_output(
                    &diagnostics,
                    "[ 50%] Building C object\n"
                    "src/main.c:abc:5: error: bad line number\n"
                    "src/main.c:12:5: info: unsupported severity\n") == TIDE_OK);

    TIDE_ASSERT(tide_diagnostics_count(&diagnostics) == 0);

    tide_diagnostics_free(&diagnostics);
}

static void test_navigation_wraps(void)
{
    TideDiagnostics diagnostics;

    TIDE_ASSERT(tide_diagnostics_init(&diagnostics) == TIDE_OK);
    TIDE_ASSERT(tide_diagnostics_parse_output(
                    &diagnostics,
                    "a.c:1:1: error: first\n"
                    "b.c:2:3: warning: second\n") == TIDE_OK);

    TIDE_ASSERT(tide_diagnostics_current_index(&diagnostics) == 0);
    TIDE_ASSERT(tide_diagnostics_next(&diagnostics) == TIDE_OK);
    TIDE_ASSERT(tide_diagnostics_current_index(&diagnostics) == 1);
    TIDE_ASSERT_STR_EQ(tide_diagnostics_current(&diagnostics)->path, "b.c");
    TIDE_ASSERT(tide_diagnostics_next(&diagnostics) == TIDE_OK);
    TIDE_ASSERT(tide_diagnostics_current_index(&diagnostics) == 0);
    TIDE_ASSERT(tide_diagnostics_previous(&diagnostics) == TIDE_OK);
    TIDE_ASSERT(tide_diagnostics_current_index(&diagnostics) == 1);

    tide_diagnostics_free(&diagnostics);
}

int main(void)
{
    test_parses_clang_errors_warnings_and_notes();
    test_parses_fatal_error_as_error();
    test_ignores_non_diagnostic_output();
    test_navigation_wraps();
    return 0;
}
