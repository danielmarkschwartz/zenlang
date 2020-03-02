#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
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

#define PUNCT_NUM 50

//Must be in same order as enum definition above
static char *punct[PUNCT_NUM] = {
    "!=", "!", "#", "%=", "%", "&&", "&=", "&", "(", ")", "*", "*=", "++", "+=", "+", ",", "--", "-=", "-", ".", "//", "/*", "/=", "/", ":", ":=", ";", "<<", "<=", "<<=", "<", "==", "=", ">>", ">=", ">>=", ">", "?", "@", "[", "\\", "]", "^=", "^", "{", "|=", "||", "|", "}", "~"
};


#define KEYWORDS_NUM 20

//Must be in same order as enum definition above
static char *keywords[KEYWORDS_NUM] = {
    "break", "case", "continue", "const", "default", "do", "else", "enum",
    "fallthrough", "for", "func", "if", "include", "let", "return", "struct",
    "switch", "typedef", "union", "volatile"
};

static bool is_space(int c) {
    return c == ' ' || c == '\t' ||  c == '\r';
}

static bool is_punct(int c) {
    switch(c) {
        case '!': case '#': case '%': case '&': case '(': case ')': case '*':
        case '+': case ',': case '-': case '.': case '/': case ':': case ';':
        case '<': case '=': case '>': case '?': case '@': case '[': case '\\':
        case ']': case '^': case '{': case '|': case '}': case '~':
            return true;
    }
    return false;
}

static bool is_digit(int c) {
    return c >= '0' && c <= '9';
}

static bool is_hex(int c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || ( c >= 'A' && c <= 'F');
}

static bool is_alpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_ident(int c) {
    return is_alpha(c) || is_digit(c) || c == '_';
}

static bool is_ident_initial(int c) {
    return is_alpha(c) || c == '_';
}

static bool is_numeric(int c) {
    switch(c){
        case '-': case '.': case '_': case 'x': case 'X': case 'o': case 'O':
        case 'b': case 'B': case 'p': case 'P':
        return true;
    }
    return is_hex(c);
}

static bool is_numeric_initial(int c) {
    return is_digit(c);
}

static bool is_str_initial(int c) {
    return c == '"' || c == '\'';
}

void skip_comment_line(struct stream *s) {
    int c;
    while(c = stream_getc(s), c != '\n' && c != EOF);
}

bool skip_comment_multiline(struct stream *s) {
    for(;;) {
        int c = stream_getc(s);
        if(c == EOF) return false;
        if(c != '*') continue;
        c = stream_getc(s);
        if(c == '/') return true;
    }
}


struct token token_next(struct stream *s) {
    assert(s);

    int c, cn;
    char buf[BUFSIZ];
    int N = 0;

start:
    while(c = stream_getc(s), is_space(c));
    stream_ungetc(s);

    struct token t = (struct token){
        .type = TOKEN_ERR,
        .line = s->line,
        .col = s->col
    };

    c = stream_getc(s);

