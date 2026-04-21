/*
 * util.h -- small helper functions used throughout mysh.
 *
 * Anything in here should be tiny, pure, and independent of the rest of the
 * shell. If a helper starts depending on jobs/state/etc., it probably belongs
 * in its own module instead.
 */

#ifndef MYSH_UTIL_H
#define MYSH_UTIL_H

/*
 * Strip a single trailing '\n' from `str`, in place.
 *
 * Intended for cleaning up lines returned by getline(), which keeps the
 * newline. Safe to call on an empty string or a string that doesn't end in
 * '\n' -- it simply does nothing in those cases.
 *
 * `str` must be a writable, NUL-terminated buffer.
 */
void remove_newline_from_input(char *str);

#endif /* MYSH_UTIL_H */
