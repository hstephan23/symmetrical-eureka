#ifndef TIDE_SYNTAX_H
#define TIDE_SYNTAX_H

#include <stddef.h>

#define TIDE_SYNTAX_MAX_TOKENS 128

typedef enum TideSyntaxKind {
    TIDE_SYNTAX_TEXT = 0,
    TIDE_SYNTAX_KEYWORD,
    TIDE_SYNTAX_TYPE,
    TIDE_SYNTAX_NUMBER,
    TIDE_SYNTAX_STRING,
    TIDE_SYNTAX_CHAR,
    TIDE_SYNTAX_COMMENT,
    TIDE_SYNTAX_PREPROCESSOR
} TideSyntaxKind;

typedef struct TideSyntaxToken {
    size_t start;
    size_t length;
    TideSyntaxKind kind;
} TideSyntaxToken;

typedef struct TideSyntaxLine {
    TideSyntaxToken tokens[TIDE_SYNTAX_MAX_TOKENS];
    size_t count;
} TideSyntaxLine;

void tide_syntax_tokenize_c_line(const char *line, size_t length, TideSyntaxLine *out);
TideSyntaxKind tide_syntax_kind_at(const TideSyntaxLine *line, size_t column);

#endif
