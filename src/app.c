#include "tide/app.h"

#include "tide/ansi.h"
#include "tide/input.h"
#include "tide/screen.h"
#include "tide/terminal.h"

#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static volatile sig_atomic_t shutdown_requested = 0;

void tide_app_request_shutdown(void)
{
    shutdown_requested = 1;
}

static void handle_signal(int signal_number)
{
    (void)signal_number;
    tide_app_request_shutdown();
}

static TideStatus draw_text(TideScreen *screen, size_t x, size_t y, const char *text, TideCell cell)
{
    if (y >= screen->height) {
        return TIDE_OK;
    }

    for (size_t i = 0; text[i] != '\0' && x + i < screen->width; ++i) {
        cell.ch = text[i];
        TideStatus status = tide_screen_set(screen, x + i, y, cell);
        if (status != TIDE_OK) {
            return status;
        }
    }

    return TIDE_OK;
}

TideStatus tide_app_render_demo(size_t width, size_t height, TideStringBuilder *out)
{
    TideScreen screen;
    TideStatus status = tide_screen_init(&screen, width, height);
    if (status != TIDE_OK) {
        return status;
    }

    TideCell normal = tide_cell_make(' ', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE);
    TideCell title = tide_cell_make(' ', TIDE_COLOR_CYAN, TIDE_COLOR_DEFAULT, TIDE_STYLE_BOLD);
    TideCell status_cell = tide_cell_make(' ', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_REVERSE);

    tide_screen_clear(&screen, normal);
    status = draw_text(&screen, 0, 0, "tide", title);
    if (status == TIDE_OK) {
        status = draw_text(&screen, 0, height - 1, "q: quit", status_cell);
    }
    if (status == TIDE_OK) {
        status = tide_ansi_render_full(&screen, out);
    }

    tide_screen_free(&screen);
    return status;
}

static void terminal_size(size_t *width, size_t *height)
{
    struct winsize size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0 && size.ws_row > 0) {
        *width = size.ws_col;
        *height = size.ws_row;
        return;
    }

    *width = 80;
    *height = 24;
}

static TideStatus write_all(int fd, const char *data, size_t length)
{
    size_t written_total = 0;
    while (written_total < length) {
        ssize_t written = write(fd, data + written_total, length - written_total);
        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            return TIDE_ERR_IO;
        }
        written_total += (size_t)written;
    }
    return TIDE_OK;
}

static TideStatus render_to_terminal(void)
{
    TideStringBuilder out;
    size_t width;
    size_t height;

    terminal_size(&width, &height);

    TideStatus status = tide_string_builder_init(&out);
    if (status != TIDE_OK) {
        return status;
    }

    status = tide_app_render_demo(width, height, &out);
    if (status == TIDE_OK) {
        status = write_all(STDOUT_FILENO, tide_string_builder_data(&out), tide_string_builder_length(&out));
    }

    tide_string_builder_free(&out);
    return status;
}

static int event_requests_quit(const TideInputEvent *event)
{
    if (event->type == TIDE_INPUT_TEXT && event->text == 'q') {
        return 1;
    }

    if (event->type == TIDE_INPUT_KEY && (event->key == TIDE_KEY_CTRL_Q || event->key == TIDE_KEY_CTRL_C)) {
        return 1;
    }

    return 0;
}

int tide_app_run(void)
{
    TideTerminal terminal;
    TideInputParser parser;
    struct sigaction action;
    struct sigaction old_int;
    struct sigaction old_term;
    int has_old_int = 0;
    int has_old_term = 0;
    int exit_code = 0;

    shutdown_requested = 0;
    memset(&terminal, 0, sizeof(terminal));
    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, &old_int) == 0) {
        has_old_int = 1;
    }
    if (sigaction(SIGTERM, &action, &old_term) == 0) {
        has_old_term = 1;
    }

    TideStatus status = tide_terminal_enable_raw(&terminal, STDIN_FILENO);
    if (status != TIDE_OK) {
        fprintf(stderr, "tide: %s\n", tide_status_string(status));
        exit_code = 1;
        goto cleanup_signals;
    }

    status = render_to_terminal();
    if (status != TIDE_OK) {
        exit_code = 1;
        goto cleanup_terminal;
    }

    tide_input_parser_init(&parser);
    while (!shutdown_requested) {
        unsigned char buffer[32];
        ssize_t nread = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (nread == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            exit_code = 1;
            break;
        }
        if (nread == 0) {
            continue;
        }

        for (ssize_t i = 0; i < nread; ++i) {
            TideInputEvent event;
            TideInputResult result = tide_input_feed(&parser, buffer[i], &event);
            if (result == TIDE_INPUT_EVENT && event_requests_quit(&event)) {
                shutdown_requested = 1;
                break;
            }
            if (result == TIDE_INPUT_INVALID) {
                tide_input_flush(&parser, &event);
            }
        }
    }

cleanup_terminal:
    {
        TideStringBuilder out;
        if (tide_string_builder_init(&out) == TIDE_OK) {
            tide_ansi_show_cursor(&out);
            tide_string_builder_append(&out, "\x1b[0m\r\n");
            write_all(STDOUT_FILENO, tide_string_builder_data(&out), tide_string_builder_length(&out));
            tide_string_builder_free(&out);
        }
    }
    tide_terminal_disable_raw(&terminal);

cleanup_signals:
    if (has_old_int) {
        sigaction(SIGINT, &old_int, NULL);
    }
    if (has_old_term) {
        sigaction(SIGTERM, &old_term, NULL);
    }

    return exit_code;
}
