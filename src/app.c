#include "tide/app.h"

#include "tide/ansi.h"
#include "tide/buffer.h"
#include "tide/command_palette.h"
#include "tide/editor.h"
#include "tide/editor_render.h"
#include "tide/input.h"
#include "tide/project_files.h"
#include "tide/screen.h"
#include "tide/session.h"
#include "tide/terminal.h"
#include "tide/tasks.h"
#include "tide/workspace.h"

#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define TIDE_APP_PALETTE_MATCHES 4
#define TIDE_APP_OPEN_PATH_CAPACITY 1024
#define TIDE_APP_DEFAULT_SESSION_PATH ".tide-session"
#define TIDE_APP_DEFAULT_BUILD_COMMAND "cmake --build build"
#define TIDE_APP_BUILD_OUTPUT_PATH "*build-output*"
#define TIDE_APP_DIAGNOSTICS_PATH "*diagnostics*"

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

static const char *skip_command_spaces(const char *text)
{
    while (*text == ' ' || *text == '\t') {
        text++;
    }
    return text;
}

static int command_text_has_arguments(const char *command)
{
    for (size_t i = 0; command[i] != '\0'; ++i) {
        if (command[i] == ' ' || command[i] == '\t') {
            return 1;
        }
    }

    return 0;
}

static size_t prompt_match_count(const TideEditor *editor)
{
    TideCommandMatch matches[TIDE_APP_PALETTE_MATCHES];
    const char *command = tide_editor_command_text(editor);
    if (command_text_has_arguments(command)) {
        return 0;
    }

    return tide_command_palette_filter(command, matches, TIDE_APP_PALETTE_MATCHES);
}

TideStatus tide_app_resolve_prompt_command(const TideEditor *editor, char *out, size_t out_size)
{
    if (out_size == 0 || !tide_editor_command_active(editor)) {
        return TIDE_ERR_INVALID;
    }

    const char *command = tide_editor_command_text(editor);
    if (!command_text_has_arguments(command)) {
        TideCommandMatch matches[TIDE_APP_PALETTE_MATCHES];
        size_t match_count = tide_command_palette_filter(command, matches, TIDE_APP_PALETTE_MATCHES);
        size_t selection = tide_editor_command_selection(editor);
        if (match_count > 0) {
            if (selection >= match_count) {
                selection = 0;
            }
            snprintf(out, out_size, "%s", matches[selection].command->name);
            return TIDE_OK;
        }
    }

    snprintf(out, out_size, "%s", command);
    return TIDE_OK;
}

static TideStatus replace_editor_file(TideEditor *editor, const char *path, const char *success_status)
{
    if (editor->buffer->dirty) {
        tide_editor_set_status(editor, "unsaved changes; write first");
        return TIDE_OK;
    }

    TideStatus status = tide_buffer_replace_with_file(editor->buffer, path);
    if (status != TIDE_OK) {
        tide_editor_set_status(editor, tide_status_string(status));
        return TIDE_OK;
    }

    tide_editor_reset_view(editor);
    tide_editor_set_status(editor, success_status);
    return TIDE_OK;
}

static TideStatus open_editor_file(TideEditor *editor, const char *path)
{
    if (path[0] == '\0') {
        tide_editor_set_status(editor, "path required");
        return TIDE_OK;
    }

    if (editor->buffer->dirty) {
        tide_editor_set_status(editor, "unsaved changes; write first");
        return TIDE_OK;
    }

    TideStatus status = tide_buffer_replace_with_file(editor->buffer, path);
    if (status != TIDE_OK) {
        tide_editor_set_status(editor, tide_status_string(status));
        return TIDE_OK;
    }

    tide_editor_reset_view(editor);
    char message[sizeof(editor->status)];
    snprintf(message, sizeof(message), "opened: %s", path);
    tide_editor_set_status(editor, message);
    return TIDE_OK;
}

static TideStatus reload_editor_file(TideEditor *editor)
{
    if (editor->buffer->path == NULL) {
        tide_editor_set_status(editor, "no file to reload");
        return TIDE_OK;
    }

    return replace_editor_file(editor, editor->buffer->path, "reloaded");
}

static void set_current_buffer_status(TideWorkspace *workspace)
{
    TideEditor *editor = tide_workspace_current_editor(workspace);
    if (editor == NULL) {
        return;
    }

    const char *path = editor->buffer->path == NULL ? "[No Name]" : editor->buffer->path;
    char message[sizeof(editor->status)];
    snprintf(message, sizeof(message), "buffer %zu: %s", tide_workspace_current_index(workspace) + 1, path);
    tide_editor_set_status(editor, message);
}