    if(c == EOF) {
        t.type = TOKEN_EOF;
    } else if(c == '\n') {
        t.type = TOKEN_NEWLINE;
    } else if(is_punct(c)) {
        cn = stream_getc(s);
        switch(c) {
            case '!':
                if(cn == '=') t.type = TOKEN_NE;
                else {t.type = TOKEN_NOT; stream_ungetc(s);}
                break;
            case '#':
                t.type = TOKEN_HASH; stream_ungetc(s);
                break;
            case '%':
                if(cn == '=') t.type = TOKEN_MODASSIGN;
                else {t.type = TOKEN_MOD; stream_ungetc(s);}
                break;
            case '&':
                if(cn == '&') t.type = TOKEN_AND;
                else if(cn == '=') t.type = TOKEN_BANDASSIGN;
                else {t.type = TOKEN_BAND; stream_ungetc(s);}
                break;
            case '(':
                t.type = TOKEN_LPAREN; stream_ungetc(s);
                break;
            case ')':
                t.type = TOKEN_RPAREN; stream_ungetc(s);
                break;
            case '*':
                if(cn == '=') t.type = TOKEN_MULASSIGN;
                else {t.type = TOKEN_MUL; stream_ungetc(s);}
                break;
            case '+':
                if(cn == '+') t.type = TOKEN_INC;
                else if(cn == '=') t.type = TOKEN_ADDASSIGN;
                else {t.type = TOKEN_ADD; stream_ungetc(s);}
                break;
            case ',':
                t.type = TOKEN_COMMA; stream_ungetc(s);
                break;
            case '-':
                if(cn == '-') t.type = TOKEN_DEC;
                else if(cn == '=') t.type = TOKEN_SUBASSIGN;
                else {t.type = TOKEN_SUB; stream_ungetc(s);}
                break;
            case '.':
                t.type = TOKEN_DOT; stream_ungetc(s);
                break;
            case '/':
                if(cn == '=') t.type = TOKEN_DIVASSIGN;
                else if(cn == '/') {
                    skip_comment_line(s);
                    goto start;
                } else if(cn == '*') {
                    if(!skip_comment_multiline(s)) {
                        t.value = strdup("EOF while parsing comment /*");
                        return t;
                    }
                    goto start;
                } else {t.type = TOKEN_DIV; stream_ungetc(s);}
                break;
            case ':':
                if(cn == '=') t.type = TOKEN_DEFASSIGN;
                else {t.type = TOKEN_COLON; stream_ungetc(s);}
                break;
            case ';':
                t.type = TOKEN_SEMICOLON; stream_ungetc(s);
                break;
            case '<':
                if(cn == '=') t.type = TOKEN_LE;
                else if(cn == '<') {
                    cn = stream_getc(s);
                    if(cn == '=') t.type = TOKEN_BSLASSIGN;
                    else {
                        t.type = TOKEN_BSL;
                        stream_ungetc(s);
                    }
                } else {t.type = TOKEN_LT; stream_ungetc(s);}
                break;
            case '=':
                if(cn == '=') t.type = TOKEN_EQ;
                else {t.type = TOKEN_ASSIGN; stream_ungetc(s);}
                break;
            case '>':
                if(cn == '=') t.type = TOKEN_GE;
                else if(cn == '>') {
                    cn = stream_getc(s);
                    if(cn == '=') t.type = TOKEN_BSRASSIGN;
                    else {
                        t.type = TOKEN_BSR;
                        stream_ungetc(s);
                    }
                } else {t.type = TOKEN_GT; stream_ungetc(s);}
                break;
            case '?':
                t.type = TOKEN_QM; stream_ungetc(s);
                break;
            case '@':
                t.type = TOKEN_AT; stream_ungetc(s);
                break;
            case '[':
                t.type = TOKEN_LBRA; stream_ungetc(s);
                break;
            case '\\':
                t.type = TOKEN_BSLASH; stream_ungetc(s);
                break;
            case ']':
                t.type = TOKEN_RBRA; stream_ungetc(s);
                break;
            case '^':
                if(cn == '=') t.type = TOKEN_XORASSIGN;
                else {t.type = TOKEN_XOR; stream_ungetc(s);}
                break;
            case '{':
                t.type = TOKEN_LCURL; stream_ungetc(s);
                break;
            case '|':
                if(cn == '=') t.type = TOKEN_BORASSIGN;
                else if(cn == '|') t.type = TOKEN_OR;
                else {t.type = TOKEN_BOR; stream_ungetc(s);}
                break;
            case '}':
                t.type = TOKEN_RCURL; stream_ungetc(s);
                break;
            case '~':
                t.type = TOKEN_BNOT; stream_ungetc(s);
                break;
            default: assert(false);
        }

    }else if(is_ident_initial(c)) {
        buf[N++] = (char)c;

        while(c = stream_getc(s), is_ident(c)) {
            assert(N < BUFSIZ - 1);
            buf[N++] = (char)c;
        }

        assert(N < BUFSIZ - 1);
        buf[N++] = 0;

        t.type = TOKEN_IDENT;

        for(int i = 0; i < KEYWORDS_NUM; i++)
            if(strcmp(keywords[i], buf) == 0) {
                t.type = TOKEN_BREAK + i;
                break;
            }

        if(t.type == TOKEN_IDENT)
            t.value = strdup(buf);
        else t.value = 0;

    } else if(is_numeric_initial(c)) {
        buf[N++] = (char)c;

        while(c = stream_getc(s), is_numeric(c)) {
            assert(N < BUFSIZ - 1);
            buf[N++] = (char)c;
        }

        assert(N < BUFSIZ - 1);
        buf[N++] = 0;

        t.type = TOKEN_NUM;
        t.value = strdup(buf);

    } else if(is_str_initial(c)) {
        char q = c;
        bool esc = q == '"';
        bool after_bt = false;

        //Consume string
        for(;;) {
            c = stream_getc(s);

            if(c == EOF) {
                t.type = TOKEN_ERR;
                t.value = strdup("EOF while parsing string");
                return t;
            }

            if((!esc || !after_bt) && c == q) break;

            assert(N < BUFSIZ - 1);
            buf[N++] = (char)c;
            after_bt = !after_bt && c == '\\';
        }

        assert(N < BUFSIZ - 1);
        buf[N++] = 0;

        t.type = esc ? TOKEN_STR_ESC : TOKEN_STR;
        t.value = strdup(buf);

    }  else {
        //Otherwise is ERR
        t.value = strdup("Unrecognized character");
    }


    return t;
}

void token_free(struct token t) {
    if(t.value) free(t.value);
}

void token_print(struct token t) {

    char *s = token_type_str[t.type];

    if(t.value)
        printf("%s [%i col %i] - \"%s\"", s, t.line, t.col, t.value);
    else
        printf("%s [%i col %i]", s, t.line, t.col);

    if(t.type >= TOKEN_NE)
        printf(" %s\n", punct[t.type - TOKEN_NE]);
    else printf("\n");
}
