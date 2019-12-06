#pragma once

#include "stream.h"

enum token_type {
    TOKEN_ERR = 0,
    TOKEN_EOF,
    TOKEN_IDENT,
    TOKEN_PUNCT,
    TOKEN_NUM,
    TOKEN_STR,
    TOKEN_STR_ESC,
    TOKEN_KEY_BREAK,
    TOKEN_KEY_CASE,
    TOKEN_KEY_CONTINUE,
    TOKEN_KEY_CONST,
    TOKEN_KEY_DEFAULT,
    TOKEN_KEY_DO,
    TOKEN_KEY_ELSE,
    TOKEN_KEY_ENUM,
    TOKEN_KEY_FALLTHROUGH,
    TOKEN_KEY_FOR,
    TOKEN_KEY_FUNC,
    TOKEN_KEY_IF,
    TOKEN_KEY_RETURN,
    TOKEN_KEY_STRUCT,
    TOKEN_KEY_SWITCH,
    TOKEN_KEY_TYPEDEF,
    TOKEN_KEY_UNION,
    TOKEN_KEY_VOLATILE,
    TOKEN_MAX
};

struct token {
    enum token_type type;
    int line, col;
    char *value;
};

struct token token_next(struct stream *s);
void token_free(struct token t);
void token_print(struct token t);
