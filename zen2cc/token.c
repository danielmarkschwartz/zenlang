#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

char *token_type_str[TOKEN_MAX] = {
    "TOKEN_ERR",
    "TOKEN_EOF",
    "TOKEN_IDENT",
    "TOKEN_PUNCT",
    "TOKEN_NUM",
    "TOKEN_STR",
    "TOKEN_STR_ESC",
    "TOKEN_KEY_BREAK",
    "TOKEN_KEY_CASE",
    "TOKEN_KEY_CONTINUE",
    "TOKEN_KEY_CONST",
    "TOKEN_KEY_DEFAULT",
    "TOKEN_KEY_DO",
    "TOKEN_KEY_ELSE",
    "TOKEN_KEY_ENUM",
    "TOKEN_KEY_FALLTHROUGH",
    "TOKEN_KEY_FOR",
    "TOKEN_KEY_FUNC",
    "TOKEN_KEY_IF",
    "TOKEN_KEY_RETURN",
    "TOKEN_KEY_STRUCT",
    "TOKEN_KEY_SWITCH",
    "TOKEN_KEY_TYPEDEF",
    "TOKEN_KEY_UNION",
    "TOKEN_KEY_VOLATILE"
};


#define KEYWORDS_NUM 18

//Must be in same order as enum definition above
char *keywords[KEYWORDS_NUM] = {
    "break", "case", "continue", "const", "default", "do", "else", "enum",
    "fallthrough", "for", "func", "if", "return", "struct", "switch",
    "typedef", "union", "volatile"
};

static bool is_space(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static bool is_punct(int c) {
    switch(c) {
        case '!': case '#': case '$': case '%': case '&':  case '(':
        case ')': case '*': case '+': case ',': case '-': case '.': case '/': case ':':
        case ';': case '<': case '=': case '>': case '?': case '@': case '[': case '\\':
        case ']': case '^': case '`': case '{': case '|': case '}': case '~':
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
    } else if(is_punct(c)) {
        cn = stream_getc(s);
        switch(c) {
            case '!':
                if(cn == '=') t.value = strdup("!=");
                else goto single;
                break;
            case '%':
                if(cn == '=') t.value = strdup("%=");
                else goto single;
                break;
            case '&':
                if(cn == '&') t.value = strdup("&&");
                else if(cn == '=') t.value = strdup("&=");
                else goto single;
                break;
            case '*':
                if(cn == '=') t.value = strdup("*=");
                else goto single;
                break;
            case '+':
                if(cn == '+') t.value = strdup("++");
                else if(cn == '=') t.value = strdup("+=");
                else goto single;
                break;
            case '-':
                if(cn == '-') t.value = strdup("--");
                else if(cn == '=') t.value = strdup("-=");
                else if(cn == '>') t.value = strdup("->");
                else goto single;
                break;
            case '/':
                if(cn == '=') t.value = strdup("/=");
                else if(cn == '/') {
                    skip_comment_line(s);
                    goto start;
                } else if(cn == '*') {
                    if(!skip_comment_multiline(s)) {
                        t.value = strdup("EOF while parsing comment /*");
                        return t;
                    }
                    goto start;
                } else goto single;
                break;
            case '<':
                if(cn == '=') t.value = strdup("<=");
                else if(cn == '<') {
                    cn = stream_getc(s);
                    if(cn == '=') t.value = strdup("<<=");
                    else {
                        t.value = strdup("<<");
                        stream_ungetc(s);
                    }
                } else goto single;
                break;
            case '=':
                if(cn == '=') t.value = strdup("==");
                else goto single;
                break;
            case '>':
                if(cn == '=') t.value = strdup(">=");
                else if(cn == '>') {
                    cn = stream_getc(s);
                    if(cn == '=') t.value = strdup(">>=");
                    else {
                        t.value = strdup(">>");
                        stream_ungetc(s);
                    }
                } else goto single;
                break;
            case '^':
                if(cn == '=') t.value = strdup("^=");
                else goto single;
                break;
            case '|':
                if(cn == '=') t.value = strdup("|=");
                else if(cn == '|') t.value = strdup("||");
                else goto single;
                break;
single:     default: {
                stream_ungetc(s);

                char buf[2] = " ";
                buf[0] = c;
                t.value = strdup(buf);
            }
        }
        t.type = TOKEN_PUNCT;

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
                t.type = TOKEN_KEY_BREAK + i;
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
        printf("%s [%i col %i] - \"%s\"\n", s, t.line, t.col, t.value);
    else
        printf("%s [%i col %i]\n", s, t.line, t.col);
}
