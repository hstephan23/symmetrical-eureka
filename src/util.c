/*
 * util.c -- implementations of the helpers declared in util.h.
 */

#include "util.h"

#include <string.h>

void remove_newline_from_input(char *str)
{
    /* Guard against two edge cases:
     *   - empty string: strlen == 0, so str[n-1] would be str[-1] (UB).
     *   - no trailing newline (e.g., EOF without '\n'): don't chop a real char.
     */
    const size_t n = strlen(str);
    if (n > 0 && str[n - 1] == '\n') str[n - 1] = '\0';
}
