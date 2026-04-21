/*
 * main.c -- entry point for mysh.
 *
 * At this stage the shell is a bare REPL:
 *   1. print a prompt
 *   2. read one line from stdin
 *   3. echo it back (or exit on the literal word "exit")
 *   4. loop
 *
 * Later milestones will replace the "echo" step with real command execution
 * (fork/exec, pipelines, redirection, etc.). Input routes through getline()
 * rather than fgets() so we can accept arbitrarily long lines without a
 * fixed-size buffer.
 */

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    /* getline() allocates and grows this buffer for us. We must free() it
     * before returning from main(). input_len tracks the current capacity;
     * getline updates it whenever it has to realloc. */
    char   *user_input = NULL;
    size_t  input_len  = 0;
    ssize_t n;

    while (1)
    {
        fputs("mysh> ", stdout);
        /* stdout is line-buffered when connected to a terminal but fully
         * buffered when redirected to a pipe. Flushing guarantees the prompt
         * actually appears before we block waiting for input. */
        fflush(stdout);

        n = getline(&user_input, &input_len, stdin);
        if (n == -1)
        {
            /* getline returns -1 in two cases: clean EOF (Ctrl-D on an empty
             * line, or the end of piped input) OR a real I/O error. feof
             * distinguishes them. */
            if (feof(stdin))
            {
                fputc('\n', stdout);   /* tidy newline so the user's outer
                                        * terminal prompt appears on its own line */
                break;
            }
            perror("getline -- ERROR with EOF");
            free(user_input);
            return 1;
        }

        /* getline keeps the trailing '\n'; strip it so string compares work. */
        remove_newline_from_input(user_input);

        if (strcmp(user_input, "exit") == 0) break;

        /* Step-01 placeholder: just echo the line back. Later milestones
         * will replace this with: tokenize -> parse -> execute. */
        printf("%s\n", user_input);
    }

    printf("mysh> Goodbye :) \n");
    free(user_input);
    return 0;
}
