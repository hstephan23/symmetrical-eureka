#ifndef TIDE_ANSI_H
#define TIDE_ANSI_H

#include "tide/screen.h"
#include "tide/status.h"
#include "tide/string_builder.h"

TideStatus tide_ansi_render_full(TideScreen *screen, TideStringBuilder *out);
TideStatus tide_ansi_render_dirty(TideScreen *screen, TideStringBuilder *out);
TideStatus tide_ansi_show_cursor(TideStringBuilder *out);
TideStatus tide_ansi_hide_cursor(TideStringBuilder *out);

#endif
