#include "tide/syntax.h"
#include "test_support.h"

#include <string.h>

static TideSyntaxKind kind_at_text(const char *line, const char *needle)
{
    TideSyntaxLine syntax;
    const char *match = strstr(line, needle);
    TIDE_ASSERT(match != NULL);
    tide_syntax_tokenize_c_line(line, strlen(line), &syntax);
    return tide_syntax_kind_at(&syntax, (size_t)(match - line));
}

static void test_keywords_types_and_identifiers(void)
{
    const char *line = "int integer = return_value + return;";
    TideSyntaxLine syntax;

    tide_syntax_tokenize_c_line(line, strlen(line), &syntax);

    TIDE_ASSERT(tide_syntax_kind_at(&syntax, 0) == TIDE_SYNTAX_TYPE);
    TIDE_ASSERT(tide_syntax_kind_at(&syntax, 4) == TIDE_SYNTAX_TEXT);
    TIDE_ASSERT(tide_syntax_kind_at(&syntax, 14) == TIDE_SYNTAX_TEXT);
    TIDE_ASSERT(tide_syntax_kind_at(&syntax, 29) == TIDE_SYNTAX_KEYWORD);
}

static void test_literals_numbers_comments_and_preprocessor(void)
{
    TIDE_ASSERT(kind_at_text("#include <stdio.h>", "#") == TIDE_SYNTAX_PREPROCESSOR);
    TIDE_ASSERT(kind_at_text("int n = 42;", "42") == TIDE_SYNTAX_NUMBER);
    TIDE_ASSERT(kind_at_text("char *s = \"hi\\\"\";", "\"hi") == TIDE_SYNTAX_STRING);
    TIDE_ASSERT(kind_at_text("char c = '\\n';", "'\\n'") == TIDE_SYNTAX_CHAR);
    TIDE_ASSERT(kind_at_text("x++; // comment", "//") == TIDE_SYNTAX_COMMENT);
    TIDE_ASSERT(kind_at_text("x = /* comment */ 1;", "/*") == TIDE_SYNTAX_COMMENT);
}

int main(void)
{
    test_keywords_types_and_identifiers();
    test_literals_numbers_comments_and_preprocessor();
    return 0;
}
