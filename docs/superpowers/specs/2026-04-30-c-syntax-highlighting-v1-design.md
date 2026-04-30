# C Syntax Highlighting v1 Design

## Goal

C Syntax Highlighting v1 makes `tide` visibly useful for C editing by coloring common C lexical elements in the editor surface. The feature stays deliberately lexical: it does not parse C, call `clang`, or keep a persistent syntax tree.

## User Experience

When editing a file, source text is colored by token category:

- C keywords and storage/control words use a keyword color.
- Built-in C types use a type color.
- Numeric literals use a number color.
- String and character literals use a string color.
- Line comments and block comments use a comment color.
- Preprocessor directives use a preprocessor color.
- Identifiers, punctuation, operators, and whitespace keep the default text color.

Search highlighting remains visually stronger than syntax color. If a character is part of the active search match, it keeps the existing reverse style while retaining the token foreground color.

## Architecture

A new `syntax` module owns C token classification. It exposes a small line tokenizer that accepts a line of text and emits spans with start column, length, and token kind. The tokenizer is independent of the editor, buffer, screen, and app layers so it can be tested directly.

The editor renderer asks the syntax module to classify each visible line, then assigns foreground colors as it draws cells. The status line, command prompt, blank cells, and file metadata remain unchanged.

## Tokenization Model

The v1 tokenizer scans one line at a time. It recognizes:

- `//` comments through end of line.
- `/* ... */` block comments only when the opening and closing markers are on the same line.
- Double-quoted string literals with escaped characters.
- Single-quoted character literals with escaped characters.
- Preprocessor directives when the first non-space character on the line is `#`; the directive span runs to end of line.
- Integer and floating numeric literals with simple suffix/exponent support.
- Identifiers and keywords using C identifier rules.

Keyword classification is table-driven. The first table covers control/storage/operator words such as `if`, `else`, `for`, `while`, `return`, `switch`, `case`, `break`, `continue`, `static`, `extern`, `const`, `volatile`, `sizeof`, `struct`, `union`, `enum`, `typedef`, and `goto`. The second table covers common built-in types such as `void`, `char`, `short`, `int`, `long`, `float`, `double`, `signed`, `unsigned`, `_Bool`, `_Complex`, and `_Atomic`.

## Rendering Rules

The renderer starts each text cell with default foreground/background/style, then applies syntax foreground color for the current token kind. Search match styling is applied after syntax coloring by setting `TIDE_STYLE_REVERSE`, so search remains visible without discarding the token color.

Suggested v1 colors:

- Keywords: cyan
- Types: green
- Numbers: yellow
- Strings/chars: magenta
- Comments: blue
- Preprocessor directives: red

These colors use the existing `TideColor` enum. No theme system is introduced in this slice.

## Error Handling

Tokenization is best-effort and non-failing for normal input. If a line exceeds the fixed token output capacity, tokenization stops adding spans and the remaining characters render with default color. Unterminated strings, character literals, and same-line block comments are colored through end of line.

Renderer failures still come only from existing screen operations. Syntax classification must not allocate memory during rendering.

## Tests

Tests cover the syntax module directly and the renderer integration:

- C keywords and built-in types classify separately from normal identifiers.
- Numbers, strings, characters, line comments, same-line block comments, and preprocessor directives produce the expected token kinds.
- Identifiers containing keyword substrings, such as `integer` or `return_value`, remain identifiers.
- The editor renderer assigns expected foreground colors to visible syntax tokens.
- Search highlighting still sets reverse style on top of syntax-colored text.

## Out Of Scope

C Syntax Highlighting v1 does not implement multiline block-comment state, semantic highlighting, macro expansion, include path awareness, diagnostics, tree-sitter-style parsing, LSP integration, themes, user-configurable colors, UTF-8 tokenization, or incremental token caches.
