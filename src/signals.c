/*
 * signals.c -- signal setup and handlers for interactive shell behavior.
 *
 * Keeping signal policy here keeps the REPL and executor focused on control
 * flow rather than low-level terminal interrupts.
 */
