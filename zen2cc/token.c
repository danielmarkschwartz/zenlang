#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "token.h"

char *token_type_str[TOKEN_MAX] = {
    "TOKEN_ERR",
    "TOKEN_EOF",
    "TOKEN_NEWLINE",

    "TOKEN_IDENT",

    "TOKEN_NUM",
    "TOKEN_STR",
    "TOKEN_STR_ESC",

    "TOKEN_BREAK",
    "TOKEN_CASE",
    "TOKEN_CONTINUE",
    "TOKEN_CONST",
    "TOKEN_DEFAULT",
    "TOKEN_DO",
    "TOKEN_ELSE",
    "TOKEN_ENUM",
    "TOKEN_FALLTHROUGH",
    "TOKEN_FOR",
    "TOKEN_FUNC",
    "TOKEN_IF",
    "TOKEN_INCLUDE",
    "TOKEN_LET",
    "TOKEN_RETURN",
    "TOKEN_STRUCT",
    "TOKEN_SWITCH",
    "TOKEN_TYPEDEF",
    "TOKEN_UNION",
    "TOKEN_VOLATILE",

    //Punctuation
    "TOKEN_NE",
    "TOKEN_NOT",
    "TOKEN_HASH",
    "TOKEN_MODASSIGN",
    "TOKEN_MOD",
    "TOKEN_AND",
    "TOKEN_BANDASSIGN",
    "TOKEN_BAND",
    "TOKEN_LPAREN",
    "TOKEN_RPAREN",
    "TOKEN_MULASSIGN",
    "TOKEN_MUL",
    "TOKEN_INC",
    "TOKEN_ADDASSIGN",
    "TOKEN_ADD",
    "TOKEN_COMMA",
    "TOKEN_DEC",
    "TOKEN_SUBASSIGN",
    "TOKEN_RARR",
    "TOKEN_SUB",
    "TOKEN_DOT",
    "TOKEN_COMMENT_LINE",
    "TOKEN_COMMENT_MLINE",
    "TOKEN_DIVASSIGN",
    "TOKEN_DIV",
    "TOKEN_COLON",
    "TOKEN_DEFASSIGN",
    "TOKEN_SEMICOLON",
    "TOKEN_BSL",
    "TOKEN_LE",
    "TOKEN_BSLASSIGN",
    "TOKEN_LT",
    "TOKEN_EQ",
    "TOKEN_ASSIGN",
    "TOKEN_BSR",
    "TOKEN_GE",
    "TOKEN_BSRASSIGN",
    "TOKEN_GT",
    "TOKEN_QM",
    "TOKEN_AT",
    "TOKEN_LBRA",
    "TOKEN_BSLASH",
    "TOKEN_RBRA",
    "TOKEN_XORASSIGN",
    "TOKEN_XOR",
    "TOKEN_LCURL",
    "TOKEN_BORASSIGN",
    "TOKEN_OR",
    "TOKEN_BOR",
    "TOKEN_RCURL",
    "TOKEN_BNOT",
};

#define PUNCT_NUM 51

//Must be in same order as enum definition above
static char *punct[PUNCT_NUM] = {
    "!=", "!", "#", "%=", "%", "&&", "&=", "&", "(", ")", "*", "*=", "++", "+=", "+", ",", "--", "-=", "->", "-", ".", "//", "/*", "/=", "/", ":", ":=", ";", "<<", "<=", "<<=", "<", "==", "=", ">>", ">=", ">>=", ">", "?", "@", "[", "\\", "]", "^=", "^", "{", "|=", "||", "|", "}", "~",
};


#define KEYWORDS_NUM 20

//Must be in same order as enum definition above
static char *keywords[KEYWORDS_NUM] = {
    "break", "case", "continue", "const", "default", "do", "else", "enum",
    "fallthrough", "for", "func", "if", "include", "let", "return", "struct",
    "switch", "typedef", "union", "volatile"
};

static bool is_space(char c) {
    return c == ' ' || c == '\t' ||  c == '\r';
}

