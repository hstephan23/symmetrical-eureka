/*
 * jobs.c -- background job bookkeeping.
 *
 * Job-control state belongs here rather than in main.c so signal handling and
 * command execution can share one representation later.
 */
