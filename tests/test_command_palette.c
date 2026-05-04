#include "tide/command_palette.h"
#include "test_support.h"

static void test_empty_query_returns_catalog_order(void)
{
    TideCommandMatch matches[4];
    size_t count = tide_command_palette_filter("", matches, 4);

    TIDE_ASSERT(count == 4);
    TIDE_ASSERT_STR_EQ(matches[0].command->name, "write");
    TIDE_ASSERT_STR_EQ(matches[1].command->name, "quit");
}

static void test_fuzzy_match_scores_subsequence(void)
{
    int save_score = tide_command_palette_match_score("save", "sv");
    int quit_score = tide_command_palette_match_score("quit", "sv");

    TIDE_ASSERT(save_score > 0);
    TIDE_ASSERT(quit_score == 0);
}

static void test_filter_prefers_prefix_and_tighter_match(void)
{
    TideCommandMatch matches[4];
    size_t count = tide_command_palette_filter("re", matches, 4);

    TIDE_ASSERT(count >= 2);
    TIDE_ASSERT_STR_EQ(matches[0].command->name, "reload");
    TIDE_ASSERT_STR_EQ(matches[1].command->name, "redo");
}

static void test_filter_prefers_exact_command_over_longer_prefix(void)
{
    TideCommandMatch matches[4];
    size_t count = tide_command_palette_filter("buffer", matches, 4);

    TIDE_ASSERT(count >= 2);
    TIDE_ASSERT_STR_EQ(matches[0].command->name, "buffer");
}

static void test_exact_lookup_finds_command(void)
{
    const TideCommandDefinition *command = tide_command_palette_find("buffers");

    TIDE_ASSERT(command != NULL);
    TIDE_ASSERT_STR_EQ(command->description, "list open buffers");
    TIDE_ASSERT(tide_command_palette_find("missing") == NULL);
}

static void test_exact_lookup_finds_build_command(void)
{
    const TideCommandDefinition *command = tide_command_palette_find("build");

    TIDE_ASSERT(command != NULL);
    TIDE_ASSERT_STR_EQ(command->description, "run build command");
}

static void test_exact_lookup_finds_diagnostic_commands(void)
{
    const TideCommandDefinition *command = tide_command_palette_find("diagnostics");

    TIDE_ASSERT(command != NULL);
    TIDE_ASSERT_STR_EQ(command->description, "show build diagnostics");
    TIDE_ASSERT(tide_command_palette_find("diagnostic-next") != NULL);
    TIDE_ASSERT(tide_command_palette_find("diagnostic-prev") != NULL);
}

int main(void)
{
    test_empty_query_returns_catalog_order();
    test_fuzzy_match_scores_subsequence();
    test_filter_prefers_prefix_and_tighter_match();
    test_filter_prefers_exact_command_over_longer_prefix();
    test_exact_lookup_finds_command();
    test_exact_lookup_finds_build_command();
    test_exact_lookup_finds_diagnostic_commands();
    return 0;
}