static TideStatus list_workspace_buffers(TideWorkspace *workspace)
{
    TideEditor *editor = tide_workspace_current_editor(workspace);
    if (editor == NULL) {
        return TIDE_ERR_INVALID;
    }

    char message[sizeof(editor->status)];
    size_t used = 0;
    int written = snprintf(message, sizeof(message), "buffers:");
    if (written < 0) {
        return TIDE_ERR_IO;
    }
    used = (size_t)written >= sizeof(message) ? sizeof(message) - 1 : (size_t)written;

    for (size_t i = 0; i < workspace->count && used + 1 < sizeof(message); ++i) {
        const char *path = workspace->entries[i].buffer.path == NULL ? "[No Name]" : workspace->entries[i].buffer.path;
        written = snprintf(message + used, sizeof(message) - used, " %zu:%s", i + 1, path);
        if (written < 0) {
            return TIDE_ERR_IO;
        }
        if ((size_t)written >= sizeof(message) - used) {
            used = sizeof(message) - 1;
        } else {
            used += (size_t)written;
        }
    }

    message[sizeof(message) - 1] = '\0';
    tide_editor_set_status(editor, message);
    return TIDE_OK;
}

static TideStatus switch_workspace_buffer(TideWorkspace *workspace, const char *argument)
{
    TideEditor *editor = tide_workspace_current_editor(workspace);
    char *end = NULL;
    unsigned long requested;

    argument = skip_command_spaces(argument);
    if (editor == NULL) {
        return TIDE_ERR_INVALID;
    }
    if (argument[0] == '\0') {
        tide_editor_set_status(editor, "buffer number required");
        return TIDE_OK;
    }

    requested = strtoul(argument, &end, 10);
    end = (char *)skip_command_spaces(end);
    if (requested == 0 || end == argument || *end != '\0') {
        tide_editor_set_status(editor, "buffer number required");
        return TIDE_OK;
    }

    if (requested > tide_workspace_count(workspace)) {
        char message[sizeof(editor->status)];
        snprintf(message, sizeof(message), "no buffer: %lu", requested);
        tide_editor_set_status(editor, message);
        return TIDE_OK;
    }

    TideStatus status = tide_workspace_switch_to(workspace, (size_t)requested - 1);
    if (status == TIDE_OK) {
        set_current_buffer_status(workspace);
    }
    return status;
}

static int workspace_has_dirty_buffers(const TideWorkspace *workspace)
{
    for (size_t i = 0; i < workspace->count; ++i) {
        if (workspace->entries[i].buffer.dirty) {
            return 1;
        }
    }

    return 0;
}

static const char *session_command_path(const char *argument)
{
    argument = skip_command_spaces(argument);
    return argument[0] == '\0' ? TIDE_APP_DEFAULT_SESSION_PATH : argument;
}

static TideStatus save_workspace_session(TideWorkspace *workspace, const char *argument)
{
    TideEditor *editor = tide_workspace_current_editor(workspace);
    const char *path = session_command_path(argument);
    TideStatus status = tide_session_save_workspace(workspace, path);
    if (editor == NULL) {
        return status;
    }

    char message[sizeof(editor->status)];
    snprintf(message, sizeof(message), "session %s: %s", status == TIDE_OK ? "saved" : "error", path);
    tide_editor_set_status(editor, status == TIDE_OK ? message : tide_status_string(status));
    return TIDE_OK;
}

static TideStatus load_workspace_session(TideWorkspace *workspace, const char *argument)
{
    TideEditor *editor = tide_workspace_current_editor(workspace);
    const char *path = session_command_path(argument);

    if (editor == NULL) {
        return TIDE_ERR_INVALID;
    }

    if (workspace_has_dirty_buffers(workspace)) {
        tide_editor_set_status(editor, "unsaved changes; write first");
        return TIDE_OK;
    }

    TideStatus status = tide_session_load_workspace(workspace, path);
    editor = tide_workspace_current_editor(workspace);
    if (editor == NULL) {
        return status;
    }

    if (status != TIDE_OK) {
        tide_editor_set_status(editor, tide_status_string(status));
        return TIDE_OK;
    }

    char message[sizeof(editor->status)];
    snprintf(message, sizeof(message), "session loaded: %s", path);
    tide_editor_set_status(editor, message);
    return TIDE_OK;
}

