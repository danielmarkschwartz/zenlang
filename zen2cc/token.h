#pragma once
#include <stdint.h>
#include <stdbool.h>

enum token_type {
    TOKEN_ERR = 0,
    TOKEN_EOF,
    TOKEN_NEWLINE,

    //Identifier (must have value)
    TOKEN_IDENT,

    //Literals (must have value)
    TOKEN_NUM,
    TOKEN_STR,
    TOKEN_STR_ESC,

    //Keywords
    TOKEN_BREAK,
    TOKEN_CASE,
    TOKEN_CONTINUE,
    TOKEN_CONST,
    TOKEN_DEFAULT,
    TOKEN_DO,
    TOKEN_ELSE,
    TOKEN_ENUM,
    TOKEN_FALLTHROUGH,
    TOKEN_FOR,
    TOKEN_FUNC,
    TOKEN_IF,
    TOKEN_INCLUDE,
    TOKEN_LET,
    TOKEN_RETURN,
    TOKEN_STRUCT,
    TOKEN_SWITCH,
    TOKEN_TYPEDEF,
    TOKEN_UNION,
    TOKEN_VOLATILE,

    //Punctuation
    TOKEN_NE,
    TOKEN_NOT,
    TOKEN_HASH,
    TOKEN_MODASSIGN,
    TOKEN_MOD,
    TOKEN_AND,
    TOKEN_BANDASSIGN,
    TOKEN_BAND,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_MULASSIGN,
    TOKEN_MUL,
    TOKEN_INC,
    TOKEN_ADDASSIGN,
    TOKEN_ADD,
    TOKEN_COMMA,
    TOKEN_DEC,
    TOKEN_SUBASSIGN,
    TOKEN_RARR,
    TOKEN_SUB,
    TOKEN_DOT,
    TOKEN_COMMENT_LINE,
    TOKEN_COMMENT_MLINE,
    TOKEN_DIVASSIGN,
    TOKEN_DIV,
    TOKEN_COLON,
    TOKEN_DEFASSIGN,
    TOKEN_SEMICOLON,
    TOKEN_BSL,
    TOKEN_LE,
    TOKEN_BSLASSIGN,
    TOKEN_LT,
    TOKEN_EQ,
    TOKEN_ASSIGN,
    TOKEN_BSR,
    TOKEN_GE,
    TOKEN_BSRASSIGN,
    TOKEN_GT,
    TOKEN_QM,
    TOKEN_AT,
    TOKEN_LBRA,
    TOKEN_BSLASH,
    TOKEN_RBRA,
    TOKEN_XORASSIGN,
    TOKEN_XOR,
    TOKEN_LCURL,
    TOKEN_BORASSIGN,
    TOKEN_OR,
    TOKEN_BOR,
    TOKEN_RCURL,
    TOKEN_BNOT,

    TOKEN_MAX
};

struct token {
    enum token_type type;
    char * str;
    uint32_t len;
};

extern char *token_type_str[TOKEN_MAX];

struct token token_next(char **s, char *end);

#define TOKEN_BUF_SIZE (1024)
#define TOKEN_MARK_MAX 20

struct token_stream {
    char *text;         //mmap'd text buffer
    int offset;     //current offset in to text buffer
    int len;            //length of text buffer

    char *path;         //file path of text buffer

    struct token buf[TOKEN_BUF_SIZE];
    int buf_i, buf_c;

    int mark[TOKEN_MARK_MAX];
    int mark_n;
};

bool token_stream_init(struct token_stream *ts, char *path);
struct token token_stream_peek(struct token_stream *ts);
struct token token_stream_next(struct token_stream *ts);
void token_stream_mark(struct token_stream *ts);
void token_stream_rewind(struct token_stream *ts);
void token_stream_unmark(struct token_stream *ts);

void token_pos(struct token_stream *ts, struct token t, int *row, int *col);
char *token_str(struct token t);
void token_print(struct token_stream *ts, struct token t);
