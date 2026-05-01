#include "tide/ansi.h"

#include <stdio.h>

typedef struct AnsiState {
    TideColor fg;
    TideColor bg;
    unsigned style;
} AnsiState;

static TideStatus append_status(TideStatus first, TideStatus second)
{
    return first != TIDE_OK ? first : second;
}

static int color_code(TideColor color, int background)
{
    if (color < TIDE_COLOR_BLACK || color > TIDE_COLOR_WHITE) {
        return 0;
    }
    return (background ? 40 : 30) + (int)color;
}

static int attributes_equal(AnsiState state, TideCell cell)
{
    return state.fg == cell.fg && state.bg == cell.bg && state.style == cell.style;
}

static int state_is_default(AnsiState state)
{
    return state.fg == TIDE_COLOR_DEFAULT && state.bg == TIDE_COLOR_DEFAULT && state.style == TIDE_STYLE_NONE;
}

static TideStatus append_sgr(TideStringBuilder *out, TideCell cell, AnsiState *state)
{
    if (attributes_equal(*state, cell)) {
        return TIDE_OK;
    }

    char sequence[64];
    size_t offset = 0;
    int written = snprintf(sequence, sizeof(sequence), "\x1b[0");
    if (written < 0 || (size_t)written >= sizeof(sequence)) {
        return TIDE_ERR_INVALID;
    }
    offset = (size_t)written;

    if ((cell.style & TIDE_STYLE_BOLD) != 0) {
        written = snprintf(sequence + offset, sizeof(sequence) - offset, ";1");
        if (written < 0 || (size_t)written >= sizeof(sequence) - offset) {
            return TIDE_ERR_INVALID;
        }
        offset += (size_t)written;
    }

    if ((cell.style & TIDE_STYLE_REVERSE) != 0) {
        written = snprintf(sequence + offset, sizeof(sequence) - offset, ";7");
        if (written < 0 || (size_t)written >= sizeof(sequence) - offset) {
            return TIDE_ERR_INVALID;
        }
        offset += (size_t)written;
    }

    int fg = color_code(cell.fg, 0);
    if (fg != 0) {
        written = snprintf(sequence + offset, sizeof(sequence) - offset, ";%d", fg);
        if (written < 0 || (size_t)written >= sizeof(sequence) - offset) {
            return TIDE_ERR_INVALID;
        }
        offset += (size_t)written;
    }

    int bg = color_code(cell.bg, 1);
    if (bg != 0) {
        written = snprintf(sequence + offset, sizeof(sequence) - offset, ";%d", bg);
        if (written < 0 || (size_t)written >= sizeof(sequence) - offset) {
            return TIDE_ERR_INVALID;
        }
        offset += (size_t)written;
    }

    written = snprintf(sequence + offset, sizeof(sequence) - offset, "m");
    if (written < 0 || (size_t)written >= sizeof(sequence) - offset) {
        return TIDE_ERR_INVALID;
    }

    TideStatus status = tide_string_builder_append(out, sequence);
    if (status == TIDE_OK) {
        state->fg = cell.fg;
        state->bg = cell.bg;
        state->style = cell.style;
    }
    return status;
}

static TideStatus append_cell(TideStringBuilder *out, TideCell cell, AnsiState *state)
{
    char ch = cell.ch == '\0' ? ' ' : cell.ch;
    TideStatus status = append_sgr(out, cell, state);
    if (status != TIDE_OK) {
        return status;
    }
    return tide_string_builder_append_char(out, ch);
}

TideStatus tide_ansi_show_cursor(TideStringBuilder *out)
{
    return tide_string_builder_append(out, "\x1b[?25h");
}

TideStatus tide_ansi_hide_cursor(TideStringBuilder *out)
{
    return tide_string_builder_append(out, "\x1b[?25l");
}

TideStatus tide_ansi_render_full(TideScreen *screen, TideStringBuilder *out)
{
    AnsiState state = {TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE};
    TideStatus status = tide_ansi_hide_cursor(out);
    status = append_status(status, tide_string_builder_append(out, "\x1b[H"));

    for (size_t y = 0; y < screen->height && status == TIDE_OK; ++y) {
        for (size_t x = 0; x < screen->width && status == TIDE_OK; ++x) {
            TideCell cell;
            status = tide_screen_get(screen, x, y, &cell);
            if (status == TIDE_OK) {
                status = append_cell(out, cell, &state);
            }
        }

        if (status == TIDE_OK && y + 1 < screen->height) {
            status = tide_string_builder_append_char(out, '\n');
        }
    }

    status = append_status(status, tide_string_builder_append(out, "\x1b[0m"));
    if (status == TIDE_OK) {
        tide_screen_mark_clean(screen);
    }
    return status;
}

TideStatus tide_ansi_render_dirty(TideScreen *screen, TideStringBuilder *out)
{
    AnsiState state = {TIDE_COLOR_DEFAULT, TIDE_COLOR_DEFAULT, TIDE_STYLE_NONE};
    TideStatus status = TIDE_OK;

    for (size_t y = 0; y < screen->height && status == TIDE_OK; ++y) {
        for (size_t x = 0; x < screen->width && status == TIDE_OK; ++x) {
            TideCell cell;
            status = tide_screen_get(screen, x, y, &cell);
            if (status != TIDE_OK) {
                break;
            }

            if (!cell.dirty) {
                continue;
            }

            status = tide_string_builder_append_format(out, "\x1b[%zu;%zuH", y + 1, x + 1);
            if (status == TIDE_OK) {
                status = append_cell(out, cell, &state);
            }
        }
    }

    if (status == TIDE_OK && !state_is_default(state)) {
        status = tide_string_builder_append(out, "\x1b[0m");
    }

    if (status == TIDE_OK) {
        tide_screen_mark_clean(screen);
    }
    return status;
}
