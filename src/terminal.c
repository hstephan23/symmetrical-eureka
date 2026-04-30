#include "tide/terminal.h"

void tide_terminal_make_raw(const struct termios *input, struct termios *output)
{
    *output = *input;
    output->c_iflag &= (tcflag_t) ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    output->c_oflag &= (tcflag_t) ~(OPOST);
    output->c_cflag &= (tcflag_t) ~(CSIZE);
    output->c_cflag |= CS8;
    output->c_lflag &= (tcflag_t) ~(ECHO | ICANON | IEXTEN | ISIG);
    output->c_cc[VMIN] = 0;
    output->c_cc[VTIME] = 1;
}

TideStatus tide_terminal_enable_raw(TideTerminal *terminal, int fd)
{
    struct termios raw;

    terminal->fd = fd;
    terminal->raw_enabled = 0;

    if (tcgetattr(fd, &terminal->original) == -1) {
        return TIDE_ERR_IO;
    }

    tide_terminal_make_raw(&terminal->original, &raw);

    if (tcsetattr(fd, TCSAFLUSH, &raw) == -1) {
        return TIDE_ERR_IO;
    }

    terminal->raw_enabled = 1;
    return TIDE_OK;
}

void tide_terminal_disable_raw(TideTerminal *terminal)
{
    if (terminal->raw_enabled) {
        tcsetattr(terminal->fd, TCSAFLUSH, &terminal->original);
        terminal->raw_enabled = 0;
    }
}
