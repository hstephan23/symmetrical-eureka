#include "tide/app.h"

#include "tide/ansi.h"
#include "tide/buffer.h"
#include "tide/editor.h"
#include "tide/editor_render.h"
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

static TideStatus render_editor_to_builder(TideEditor *editor, size_t width, size_t height, TideStringBuilder *out)
{
    TideScreen screen;
    TideStatus status = tide_screen_init(&screen, width, height);
    if (status != TIDE_OK) {
        return status;
    }

    status = tide_editor_render(editor, &screen);
    if (status == TIDE_OK) {
        status = tide_ansi_render_full(&screen, out);
    }

    tide_screen_free(&screen);
    return status;
}

TideStatus tide_app_render_file_demo(const char *path, size_t width, size_t height, TideStringBuilder *out)
{
    TideBuffer buffer;
    TideEditor editor;
    TideStatus status = tide_buffer_load_file(&buffer, path);
    if (status != TIDE_OK) {
        return status;
    }

    tide_editor_init(&editor, &buffer);
    status = render_editor_to_builder(&editor, width, height, out);

    tide_buffer_free(&buffer);
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

static TideStatus render_editor_to_terminal(TideEditor *editor)
{
    TideStringBuilder out;
    size_t width;
    size_t height;

    terminal_size(&width, &height);

    TideStatus status = tide_string_builder_init(&out);
    if (status != TIDE_OK) {
        return status;
    }

    status = render_editor_to_builder(editor, width, height, &out);
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

static int command_matches(const char *command, const char *a, const char *b)
{
    return strcmp(command, a) == 0 || strcmp(command, b) == 0;
}

TideStatus tide_app_execute_editor_command(TideEditor *editor, const char *command, int *quit)
{
    *quit = 0;

    if (command_matches(command, "save", "write") || strcmp(command, "w") == 0) {
        TideStatus status = tide_buffer_save(editor->buffer);
        tide_editor_set_status(editor, status == TIDE_OK ? "saved" : tide_status_string(status));
        return TIDE_OK;
    }

    if (command_matches(command, "quit", "q")) {
        *quit = 1;
        return TIDE_OK;
    }

    if (strcmp(command, "wq") == 0) {
        TideStatus status = tide_buffer_save(editor->buffer);
        tide_editor_set_status(editor, status == TIDE_OK ? "saved" : tide_status_string(status));
        if (status == TIDE_OK) {
            *quit = 1;
        }
        return TIDE_OK;
    }

    char message[sizeof(editor->status)];
    snprintf(message, sizeof(message), "unknown command: %s", command);
    tide_editor_set_status(editor, message);
    return TIDE_OK;
}

static TideStatus handle_editor_event(TideEditor *editor, const TideInputEvent *event, int *quit)
{
    *quit = 0;

    if (tide_editor_command_active(editor)) {
        if (event->type == TIDE_INPUT_TEXT) {
            return tide_editor_command_insert_char(editor, (char)event->text);
        }

        if (event->type != TIDE_INPUT_KEY) {
            return TIDE_OK;
        }

        switch (event->key) {
        case TIDE_KEY_ENTER: {
            char command[TIDE_EDITOR_COMMAND_CAPACITY];
            snprintf(command, sizeof(command), "%s", tide_editor_command_text(editor));
            tide_editor_cancel_command_prompt(editor);
            return tide_app_execute_editor_command(editor, command, quit);
        }
        case TIDE_KEY_BACKSPACE:
            tide_editor_command_backspace(editor);
            return TIDE_OK;
        case TIDE_KEY_ESCAPE:
        case TIDE_KEY_CTRL_P:
            tide_editor_cancel_command_prompt(editor);
            return TIDE_OK;
        default:
            return TIDE_OK;
        }
    }

    if (event->type == TIDE_INPUT_TEXT) {
        TideStatus status = tide_editor_insert_char(editor, (char)event->text);
        if (status == TIDE_OK) {
            tide_editor_set_status(editor, "");
        }
        return status;
    }

    if (event->type != TIDE_INPUT_KEY) {
        return TIDE_OK;
    }

    switch (event->key) {
    case TIDE_KEY_ENTER:
        tide_editor_set_status(editor, "");
        return tide_editor_insert_newline(editor);
    case TIDE_KEY_BACKSPACE:
        tide_editor_set_status(editor, "");
        return tide_editor_backspace(editor);
    case TIDE_KEY_ARROW_UP:
        tide_editor_move(editor, TIDE_EDITOR_MOVE_UP);
        return TIDE_OK;
    case TIDE_KEY_ARROW_DOWN:
        tide_editor_move(editor, TIDE_EDITOR_MOVE_DOWN);
        return TIDE_OK;
    case TIDE_KEY_ARROW_LEFT:
        tide_editor_move(editor, TIDE_EDITOR_MOVE_LEFT);
        return TIDE_OK;
    case TIDE_KEY_ARROW_RIGHT:
        tide_editor_move(editor, TIDE_EDITOR_MOVE_RIGHT);
        return TIDE_OK;
    case TIDE_KEY_CTRL_S: {
        TideStatus status = tide_buffer_save(editor->buffer);
        tide_editor_set_status(editor, status == TIDE_OK ? "saved" : tide_status_string(status));
        return TIDE_OK;
    }
    case TIDE_KEY_CTRL_P:
        tide_editor_open_command_prompt(editor);
        return TIDE_OK;
    case TIDE_KEY_CTRL_Q:
    case TIDE_KEY_CTRL_C:
        *quit = 1;
        return TIDE_OK;
    default:
        return TIDE_OK;
    }
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

int tide_app_run_file(const char *path)
{
    TideBuffer buffer;
    TideEditor editor;
    TideTerminal terminal;
    TideInputParser parser;
    struct sigaction action;
    struct sigaction old_int;
    struct sigaction old_term;
    int has_old_int = 0;
    int has_old_term = 0;
    int exit_code = 0;
    int buffer_loaded = 0;

    shutdown_requested = 0;
    memset(&terminal, 0, sizeof(terminal));
    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);

    TideStatus status = tide_buffer_load_file(&buffer, path);
    if (status != TIDE_OK) {
        fprintf(stderr, "tide: %s: %s\n", path, tide_status_string(status));
        return 1;
    }
    buffer_loaded = 1;
    tide_editor_init(&editor, &buffer);

    if (sigaction(SIGINT, &action, &old_int) == 0) {
        has_old_int = 1;
    }
    if (sigaction(SIGTERM, &action, &old_term) == 0) {
        has_old_term = 1;
    }

    status = tide_terminal_enable_raw(&terminal, STDIN_FILENO);
    if (status != TIDE_OK) {
        fprintf(stderr, "tide: %s\n", tide_status_string(status));
        exit_code = 1;
        goto cleanup_signals;
    }

    status = render_editor_to_terminal(&editor);
    if (status != TIDE_OK) {
        exit_code = 1;
        goto cleanup_terminal;
    }

    tide_input_parser_init(&parser);
    while (!shutdown_requested) {
        unsigned char input[32];
        ssize_t nread = read(STDIN_FILENO, input, sizeof(input));
        if (nread == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            exit_code = 1;
            break;
        }
        if (nread == 0) {
            TideInputEvent event;
            int should_quit = 0;
            TideInputResult result = tide_input_flush(&parser, &event);
            if (result == TIDE_INPUT_EVENT) {
                status = handle_editor_event(&editor, &event, &should_quit);
                if (status != TIDE_OK) {
                    tide_editor_set_status(&editor, tide_status_string(status));
                }
                if (should_quit) {
                    shutdown_requested = 1;
                }

                status = render_editor_to_terminal(&editor);
                if (status != TIDE_OK) {
                    exit_code = 1;
                    shutdown_requested = 1;
                }
            }
            continue;
        }

        for (ssize_t i = 0; i < nread; ++i) {
            TideInputEvent event;
            int should_quit = 0;
            TideInputResult result = tide_input_feed(&parser, input[i], &event);
            if (result == TIDE_INPUT_INVALID) {
                result = tide_input_flush(&parser, &event);
            }
            if (result != TIDE_INPUT_EVENT) {
                continue;
            }

            status = handle_editor_event(&editor, &event, &should_quit);
            if (status != TIDE_OK) {
                tide_editor_set_status(&editor, tide_status_string(status));
            }
            if (should_quit) {
                shutdown_requested = 1;
                break;
            }

            status = render_editor_to_terminal(&editor);
            if (status != TIDE_OK) {
                exit_code = 1;
                shutdown_requested = 1;
                break;
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
    if (buffer_loaded) {
        tide_buffer_free(&buffer);
    }

    return exit_code;
}
