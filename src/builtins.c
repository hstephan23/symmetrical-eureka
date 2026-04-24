/*
 * builtins.c -- implementations for commands that must run inside mysh.
 *
 * Commands like cd, pwd, and exit cannot all be delegated to execvp(): cd must
 * modify the shell process itself, and exit needs to end the REPL.
 */
