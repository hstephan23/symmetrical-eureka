#include "lexer.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Append an already-built token to the lexer's dynamic token array. */
static int lexer_push_token(Lexer* lexer, TokenType type, char* text);

/* Emit the current input slice as a TOKEN_WORD, if the slice is non-empty. */
static int lexer_emit_word(Lexer* lexer);

Token* tokenize(const char* line, size_t* n_tokens)
{
    assert(line != NULL);
    assert(n_tokens != NULL);
    *n_tokens = 0;

    if (line[0] == '\0')
    {
        char **empty = malloc(sizeof(char*));
        assert(empty != NULL);
        empty[0] = NULL;
        free(empty);
        return NULL;
    }

    Lexer lexer = {
        .line = line,
        .position = 0,
        .length = strlen(line),
        .token_start = 0,
        .tokens = NULL,
        .n_tokens = 0,
        .cap = 0
    };

    /* Scan left to right. Separators/operators cause the current word to be
     * emitted before the lexer moves on to the next token boundary. */
    for (size_t i = 0; i < lexer.length; i++)
    {
        lexer.position = i;
        if (line[i] == '"')
        {

        } else if (line[i] == ' ' || line[i] == '\t')
        {
            if (lexer_emit_word(&lexer) != 0)
            {
                tokens_free(lexer.tokens, lexer.n_tokens);
                return NULL;
            }
        }
    }

    *n_tokens = lexer.n_tokens;
    return lexer.tokens;
}


void tokens_free(Token* tokens, size_t n_tokens)
{
    if (tokens == NULL) return;

    /* Each Token owns its text allocation; the array owns the Token structs. */
    for (size_t i = 0; i < n_tokens; i++)
    {
        free(tokens[i].text);
    }

    free(tokens);
}

static int lexer_push_token(Lexer* lexer, TokenType type, char* text)
{
    if (lexer->n_tokens == lexer->cap)
    {
        /* Grow geometrically so repeated token pushes stay amortized O(1). */
        size_t new_cap = lexer->cap == 0 ? 8 : lexer->cap * 2;
        Token *new_tokens = realloc(lexer->tokens, sizeof(Token) * new_cap);
        if (new_tokens == NULL)
        {
            /* Ownership of `text` is only accepted after the push succeeds. */
            free(text);
            return -1;
        }

        lexer->tokens = new_tokens;
        lexer->cap = new_cap;
    }

    lexer->tokens[lexer->n_tokens].type = type;
    lexer->tokens[lexer->n_tokens].text = text;
    lexer->n_tokens++;

    return 0;
}

static int lexer_emit_word(Lexer* lexer)
{
    size_t token_length = lexer->position - lexer->token_start;

    /* Consecutive separators do not produce empty word tokens. */
    if (token_length == 0) return 0;

    char* text = malloc(sizeof(char) * (token_length + 1));
    if (text == NULL) return -1;

    memcpy(text, lexer->line + lexer->token_start, token_length);
    text[token_length] = '\0';

    return lexer_push_token(lexer, TOKEN_WORD, text);
}
