#include "tide/app.h"
#include "tide/string_builder.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        puts("tide 0.1.0");
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "--render-demo") == 0) {
        TideStringBuilder out;
        if (tide_string_builder_init(&out) != TIDE_OK) {
            fputs("tide: allocation failed\n", stderr);
            return 1;
        }
        if (tide_app_render_demo(40, 8, &out) != TIDE_OK) {
            tide_string_builder_free(&out);
            fputs("tide: render failed\n", stderr);
            return 1;
        }
        fputs(tide_string_builder_data(&out), stdout);
        tide_string_builder_free(&out);
        return 0;
    }

    if (argc == 2) {
        return tide_app_run_file(argv[1]);
    }

    if (argc > 2) {
        fputs("tide: expected at most one file path\n", stderr);
        return 1;
    }

    return tide_app_run();
}
