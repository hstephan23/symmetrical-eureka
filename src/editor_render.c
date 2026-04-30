#include "tide/editor_render.h"

#include "tide/syntax.h"

#include <stdio.h>
#include <string.h>

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

static TideColor syntax_color(TideSyntaxKind kind)
{
    switch (kind) {
    case TIDE_SYNTAX_KEYWORD:
        return TIDE_COLOR_CYAN;
    case TIDE_SYNTAX_TYPE:
        return TIDE_COLOR_GREEN;
    case TIDE_SYNTAX_NUMBER:
        return TIDE_COLOR_YELLOW;
    case TIDE_SYNTAX_STRING:
    case TIDE_SYNTAX_CHAR:
        return TIDE_COLOR_MAGENTA;
    case TIDE_SYNTAX_COMMENT:
        return TIDE_COLOR_BLUE;
    case TIDE_SYNTAX_PREPROCESSOR:
        return TIDE_COLOR_RED;
    case TIDE_SYNTAX_TEXT:
        return TIDE_COLOR_DEFAULT;
    }
    return TIDE_COLOR_DEFAULT;
}

static TideStatus draw_buffer_lines(TideEditor *editor, TideScreen *screen)
{
    size_t editable_height = screen->height > 1 ? screen->height - 1 : screen->height;
    TideCell text_cell = tide_cell_make(' ', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE);
    TideBufferPosition match = tide_editor_search_match(editor);
    size_t match_length = tide_editor_search_match_length(editor);
    int has_match = tide_editor_search_has_match(editor);

    for (size_t row = 0; row < editable_height; ++row) {
        size_t line_index = editor->viewport_line + row;
        const char *line = tide_buffer_line_text(editor->buffer, line_index);
        size_t line_length = tide_buffer_line_length(editor->buffer, line_index);

        if (line_index >= editor->buffer->line_count || editor->viewport_column >= line_length) {
            continue;
        }

        TideSyntaxLine syntax;
        tide_syntax_tokenize_c_line(line, line_length, &syntax);

        for (size_t col = 0; col < screen->width && editor->viewport_column + col < line_length; ++col) {
            size_t buffer_column = editor->viewport_column + col;
            char ch = line[editor->viewport_column + col];
            text_cell.ch = ch == '\t' ? ' ' : ch;
            text_cell.fg = syntax_color(tide_syntax_kind_at(&syntax, buffer_column));
            text_cell.bg = TIDE_COLOR_DEFAULT;
            text_cell.style = TIDE_STYLE_NONE;
            if (has_match && line_index == match.line && buffer_column >= match.column && buffer_column < match.column + match_length) {
                text_cell.style = TIDE_STYLE_REVERSE;
            }
            TideStatus status = tide_screen_set(screen, col, row, text_cell);
            if (status != TIDE_OK) {
                return status;
            }
        }
    }

    return TIDE_OK;
}

static TideStatus draw_status(TideEditor *editor, TideScreen *screen)
{
    if (screen->height == 0) {
        return TIDE_OK;
    }

    TideCell status_cell = tide_cell_make(' ', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_REVERSE);
    size_t status_y = screen->height - 1;
    char status[256];
    const char *state = editor->buffer->dirty ? "dirty" : "clean";
    const char *path = editor->buffer->path != NULL ? editor->buffer->path : "[No Name]";

    snprintf(status, sizeof(status), "[%s] %s Ln %zu, Col %zu %s",
        state,
        path,
        editor->cursor.line + 1,
        editor->cursor.column + 1,
        editor->status);

    for (size_t x = 0; x < screen->width; ++x) {
        TideStatus set_status = tide_screen_set(screen, x, status_y, status_cell);
        if (set_status != TIDE_OK) {
            return set_status;
        }
    }

    if (tide_editor_command_active(editor)) {
        char prompt[sizeof(editor->command) + 2];
        snprintf(prompt, sizeof(prompt), ":%s", tide_editor_command_text(editor));
        return draw_text(screen, 0, status_y, prompt, status_cell);
    }

    return draw_text(screen, 0, status_y, status, status_cell);
}

TideStatus tide_editor_render(TideEditor *editor, TideScreen *screen)
{
    TideCell blank = tide_cell_make(' ', TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE);
    tide_screen_clear(screen, blank);
    tide_editor_ensure_cursor_visible(editor, screen->width, screen->height);

    TideStatus status = draw_buffer_lines(editor, screen);
    if (status != TIDE_OK) {
        return status;
    }

    return draw_status(editor, screen);
}