static const char *build_command_text(const char *argument)
{
    argument = skip_command_spaces(argument);
    return argument[0] == '\0' ? TIDE_APP_DEFAULT_BUILD_COMMAND : argument;
}

static TideStatus append_build_output(TideStringBuilder *output, const char *command, const TideTaskResult *result)
{
    char exit_line[32];
    int written;
    TideStatus status = tide_string_builder_append(output, "$ ");
    if (status == TIDE_OK) {
        status = tide_string_builder_append(output, command);
    }
    if (status == TIDE_OK) {
        status = tide_string_builder_append_char(output, '\n');
    }
    if (status == TIDE_OK) {
        status = tide_string_builder_append(output, result->output == NULL ? "" : result->output);
    }
    if (status == TIDE_OK && output->length > 0 && output->data[output->length - 1] != '\n') {
        status = tide_string_builder_append_char(output, '\n');
    }
    if (status != TIDE_OK) {
        return status;
    }

    written = snprintf(exit_line, sizeof(exit_line), "[exit %d]\n", result->exit_code);
    if (written < 0 || (size_t)written >= sizeof(exit_line)) {
        return TIDE_ERR_INVALID;
    }
    return tide_string_builder_append(output, exit_line);
}

static TideStatus set_build_output_status(TideWorkspace *workspace, int exit_code)
{
    TideEditor *editor = tide_workspace_current_editor(workspace);

    if (editor == NULL) {
        return TIDE_ERR_INVALID;
    }

    if (exit_code == 0) {
        tide_editor_set_status(editor, "build passed");
        return TIDE_OK;
    }

    char message[sizeof(editor->status)];
    snprintf(message, sizeof(message), "build failed: exit %d", exit_code);
    tide_editor_set_status(editor, message);
    return TIDE_OK;
}

static const char *diagnostic_word(size_t count)
{
    return count == 1 ? "diagnostic" : "diagnostics";
}

static TideStatus append_diagnostics_output(TideStringBuilder *output, const TideDiagnostics *diagnostics)
{
    char line[TIDE_DIAGNOSTIC_PATH_CAPACITY + TIDE_DIAGNOSTIC_MESSAGE_CAPACITY + 96];

    for (size_t i = 0; i < tide_diagnostics_count(diagnostics); ++i) {
        const TideDiagnostic *diagnostic = tide_diagnostics_at(diagnostics, i);
        int written;
        if (diagnostic == NULL) {
            return TIDE_ERR_INVALID;
        }

        written = snprintf(
            line,
            sizeof(line),
            "%zu. %s %s:%zu:%zu %s\n",
            i + 1,
            tide_diagnostic_severity_label(diagnostic->severity),
            diagnostic->path,
            diagnostic->line,
            diagnostic->column,
            diagnostic->message);
        if (written < 0) {
            return TIDE_ERR_IO;
        }
        if ((size_t)written >= sizeof(line)) {
            line[sizeof(line) - 2] = '\n';
            line[sizeof(line) - 1] = '\0';
        }

        TideStatus status = tide_string_builder_append(output, line);
        if (status != TIDE_OK) {
            return status;
        }
    }

    return TIDE_OK;
}

static TideStatus open_diagnostics_buffer(TideWorkspace *workspace)
{
    TideStringBuilder output;
    TideStatus status;
    const TideDiagnostics *diagnostics = tide_workspace_diagnostics_const(workspace);

    if (tide_diagnostics_count(diagnostics) == 0) {
        TideEditor *editor = tide_workspace_current_editor(workspace);
        if (editor != NULL) {
            tide_editor_set_status(editor, "no diagnostics");
        }
        return TIDE_OK;
    }

    status = tide_string_builder_init(&output);
    if (status != TIDE_OK) {
        return status;
    }

    status = append_diagnostics_output(&output, diagnostics);
    if (status == TIDE_OK) {
        status = tide_workspace_open_text(workspace, TIDE_APP_DIAGNOSTICS_PATH, tide_string_builder_data(&output));
    }

    tide_string_builder_free(&output);
    return status;
}

static TideStatus set_build_diagnostics_status(TideWorkspace *workspace, int exit_code)
{
    TideEditor *editor = tide_workspace_current_editor(workspace);
    size_t count = tide_diagnostics_count(tide_workspace_diagnostics_const(workspace));

    if (editor == NULL) {
        return TIDE_ERR_INVALID;
    }

    char message[sizeof(editor->status)];
    snprintf(
        message,
        sizeof(message),
        "build %s: %zu %s",
        exit_code == 0 ? "passed" : "failed",
        count,
        diagnostic_word(count));
    tide_editor_set_status(editor, message);
    return TIDE_OK;
}