static bool is_punct(char c) {
    switch(c) {
        case '!': case '#': case '%': case '&': case '(': case ')': case '*':
        case '+': case ',': case '-': case '.': case '/': case ':': case ';':
        case '<': case '=': case '>': case '?': case '@': case '[': case '\\':
        case ']': case '^': case '{': case '|': case '}': case '~':
            return true;
    }
    return false;
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_hex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || ( c >= 'A' && c <= 'F');
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_ident(char c) {
    return is_alpha((int)c) || is_digit((int)c) || c == '_';
}

static bool is_ident_initial(char c) {
    return is_alpha((int)c) || c == '_';
}

static bool is_numeric(char c) {
    switch(c){
        case '-': case '.': case '_': case 'x': case 'X': case 'o': case 'O':
        case 'b': case 'B': case 'p': case 'P':
        return true;
    }
    return is_hex(c);
}

static bool is_numeric_initial(char c) {
    return is_digit((int)c);
}

static bool is_str_initial(char c) {
    return c == '"' || c == '\'';
}

#define CHAR_NEXT(s) *((*s)++)

void skip_comment_line(char **s, char *end) {
    char c;
    while(c = CHAR_NEXT(s), c != '\n' && *s <= end);
}

bool skip_comment_multiline(char **s, char *end) {
    for(;;) {
        int c = CHAR_NEXT(s);
        if(*s >= end) return false;
        if(c != '*') continue;
        c = CHAR_NEXT(s);
        if(c == '/') return true;
    }
}


struct token token_next(char **s, char *end) {
    assert(s);

start:
    while(is_space(**s)) (*s)++;

    struct token t = (struct token){
        .type = TOKEN_ERR,
        .str = *s,
        .len = 0
    };

    if(*s >= end) {
        t.type = TOKEN_EOF;
    } else if(**s == '\n') {
        t.type = TOKEN_NEWLINE;
        (*s)++;
    } else if(is_punct(**s)) {
        switch(**s) {
            case '!':
                if((*s)[1] == '=') t.type = TOKEN_NE, (*s)++;
                else t.type = TOKEN_NOT;
                break;
            case '#':
                t.type = TOKEN_HASH;
                break;
            case '%':
                if((*s)[1] == '=') t.type = TOKEN_MODASSIGN, (*s)++;
                else t.type = TOKEN_MOD;
                break;
            case '&':
                if((*s)[1] == '&') t.type = TOKEN_AND, (*s)++;
                else if((*s)[1] == '=') t.type = TOKEN_BANDASSIGN, (*s)++;
                else t.type = TOKEN_BAND;
                break;
            case '(':
                t.type = TOKEN_LPAREN;
                break;
            case ')':
                t.type = TOKEN_RPAREN;
                break;
            case '*':
                if((*s)[1] == '=') t.type = TOKEN_MULASSIGN, (*s)++;
                else t.type = TOKEN_MUL;
                break;
            case '+':
                if((*s)[1] == '+') t.type = TOKEN_INC, (*s)++;
                else if((*s)[1] == '=') t.type = TOKEN_ADDASSIGN, (*s)++;
                else t.type = TOKEN_ADD;
                break;
            case ',':
                t.type = TOKEN_COMMA;
                break;
            case '-':
                if((*s)[1] == '-') t.type = TOKEN_DEC, (*s)++;
                else if((*s)[1] == '=') t.type = TOKEN_SUBASSIGN, (*s)++;
                else if((*s)[1] == '>') t.type = TOKEN_RARR, (*s)++;
                else t.type = TOKEN_SUB;
                break;
            case '.':
                t.type = TOKEN_DOT;
                break;
            case '/':
                if((*s)[1] == '=') t.type = TOKEN_DIVASSIGN, (*s)++;
                else if((*s)[1] == '/') {
                    skip_comment_line(s, end);
                    goto start;
                } else if((*s)[1] == '*') {
                    if(!skip_comment_multiline(s, end)) {
                        t.str = strdup("EOF while parsing comment /*");
                        return t;
                    }
                    goto start;
                } else t.type = TOKEN_DIV;
                break;
            case ':':
                if((*s)[1] == '=') t.type = TOKEN_DEFASSIGN, (*s)++;
                else t.type = TOKEN_COLON;
                break;
            case ';':
                t.type = TOKEN_SEMICOLON;
                break;
            case '<':
                if((*s)[1] == '=') t.type = TOKEN_LE, (*s)++;
                else if((*s)[1] == '<') {
                    if((*s)[2] == '=') t.type = TOKEN_BSLASSIGN, (*s)+=2;
                    else t.type = TOKEN_BSL, (*s)++;
                } else t.type = TOKEN_LT;
                break;
            case '=':
                if((*s)[1] == '=') t.type = TOKEN_EQ, (*s)++;
                else t.type = TOKEN_ASSIGN;
                break;
            case '>':
                if((*s)[1] == '=') t.type = TOKEN_GE, (*s)++;
                else if((*s)[1] == '>') {
                    if((*s)[2] == '=') t.type = TOKEN_BSRASSIGN, (*s)+=2;
                    else t.type = TOKEN_BSR, (*s)++;
                } else t.type = TOKEN_GT;
                break;
            case '?': t.type = TOKEN_QM; break;
            case '@': t.type = TOKEN_AT; break;
            case '[': t.type = TOKEN_LBRA; break;
            case '\\': t.type = TOKEN_BSLASH; break;
            case ']': t.type = TOKEN_RBRA; break;
            case '^':
                if((*s)[1] == '=') t.type = TOKEN_XORASSIGN, (*s)++;
                else t.type = TOKEN_XOR;
                break;
            case '{': t.type = TOKEN_LCURL; break;
            case '|':
                if((*s)[1] == '=') t.type = TOKEN_BORASSIGN, (*s)++;
                else if((*s)[1] == '|') t.type = TOKEN_OR, (*s)++;
                else t.type = TOKEN_BOR;
                break;
            case '}': t.type = TOKEN_RCURL; break;
            case '~': t.type = TOKEN_BNOT; break;
            default: assert(false);
        }

        (*s)++;

    }else if(is_ident_initial(**s)) {
        t.type = TOKEN_IDENT;
        t.str = (*s)++;
        t.len = 1;

        while(is_ident(**s)) (*s)++, t.len++;

        for(int i = 0; i < KEYWORDS_NUM; i++)
            if(t.len == strlen(keywords[i]) && strncmp(keywords[i], t.str, t.len) == 0) {
                t.type = TOKEN_BREAK + i;
                t.len = 0;
                break;
            }

    } else if(is_numeric_initial(**s)) {
        t.type = TOKEN_NUM;
        t.str = (*s)++;
        t.len = 1;

        while(is_numeric(**s)) (*s)++, t.len++;

    } else if(is_str_initial(**s)) {
        char q = *((*s)++);
        bool esc = q == '"';
        bool after_bt = false;

        t.type = esc ? TOKEN_STR_ESC : TOKEN_STR;
        t.str = *s;
        t.len = 0;

        //Consume string
        for(;;) {
            if(*s >= end) {
                t.type = TOKEN_ERR;
                t.str = strdup("EOF while parsing string");
                return t;
            }

            if((!esc || !after_bt) && **s == q) break;
            after_bt = !after_bt && **s == '\\';

            t.len++;
            (*s)++;
        }

        (*s)++;


    }  else {
        //Otherwise is ERR
        t.str = strdup("Unrecognized character");
    }


    return t;
}

void token_pos(struct token_stream *ts, struct token t, int *row, int *col) {
    assert(t.str);
    assert(ts);
    assert(ts->text);
    assert(t.str <= ts->text + ts->len);

    int r=1,c=1;
    char *s = ts->text;
    while(s < t.str) {
       if(*s == '\n') r++,c=1;
       else c++;
       s++;
    }

    *row = r, *col = c;
}

//Allocates a malloc'd string with a copy of this token as a string.
char *token_str(struct token t) {
    char *str = malloc(t.len + 1);
    assert(str);
    memcpy(str, t.str, t.len);
    str[t.len] = '\0';
    return str;
}

void token_print(struct token_stream *ts, struct token t) {

    char *s = token_type_str[t.type];

    int line = 0, col = 0;
    token_pos(ts, t, &line, &col);

    if(t.type == TOKEN_STR || t.type == TOKEN_STR_ESC) col--;

    if(t.len)
        printf("%s [%i col %i] - \"%.*s\"", s, line, col, t.len, t.str);
    else
        printf("%s [%i col %i]", s, line, col);

    if(t.type >= TOKEN_NE)
        printf(" %s\n", punct[t.type - TOKEN_NE]);
    else printf("\n");
}

bool token_stream_init(struct token_stream *ts, char *path) {
    assert(ts);
    assert(path);

    int fd = open(path, O_RDONLY);
    if (fd == -1) return false;

    struct stat sb;
    if (fstat(fd, &sb) == -1) goto err;

    ts->text = mmap(NULL, sb.st_size+1, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ts->text == MAP_FAILED) goto err;

    ts->len = sb.st_size;
    ts->path = strdup(path);
    ts->offset = 0;
    ts->buf_c = 0;
    ts->buf_i = 0;
    ts->mark_n = 0;

    close(fd);
    return true;

err:
    close(fd);
    return false;
}

void token_stream_close(struct token_stream *ts) {
    if(ts->text) munmap(ts->text, ts->len);
    if(ts->path) free(ts->path);
    ts->text = 0;
    ts->offset = 0;
    ts->len = 0;
    ts->path = 0;
    ts->buf_c = 0;
    ts->buf_i = 0;
    ts->mark_n = 0;
}

//Fill token stream buffer as much as possible. Will keep any tokens
//in buffer that are required for a mark
void token_stream_fill(struct token_stream *ts) {
    assert(ts);

    //TODO: only clear out unmarked tokens
    assert(ts->mark_n == 0);

    ts->buf_c -= ts->buf_i;
    ts->buf_i = 0;

    char *s = ts->text + ts->offset;
    while(ts->buf_c < 2*TOKEN_BUF_SIZE)  {
        struct token t = token_next(&s, ts->text + ts->len);
        ts->buf[ts->buf_c++] = t;
        if(t.type == TOKEN_EOF) break;
    }

    ts->offset = s - ts->text;
    assert(ts->offset <= ts->len);
}

//Get next token from buffer. Will return EOF token continually once end
//of stream is reached.
struct token token_stream_peek(struct token_stream *ts) {
    assert(ts);

    if(ts->buf_i > ts->buf_c || ts->buf_c == 0) token_stream_fill(ts);
    assert(ts->buf_i <= ts->buf_c);

    return ts->buf[ts->buf_i];
}

//View next token in buffer without consuming
struct token token_stream_next(struct token_stream *ts) {
    assert(ts);
    if(ts->buf_i >= ts->buf_c) token_stream_fill(ts);
    assert(ts->buf_i < ts->buf_c);

    struct token t;
    if(ts->buf[ts->buf_i].type == TOKEN_EOF)
         t = ts->buf[ts->buf_i];
    else t = ts->buf[ts->buf_i++];

    return t;
}

//Save this location in the token stream as a rewind point.
//The next call to token_stream_rewind() will reset the stream state to what it is
//now, undoing and token_stream_next() calls. Multiple calls to token_stream_mark()
//are allowed, up to TOKEN_STREAM_MARK_MAX, each of which pops the state on a stack
//which is unwound one step by each call to *_rewind().
void token_stream_mark(struct token_stream *ts) {
    assert(ts);
    assert(ts->mark_n < TOKEN_MARK_MAX);

    if(ts->buf_c <= 0) token_stream_fill(ts);
    assert(ts->buf_c > 0);

    ts->mark[ts->mark_n++] = ts->buf_i;
}

//Rewinds the stream back to the last call of token_stream_mark()
void token_stream_rewind(struct token_stream *ts) {
    assert(ts);
    assert(ts->mark_n > 0);
    ts->buf_i = ts->mark[--ts->mark_n];
}

//Remove most recent mark without changing the stream status. Marks should be removed
//this way when they are no longer needed
void token_stream_unmark(struct token_stream *ts) {
    assert(ts);
    assert(ts->mark_n > 0);
    ts->mark_n--;
}
