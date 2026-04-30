#include "tide/syntax.h"

#include <ctype.h>
#include <string.h>

static int is_identifier_start(char ch)
{
    unsigned char value = (unsigned char)ch;
    return isalpha(value) || ch == '_';
}

static int is_identifier_part(char ch)
{
    unsigned char value = (unsigned char)ch;
    return isalnum(value) || ch == '_';
}

static void add_token(TideSyntaxLine *out, size_t start, size_t length, TideSyntaxKind kind)
{
    if (length == 0 || kind == TIDE_SYNTAX_TEXT || out->count == TIDE_SYNTAX_MAX_TOKENS) {
        return;
    }

    out->tokens[out->count++] = (TideSyntaxToken){start, length, kind};
}

static int word_matches(const char *line, size_t start, size_t length, const char *word)
{
    return strlen(word) == length && memcmp(line + start, word, length) == 0;
}

static int is_keyword(const char *line, size_t start, size_t length)
{
    static const char *keywords[] = {
        "auto", "break", "case", "const", "continue", "default", "do", "else",
        "enum", "extern", "for", "goto", "if", "register", "restrict", "return",
        "sizeof", "static", "struct", "switch", "typedef", "union", "volatile", "while"
    };

    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
        if (word_matches(line, start, length, keywords[i])) {
            return 1;
        }
    }
    return 0;
}

static int is_type_keyword(const char *line, size_t start, size_t length)
{
    static const char *types[] = {
        "_Atomic", "_Bool", "_Complex", "char", "double", "float", "int",
        "long", "short", "signed", "unsigned", "void"
    };

    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
        if (word_matches(line, start, length, types[i])) {
            return 1;
        }
    }
    return 0;
}

static size_t scan_quoted(const char *line, size_t length, size_t start, char quote)
{
    size_t i = start + 1;
    while (i < length) {
        if (line[i] == '\\' && i + 1 < length) {
            i += 2;
            continue;
        }
        if (line[i] == quote) {
            return i + 1;
        }
        i++;
    }
    return length;
}

static size_t scan_block_comment(const char *line, size_t length, size_t start)
{
    size_t i = start + 2;
    while (i + 1 < length) {
        if (line[i] == '*' && line[i + 1] == '/') {
            return i + 2;
        }
        i++;
    }
    return length;
}

static size_t scan_number(const char *line, size_t length, size_t start)
{
    size_t i = start;

    if (i + 1 < length && line[i] == '0' && (line[i + 1] == 'x' || line[i + 1] == 'X')) {
        i += 2;
        while (i < length && isxdigit((unsigned char)line[i])) {
            i++;
        }
    } else {
        while (i < length && isdigit((unsigned char)line[i])) {
            i++;
        }
        if (i < length && line[i] == '.') {
            i++;
            while (i < length && isdigit((unsigned char)line[i])) {
                i++;
            }
        }
    }

    if (i < length && (line[i] == 'e' || line[i] == 'E' || line[i] == 'p' || line[i] == 'P')) {
        size_t exponent = i + 1;
        if (exponent < length && (line[exponent] == '+' || line[exponent] == '-')) {
            exponent++;
        }
        if (exponent < length && isdigit((unsigned char)line[exponent])) {
            i = exponent + 1;
            while (i < length && isdigit((unsigned char)line[i])) {
                i++;
            }
        }
    }

    while (i < length && isalpha((unsigned char)line[i])) {
        i++;
    }

    return i;
}

void tide_syntax_tokenize_c_line(const char *line, size_t length, TideSyntaxLine *out)
{
    out->count = 0;

    size_t first = 0;
    while (first < length && (line[first] == ' ' || line[first] == '\t')) {
        first++;
    }
    if (first < length && line[first] == '#') {
        add_token(out, first, length - first, TIDE_SYNTAX_PREPROCESSOR);
        return;
    }

    size_t i = 0;
    while (i < length) {
        if (line[i] == '/' && i + 1 < length && line[i + 1] == '/') {
            add_token(out, i, length - i, TIDE_SYNTAX_COMMENT);
            return;
        }

        if (line[i] == '/' && i + 1 < length && line[i + 1] == '*') {
            size_t end = scan_block_comment(line, length, i);
            add_token(out, i, end - i, TIDE_SYNTAX_COMMENT);
            i = end;
            continue;
        }

        if (line[i] == '"') {
            size_t end = scan_quoted(line, length, i, '"');
            add_token(out, i, end - i, TIDE_SYNTAX_STRING);
            i = end;
            continue;
        }

        if (line[i] == '\'') {
            size_t end = scan_quoted(line, length, i, '\'');
            add_token(out, i, end - i, TIDE_SYNTAX_CHAR);
            i = end;
            continue;
        }

        if (isdigit((unsigned char)line[i])) {
            size_t end = scan_number(line, length, i);
            add_token(out, i, end - i, TIDE_SYNTAX_NUMBER);
            i = end;
            continue;
        }

        if (is_identifier_start(line[i])) {
            size_t start = i;
            i++;
            while (i < length && is_identifier_part(line[i])) {
                i++;
            }

            if (is_type_keyword(line, start, i - start)) {
                add_token(out, start, i - start, TIDE_SYNTAX_TYPE);
            } else if (is_keyword(line, start, i - start)) {
                add_token(out, start, i - start, TIDE_SYNTAX_KEYWORD);
            }
            continue;
        }

        i++;
    }
}

TideSyntaxKind tide_syntax_kind_at(const TideSyntaxLine *line, size_t column)
{
    for (size_t i = 0; i < line->count; ++i) {
        TideSyntaxToken token = line->tokens[i];
        if (column >= token.start && column < token.start + token.length) {
            return token.kind;
        }
    }
    return TIDE_SYNTAX_TEXT;
}
