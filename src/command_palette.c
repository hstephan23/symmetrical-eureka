#include "tide/command_palette.h"

#include <string.h>

static const TideCommandDefinition catalog[] = {
    {"write", "save current buffer"},
    {"quit", "quit editor"},
    {"wq", "save and quit"},
    {"open", "open file"},
    {"reload", "reload current file"},
    {"find", "search current buffer"},
    {"next", "next search match"},
    {"prev", "previous search match"},
    {"undo", "undo last edit"},
    {"redo", "redo last undone edit"},
    {"buffers", "list open buffers"},
    {"buffer", "switch to buffer number"},
    {"next-buffer", "switch to next buffer"},
    {"prev-buffer", "switch to previous buffer"},
    {"session-save", "save open buffers"},
    {"session-load", "load open buffers"},
    {"build", "run build command"},
};

static int ascii_lower(int ch)
{
    if (ch >= 'A' && ch <= 'Z') {
        return ch + ('a' - 'A');
    }
    return ch;
}

static int is_word_start(const char *name, size_t index)
{
    if (index == 0) {
        return 1;
    }

    return name[index - 1] == '-' || name[index - 1] == '_' || name[index - 1] == ' ';
}

size_t tide_command_palette_catalog_count(void)
{
    return sizeof(catalog) / sizeof(catalog[0]);
}

const TideCommandDefinition *tide_command_palette_catalog(void)
{
    return catalog;
}

const TideCommandDefinition *tide_command_palette_find(const char *name)
{
    for (size_t i = 0; i < tide_command_palette_catalog_count(); ++i) {
        if (strcmp(catalog[i].name, name) == 0) {
            return &catalog[i];
        }
    }

    return NULL;
}

int tide_command_palette_match_score(const char *name, const char *query)
{
    if (query == NULL || query[0] == '\0') {
        return 1;
    }

    size_t name_length = strlen(name);
    size_t query_length = strlen(query);
    size_t name_index = 0;
    size_t first_match = 0;
    size_t previous_match = 0;
    int matched_any = 0;
    int score = 1000;

    for (size_t query_index = 0; query[query_index] != '\0'; ++query_index) {
        int query_ch = ascii_lower((unsigned char)query[query_index]);
        int found = 0;

        while (name_index < name_length) {
            if (ascii_lower((unsigned char)name[name_index]) == query_ch) {
                if (!matched_any) {
                    first_match = name_index;
                    matched_any = 1;
                } else if (name_index == previous_match + 1) {
                    score += 35;
                }

                if (is_word_start(name, name_index)) {
                    score += 20;
                }

                previous_match = name_index;
                name_index++;
                found = 1;
                break;
            }
            name_index++;
        }

        if (!found) {
            return 0;
        }
    }

    if (name_length == query_length) {
        score += 100;
    }

    score -= (int)(first_match * 10);
    return score > 0 ? score : 1;
}

static void insert_match(TideCommandMatch *matches, size_t *count, size_t capacity, TideCommandMatch candidate)
{
    size_t insert_at = *count;

    while (insert_at > 0 && candidate.score > matches[insert_at - 1].score) {
        if (insert_at < capacity) {
            matches[insert_at] = matches[insert_at - 1];
        }
        insert_at--;
    }

    if (insert_at >= capacity) {
        return;
    }

    if (*count < capacity) {
        (*count)++;
    }

    for (size_t i = *count - 1; i > insert_at; --i) {
        matches[i] = matches[i - 1];
    }

    matches[insert_at] = candidate;
}

size_t tide_command_palette_filter(const char *query, TideCommandMatch *matches, size_t capacity)
{
    size_t count = 0;
    if (capacity == 0) {
        return 0;
    }

    if (query == NULL || query[0] == '\0') {
        size_t catalog_count = tide_command_palette_catalog_count();
        size_t limit = catalog_count < capacity ? catalog_count : capacity;
        for (size_t i = 0; i < limit; ++i) {
            matches[i] = (TideCommandMatch){&catalog[i], 0};
        }
        return limit;
    }

    for (size_t i = 0; i < tide_command_palette_catalog_count(); ++i) {
        int score = tide_command_palette_match_score(catalog[i].name, query);
        if (score > 0) {
            insert_match(matches, &count, capacity, (TideCommandMatch){&catalog[i], score});
        }
    }

    return count;
}
