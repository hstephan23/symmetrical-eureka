#include "tide/ansi.h"

static TideStatus append_status(TideStatus first, TideStatus second)
{
    return first != TIDE_OK ? first : second;
}

static TideStatus append_cell(TideStringBuilder *out, TideCell cell)
{
    char ch = cell.ch == '\0' ? ' ' : cell.ch;
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
    TideStatus status = tide_ansi_hide_cursor(out);
    status = append_status(status, tide_string_builder_append(out, "\x1b[H"));

    for (size_t y = 0; y < screen->height && status == TIDE_OK; ++y) {
        for (size_t x = 0; x < screen->width && status == TIDE_OK; ++x) {
            TideCell cell;
            status = tide_screen_get(screen, x, y, &cell);
            if (status == TIDE_OK) {
                status = append_cell(out, cell);
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
                status = append_cell(out, cell);
            }
        }
    }

    if (status == TIDE_OK) {
        tide_screen_mark_clean(screen);
    }
    return status;
}
