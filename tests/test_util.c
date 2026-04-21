/*
 * test_util.c -- unit tests for util.c.
 *
 * Plain C assert() tests; no framework. Each test_* function asserts a
 * specific behavior and main() runs them all in sequence. The program
 * prints "util test passed" on success, or aborts with an "assertion
 * failed" message telling you exactly which line died.
 *
 * Run manually:   ./build/tests/test_util
 * Run via ctest:  ctest --test-dir build --output-on-failure
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

/* Happy path: a trailing '\n' is removed. */
static void test_strips_trailing_newline(void)
{
    char buf[] = "hello\n";
    remove_newline_from_input(buf);
    assert(strcmp(buf, "hello") == 0);
}

/* A string without a trailing newline must NOT have its last char chopped.
 * This was a real bug in the first version of remove_newline_from_input. */
static void test_no_trailing_newline_is_unchanged(void)
{
    char buf[] = "hello";
    remove_newline_from_input(buf);
    assert(strcmp(buf, "hello") == 0);
}

/* A single '\n' should collapse to an empty string. */
static void test_only_newline_becomes_empty(void)
{
    char buf[] = "\n";
    remove_newline_from_input(buf);
    assert(strcmp(buf, "") == 0);
}

/* Passing an empty string must be safe -- no write past the buffer. */
static void test_empty_string_is_safe(void)
{
    char buf[] = "";
    remove_newline_from_input(buf);
    assert(strcmp(buf, "") == 0);
}

int main(void)
{
    test_strips_trailing_newline();
    test_no_trailing_newline_is_unchanged();
    test_only_newline_becomes_empty();
    test_empty_string_is_safe();
    puts("util test passed");
    return 0;
}
