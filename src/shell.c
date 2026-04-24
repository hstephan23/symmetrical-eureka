#include "shell.h"
#include "lexer.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Read one line from stdin into shell->input and strip the trailing newline. */
static int shell_read_line(Shell* shell);

/* Handle one complete input line. This will eventually call parser/executor. */
static int shell_execute_line(Shell* shell);

int shell_run(Shell* shell)
{
    while (!shell->should_exit)
    {
        if (shell_read_line(shell) != 0) return 1;

        if (shell->should_exit) break;

        if (shell_execute_line(shell) != 0) return 1;
    }

    puts("mysh> Goodbye! :)");
    return 0;
}

void shell_destroy(Shell* shell)
{
    free(shell->input);
}

static int shell_read_line(Shell* shell)
{
    fputs("mysh> ", stdout);
    /* Prompts need an explicit flush when stdout is not line-buffered. */
    fflush(stdout);

    ssize_t n = getline(&shell->input, &shell->input_length, stdin);
    if (n == -1)
    {
        if (feof(stdin))
        {
            fputc('\n', stdout);
            shell->should_exit = 1;
            return 0;
        }

        perror("getline");
        return -1;
    }
    remove_newline_from_input(shell->input);
    return 0;
}

static int shell_execute_line(Shell* shell)
{
    /* Built-in exit is handled before tokenization so it works even while the
     * lexer/parser are still under construction. */
    if (strcmp(shell->input, "exit") == 0)
    {
        shell->should_exit = 1;
        return 0;
    }

    size_t n_tokens = 0;
    Token* tokens = tokenize(shell->input, &n_tokens);
    if (tokens == NULL)
    {
        fputs("mysh: failed to tokenize input\n", stderr);
        return -1;
    }

    /* Temporary debug output until parser/executor replace this stage. */
    for (size_t i =0; i <n_tokens; i++)
    {
        printf("Token %zu: %s\n", i, tokens[i].text);
    }

    tokens_free(tokens, n_tokens);
    return 0;
}
