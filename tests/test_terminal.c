#include "tide/terminal.h"
#include "test_support.h"

#include <termios.h>

static void test_make_raw_clears_canonical_echo_and_signals(void)
{
    struct termios input = {0};
    struct termios output = {0};

    input.c_lflag = ECHO | ICANON | IEXTEN | ISIG;
    input.c_iflag = IXON | ICRNL | BRKINT | INPCK | ISTRIP;
    input.c_oflag = OPOST;
    input.c_cflag = CS7;

    tide_terminal_make_raw(&input, &output);

    TIDE_ASSERT((output.c_lflag & ECHO) == 0);
    TIDE_ASSERT((output.c_lflag & ICANON) == 0);
    TIDE_ASSERT((output.c_lflag & ISIG) == 0);
    TIDE_ASSERT((output.c_iflag & IXON) == 0);
    TIDE_ASSERT((output.c_iflag & ICRNL) == 0);
    TIDE_ASSERT((output.c_oflag & OPOST) == 0);
    TIDE_ASSERT((output.c_cflag & CS8) == CS8);
    TIDE_ASSERT(output.c_cc[VMIN] == 0);
    TIDE_ASSERT(output.c_cc[VTIME] == 1);
}

int main(void)
{
    test_make_raw_clears_canonical_echo_and_signals();
    return 0;
}