static int current_buffer_is_diagnostics(const TideWorkspace *workspace)
{
    const TideEditor *editor = tide_workspace_current_editor_const(workspace);
    return editor != NULL && editor->buffer->path != NULL && strcmp(editor->buffer->path, TIDE_APP_DIAGNOSTICS_PATH) == 0;
}

static TideStatus jump_to_current_diagnostic(TideWorkspace *workspace)
{
    TideDiagnostics *diagnostics = tide_workspace_diagnostics(workspace);
    const TideDiagnostic *diagnostic = tide_diagnostics_current(diagnostics);
    TideStatus status;
    TideEditor *editor;
    size_t line;
    size_t column;
    char message[128];

    if (diagnostic == NULL) {
        editor = tide_workspace_current_editor(workspace);
        if (editor != NULL) {
            tide_editor_set_status(editor, "no diagnostics");
        }
        return TIDE_OK;
    }

    status = tide_workspace_open_file(workspace, diagnostic->path);
    editor = tide_workspace_current_editor(workspace);
    if (status != TIDE_OK) {
        if (editor != NULL) {
            tide_editor_set_status(editor, tide_status_string(status));
        }
        return TIDE_OK;
    }

    editor = tide_workspace_current_editor(workspace);
    if (editor == NULL) {
        return TIDE_ERR_INVALID;
    }

    line = diagnostic->line == 0 ? 0 : diagnostic->line - 1;
    if (line >= editor->buffer->line_count && editor->buffer->line_count > 0) {
        line = editor->buffer->line_count - 1;
    }

    column = diagnostic->column == 0 ? 0 : diagnostic->column - 1;
    if (editor->buffer->line_count > 0) {
        size_t line_length = tide_buffer_line_length(editor->buffer, line);
        if (column > line_length) {
            column = line_length;
        }
    }

    editor->cursor = (TideBufferPosition){line, column};
    snprintf(
        message,
        sizeof(message),
        "diagnostic %zu/%zu: %s: %s",
        tide_diagnostics_current_index(diagnostics) + 1,
        tide_diagnostics_count(diagnostics),
        tide_diagnostic_severity_label(diagnostic->severity),
        diagnostic->message);
    tide_editor_set_status(editor, message);
    return TIDE_OK;
}

static TideStatus move_to_next_diagnostic(TideWorkspace *workspace)
{
    if (!current_buffer_is_diagnostics(workspace) &&
        tide_diagnostics_next(tide_workspace_diagnostics(workspace)) != TIDE_OK) {
        TideEditor *editor = tide_workspace_current_editor(workspace);
        if (editor != NULL) {
            tide_editor_set_status(editor, "no diagnostics");
        }
        return TIDE_OK;
    }

    return jump_to_current_diagnostic(workspace);
}

static TideStatus move_to_previous_diagnostic(TideWorkspace *workspace)
{
    if (tide_diagnostics_previous(tide_workspace_diagnostics(workspace)) != TIDE_OK) {
        TideEditor *editor = tide_workspace_current_editor(workspace);
        if (editor != NULL) {
            tide_editor_set_status(editor, "no diagnostics");
        }
        return TIDE_OK;
    }

    return jump_to_current_diagnostic(workspace);
}

static TideStatus run_workspace_build(TideWorkspace *workspace, const char *argument)
{
    const char *command = build_command_text(argument);
    TideTaskResult result;
    TideStringBuilder output;
    int output_initialized = 0;
    int exit_code;
    TideStatus status = tide_tasks_run_shell(command, &result);

    if (status != TIDE_OK) {
        TideEditor *editor = tide_workspace_current_editor(workspace);
        if (editor != NULL) {
            tide_editor_set_status(editor, tide_status_string(status));
        }
        return TIDE_OK;
    }

    exit_code = result.exit_code;
    status = tide_string_builder_init(&output);
    if (status == TIDE_OK) {
        output_initialized = 1;
    }
    if (status == TIDE_OK) {
        status = append_build_output(&output, command, &result);
    }
    if (status == TIDE_OK) {
        status = tide_workspace_parse_diagnostics(workspace, result.output == NULL ? "" : result.output);
    }
    if (status == TIDE_OK) {
        status = tide_workspace_open_text(workspace, TIDE_APP_BUILD_OUTPUT_PATH, tide_string_builder_data(&output));
    }
    if (status == TIDE_OK && tide_diagnostics_count(tide_workspace_diagnostics_const(workspace)) > 0) {
        status = open_diagnostics_buffer(workspace);
    }

    tide_task_result_free(&result);
    if (status != TIDE_OK) {
        TideEditor *editor = tide_workspace_current_editor(workspace);
        if (output_initialized) {
            tide_string_builder_free(&output);
        }
        if (editor != NULL) {
            tide_editor_set_status(editor, tide_status_string(status));
        }
        return TIDE_OK;
    }

    tide_string_builder_free(&output);
    if (tide_diagnostics_count(tide_workspace_diagnostics_const(workspace)) > 0) {
        return set_build_diagnostics_status(workspace, exit_code);
    }
    return set_build_output_status(workspace, exit_code);
}

