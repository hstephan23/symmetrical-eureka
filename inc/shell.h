#ifndef MYSH_SHELL_H
#define MYSH_SHELL_H

#include <stdio.h>

/* Shell owns the process-level REPL state. Keeping it in one struct makes it easier
 * to add later state such as last exit status, jobs, and environment data. */
typedef struct Shell
{
    /* getline() owns and resizes this buffer; shell_destroy() frees it. */
    char* input;
    size_t input_length;
    int should_exit;
} Shell;

/* Run the read/evaluate loop until EOF, "exit", or a fatal shell error. */
int shell_run(Shell* shell);

/* Release allocations owned by Shell. Safe for a zero-initialized Shell. */
void shell_destroy(Shell* shell);

#endif
