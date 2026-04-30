#ifndef TIDE_TERMINAL_H
#define TIDE_TERMINAL_H

#include <termios.h>

#include "tide/status.h"

typedef struct TideTerminal {
    int fd;
    int raw_enabled;
    struct termios original;
} TideTerminal;

void tide_terminal_make_raw(const struct termios *input, struct termios *output);
TideStatus tide_terminal_enable_raw(TideTerminal *terminal, int fd);
void tide_terminal_disable_raw(TideTerminal *terminal);

#endif