static TideStatus resolve_project_open_path(const char *argument, char *out, size_t out_size)
{
    if (out_size == 0) {
        return TIDE_ERR_INVALID;
    }

    if (access(argument, F_OK) == 0) {
        snprintf(out, out_size, "%s", argument);
        return TIDE_OK;
    }

    TideProjectFiles files;
    TideStatus status = tide_project_files_init(&files);
    if (status != TIDE_OK) {
        return status;
    }

    status = tide_project_files_scan(&files, ".");
    if (status == TIDE_OK) {
        TideProjectFileMatch matches[1];
        if (tide_project_files_filter(&files, argument, matches, 1) > 0) {
            snprintf(out, out_size, "%s", matches[0].path);
            tide_project_files_free(&files);
            return TIDE_OK;
        }
    }

    tide_project_files_free(&files);
    snprintf(out, out_size, "%s", argument);
    return TIDE_OK;
}

TideStatus tide_app_execute_workspace_command(TideWorkspace *workspace, const char *command, int *quit)
{
    TideEditor *editor = tide_workspace_current_editor(workspace);
    *quit = 0;

    if (editor == NULL) {
        return TIDE_ERR_INVALID;
    }

    if (strncmp(command, "build", 5) == 0 && (command[5] == '\0' || command[5] == ' ' || command[5] == '\t')) {
        return run_workspace_build(workspace, command + 5);
    }

    if (strncmp(command, "open", 4) == 0 && (command[4] == '\0' || command[4] == ' ' || command[4] == '\t')) {
        const char *path = skip_command_spaces(command + 4);
        char resolved_path[TIDE_APP_OPEN_PATH_CAPACITY];
        if (path[0] == '\0') {
            tide_editor_set_status(editor, "path required");
            return TIDE_OK;
        }

        TideStatus status = resolve_project_open_path(path, resolved_path, sizeof(resolved_path));
        if (status != TIDE_OK) {
            tide_editor_set_status(editor, tide_status_string(status));
            return TIDE_OK;
        }

        status = tide_workspace_open_file(workspace, resolved_path);
        if (status != TIDE_OK) {
            tide_editor_set_status(editor, tide_status_string(status));
            return TIDE_OK;
        }

        editor = tide_workspace_current_editor(workspace);
        if (editor != NULL) {
            char message[sizeof(editor->status)];
            snprintf(message, sizeof(message), "opened: %s", resolved_path);
            tide_editor_set_status(editor, message);
        }
        return TIDE_OK;
    }

    if (strcmp(command, "buffers") == 0) {
        return list_workspace_buffers(workspace);
    }

    if (strcmp(command, "diagnostics") == 0) {
        return open_diagnostics_buffer(workspace);
    }

    if (strcmp(command, "dn") == 0 || strcmp(command, "diagnostic-next") == 0) {
        return move_to_next_diagnostic(workspace);
    }

    if (strcmp(command, "dp") == 0 || strcmp(command, "diagnostic-prev") == 0) {
        return move_to_previous_diagnostic(workspace);
    }

    if (strcmp(command, "bn") == 0 || strcmp(command, "next-buffer") == 0) {
        TideStatus status = tide_workspace_next(workspace);
        if (status == TIDE_OK) {
            set_current_buffer_status(workspace);
        }
        return status;
    }

    if (strcmp(command, "bp") == 0 || strcmp(command, "prev-buffer") == 0) {
        TideStatus status = tide_workspace_previous(workspace);
        if (status == TIDE_OK) {
            set_current_buffer_status(workspace);
        }
        return status;
    }

    if (strncmp(command, "buffer", 6) == 0 && (command[6] == '\0' || command[6] == ' ' || command[6] == '\t')) {
        return switch_workspace_buffer(workspace, command + 6);
    }

    if (strncmp(command, "session-save", 12) == 0 && (command[12] == '\0' || command[12] == ' ' || command[12] == '\t')) {
        return save_workspace_session(workspace, command + 12);
    }

    if (strncmp(command, "session-load", 12) == 0 && (command[12] == '\0' || command[12] == ' ' || command[12] == '\t')) {
        return load_workspace_session(workspace, command + 12);
    }

    return tide_app_execute_editor_command(editor, command, quit);
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

    if (strncmp(command, "find", 4) == 0 && (command[4] == '\0' || command[4] == ' ' || command[4] == '\t')) {
        const char *query = skip_command_spaces(command + 4);
        return tide_editor_find(editor, query);
    }

    if (strcmp(command, "next") == 0) {
        return tide_editor_find_next(editor);
    }

    if (strcmp(command, "prev") == 0) {
        return tide_editor_find_previous(editor);
    }

    if (strcmp(command, "undo") == 0) {
        return tide_editor_undo(editor);
    }

    if (strcmp(command, "redo") == 0) {
        return tide_editor_redo(editor);
    }

    if (strncmp(command, "open", 4) == 0 && (command[4] == '\0' || command[4] == ' ' || command[4] == '\t')) {
        const char *path = skip_command_spaces(command + 4);
        return open_editor_file(editor, path);
    }

    if (strcmp(command, "reload") == 0) {
        return reload_editor_file(editor);
    }

    char message[sizeof(editor->status)];
    snprintf(message, sizeof(message), "unknown command: %s", command);
    tide_editor_set_status(editor, message);
    return TIDE_OK;
}

