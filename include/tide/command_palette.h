#ifndef TIDE_COMMAND_PALETTE_H
#define TIDE_COMMAND_PALETTE_H

#include <stddef.h>

typedef struct TideCommandDefinition {
    const char *name;
    const char *description;
} TideCommandDefinition;

typedef struct TideCommandMatch {
    const TideCommandDefinition *command;
    int score;
} TideCommandMatch;

size_t tide_command_palette_catalog_count(void);
const TideCommandDefinition *tide_command_palette_catalog(void);
const TideCommandDefinition *tide_command_palette_find(const char *name);
int tide_command_palette_match_score(const char *name, const char *query);
size_t tide_command_palette_filter(const char *query, TideCommandMatch *matches, size_t capacity);

#endif
