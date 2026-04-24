/*
 * test_lexer.c -- unit tests for lexer.c.
 *
 * These tests describe the lexer behavior the parser will need: words are
 * emitted as TOKEN_WORD values, whitespace separates words, and shell
 * operators become their own typed tokens.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"

static void assert_token(
    const Token *tokens,
    size_t index,
    TokenType expected_type,
    const char *expected_text)
{
    assert(tokens[index].type == expected_type);
    assert(strcmp(tokens[index].text, expected_text) == 0);
}

static void test_empty_input_has_no_tokens(void)
{
    /* Start with a non-zero value so the test proves tokenize() resets it. */
    size_t n_tokens = 123;
    Token *tokens = tokenize("", &n_tokens);

    assert(n_tokens == 0);
    tokens_free(tokens, n_tokens);
}

static void test_single_word(void)
{
    size_t n_tokens = 0;
    Token *tokens = tokenize("hello", &n_tokens);

    assert(tokens != NULL);
    assert(n_tokens == 1);
    assert_token(tokens, 0, TOKEN_WORD, "hello");

    tokens_free(tokens, n_tokens);
}

static void test_two_words(void)
{
    size_t n_tokens = 0;
    Token *tokens = tokenize("hello world", &n_tokens);

    assert(tokens != NULL);
    assert(n_tokens == 2);
    assert_token(tokens, 0, TOKEN_WORD, "hello");
    assert_token(tokens, 1, TOKEN_WORD, "world");

    tokens_free(tokens, n_tokens);
}

static void test_repeated_whitespace_is_ignored(void)
{
    /* Whitespace separates words but should not create empty tokens. */
    size_t n_tokens = 0;
    Token *tokens = tokenize("  hello\t\tworld  ", &n_tokens);

    assert(tokens != NULL);
    assert(n_tokens == 2);
    assert_token(tokens, 0, TOKEN_WORD, "hello");
    assert_token(tokens, 1, TOKEN_WORD, "world");

    tokens_free(tokens, n_tokens);
}

static void test_pipe_token(void)
{
    size_t n_tokens = 0;
    Token *tokens = tokenize("echo hello | wc", &n_tokens);

    assert(tokens != NULL);
    assert(n_tokens == 4);
    assert_token(tokens, 0, TOKEN_WORD, "echo");
    assert_token(tokens, 1, TOKEN_WORD, "hello");
    assert_token(tokens, 2, TOKEN_PIPE, "|");
    assert_token(tokens, 3, TOKEN_WORD, "wc");

    tokens_free(tokens, n_tokens);
}

static void test_redirection_tokens(void)
{
    /* Append redirection should be one TOKEN_REDIR_APPEND, not two > tokens. */
    size_t n_tokens = 0;
    Token *tokens = tokenize("cat < in.txt >> out.txt", &n_tokens);

    assert(tokens != NULL);
    assert(n_tokens == 5);
    assert_token(tokens, 0, TOKEN_WORD, "cat");
    assert_token(tokens, 1, TOKEN_REDIR_IN, "<");
    assert_token(tokens, 2, TOKEN_WORD, "in.txt");
    assert_token(tokens, 3, TOKEN_REDIR_APPEND, ">>");
    assert_token(tokens, 4, TOKEN_WORD, "out.txt");

    tokens_free(tokens, n_tokens);
}

int main(void)
{
    test_empty_input_has_no_tokens();
    test_single_word();
    test_two_words();
    test_repeated_whitespace_is_ignored();
    test_pipe_token();
    test_redirection_tokens();

    puts("lexer tests passed");
    return 0;
}