static TideStatus handle_editor_event(TideEditor *editor, TideWorkspace *workspace, const TideInputEvent *event, int *quit)
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
            TideStatus status = tide_app_resolve_prompt_command(editor, command, sizeof(command));
            tide_editor_cancel_command_prompt(editor);
            if (status != TIDE_OK) {
                return status;
            }
            if (workspace != NULL) {
                return tide_app_execute_workspace_command(workspace, command, quit);
            }
            return tide_app_execute_editor_command(editor, command, quit);
        }
        case TIDE_KEY_ARROW_UP:
            tide_editor_command_move_selection(editor, TIDE_EDITOR_MOVE_UP, prompt_match_count(editor));
            return TIDE_OK;
        case TIDE_KEY_ARROW_DOWN:
            tide_editor_command_move_selection(editor, TIDE_EDITOR_MOVE_DOWN, prompt_match_count(editor));
            return TIDE_OK;
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
    TideWorkspace workspace;
    TideTerminal terminal;
    TideInputParser parser;
    struct sigaction action;
    struct sigaction old_int;
    struct sigaction old_term;
    int has_old_int = 0;
    int has_old_term = 0;
    int exit_code = 0;
    int workspace_initialized = 0;

    shutdown_requested = 0;
    tide_workspace_init(&workspace);
    workspace_initialized = 1;
    memset(&terminal, 0, sizeof(terminal));
    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);

    TideStatus status = tide_workspace_open_file(&workspace, path);
    if (status != TIDE_OK) {
        fprintf(stderr, "tide: %s: %s\n", path, tide_status_string(status));
        tide_workspace_free(&workspace);
        return 1;
    }

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

    status = render_editor_to_terminal(tide_workspace_current_editor(&workspace));
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
                status = handle_editor_event(tide_workspace_current_editor(&workspace), &workspace, &event, &should_quit);
                if (status != TIDE_OK) {
                    TideEditor *editor = tide_workspace_current_editor(&workspace);
                    if (editor != NULL) {
                        tide_editor_set_status(editor, tide_status_string(status));
                    }
                }
                if (should_quit) {
                    shutdown_requested = 1;
                }

                status = render_editor_to_terminal(tide_workspace_current_editor(&workspace));
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

            status = handle_editor_event(tide_workspace_current_editor(&workspace), &workspace, &event, &should_quit);
            if (status != TIDE_OK) {
                TideEditor *editor = tide_workspace_current_editor(&workspace);
                if (editor != NULL) {
                    tide_editor_set_status(editor, tide_status_string(status));
                }
            }
            if (should_quit) {
                shutdown_requested = 1;
                break;
            }

            status = render_editor_to_terminal(tide_workspace_current_editor(&workspace));
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
    if (workspace_initialized) {
        tide_workspace_free(&workspace);
    }

    return exit_code;
}
