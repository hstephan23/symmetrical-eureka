#include "tide/session.h"
#include "test_support.h"

#include <stdio.h>

static void write_text_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");
    TIDE_ASSERT(file != NULL);
    fputs(text, file);
    fclose(file);
}

static void test_session_save_and_load_roundtrip(void)
{
    TideWorkspace workspace;
    TideWorkspace loaded;
    const char *session_path = "test-session-roundtrip.tide";

    write_text_file("test-session-one.c", "one");
    write_text_file("test-session-two.c", "two");

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-session-one.c") == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-session-two.c") == TIDE_OK);
    TIDE_ASSERT(tide_workspace_switch_to(&workspace, 0) == TIDE_OK);

    TIDE_ASSERT(tide_session_save_workspace(&workspace, session_path) == TIDE_OK);

    TIDE_ASSERT(tide_workspace_init(&loaded) == TIDE_OK);
    TIDE_ASSERT(tide_session_load_workspace(&loaded, session_path) == TIDE_OK);

    TIDE_ASSERT(tide_workspace_count(&loaded) == 2);
    TIDE_ASSERT(tide_workspace_current_index(&loaded) == 0);
    TIDE_ASSERT_STR_EQ(loaded.entries[0].buffer.path, "test-session-one.c");
    TIDE_ASSERT_STR_EQ(loaded.entries[1].buffer.path, "test-session-two.c");
    TIDE_ASSERT_STR_EQ(tide_workspace_current_editor(&loaded)->buffer->lines[0].data, "one");

    tide_workspace_free(&loaded);
    tide_workspace_free(&workspace);
}

static void test_session_load_replaces_existing_workspace(void)
{
    TideWorkspace workspace;
    const char *session_path = "test-session-replace.tide";

    write_text_file("test-session-old.c", "old");
    write_text_file("test-session-new.c", "new");

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-session-new.c") == TIDE_OK);
    TIDE_ASSERT(tide_session_save_workspace(&workspace, session_path) == TIDE_OK);
    tide_workspace_free(&workspace);

    TIDE_ASSERT(tide_workspace_init(&workspace) == TIDE_OK);
    TIDE_ASSERT(tide_workspace_open_file(&workspace, "test-session-old.c") == TIDE_OK);
    TIDE_ASSERT(tide_session_load_workspace(&workspace, session_path) == TIDE_OK);

    TIDE_ASSERT(tide_workspace_count(&workspace) == 1);
    TIDE_ASSERT_STR_EQ(tide_workspace_current_editor(&workspace)->buffer->path, "test-session-new.c");
    TIDE_ASSERT_STR_EQ(tide_workspace_current_editor(&workspace)->buffer->lines[0].data, "new");

    tide_workspace_free(&workspace);
}

int main(void)
{
    test_session_save_and_load_roundtrip();
    test_session_load_replaces_existing_workspace();
    return 0;
}
