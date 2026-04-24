#include "shell.h"

/* Keep main tiny: initialize process-level shell state, run the REPL, clean up,
 * and return the shell's final status to the parent process. */
int main(void) {
    Shell shell = {0};
    const int status = shell_run(&shell);
    shell_destroy(&shell);
    return status;
}
