#ifndef TIDE_TEST_SUPPORT_H
#define TIDE_TEST_SUPPORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TIDE_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "%s:%d: assertion failed: %s\n", __FILE__, __LINE__, #expr); \
            exit(1); \
        } \
    } while (0)

#define TIDE_ASSERT_STR_EQ(actual, expected) \
    do { \
        const char *actual_value = (actual); \
        const char *expected_value = (expected); \
        if (strcmp(actual_value, expected_value) != 0) { \
            fprintf(stderr, "%s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, expected_value, actual_value); \
            exit(1); \
        } \
    } while (0)

#endif
