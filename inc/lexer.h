#ifndef MYSH_LEXER_H
#define MYSH_LEXER_H

#include <stdio.h>

/* TokenType identifies the syntactic role of each token. TOKEN_WORD covers
 * ordinary command names/arguments; the other values are shell operators the
 * parser will care about later. */
typedef enum TokenType {
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_REDIR_APPEND,
    TOKEN_BACKGROUND,
} TokenType;

/* A Token owns its `text` buffer. Call tokens_free() with the whole token array
 * when the caller is done with the result from tokenize(). */
typedef struct Token
{
    TokenType type;
    char* text;
} Token;

/* Lexer is the private scan state used while tokenizing one input line.
 * It is declared here for now, but callers should treat tokenize() and
 * tokens_free() as the public API. */
typedef struct Lexer
{
    const char* line;
    size_t position;
    size_t length;
    size_t token_start;

    Token* tokens;
    size_t n_tokens;
    size_t cap;
} Lexer;

/* Tokenize `line` into a dynamically allocated Token array.
 * On success, `*n_tokens` is set to the number of tokens returned.
 * On failure, NULL is returned and `*n_tokens` remains 0. */
Token* tokenize(const char* line, size_t* n_tokens);

/* Free a token array returned by tokenize(), including each token's text. */
void tokens_free(Token* tokens, size_t n_tokens);

#endif //MYSH_LEXER_H
