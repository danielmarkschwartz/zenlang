#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parse.h"

void parse_init(struct parse *p, struct token_stream *ts, error_func err) {
    assert(p); assert(ts);

    ns_init(&p->globals);
    ts_init(&p->types);
    p->ts = ts;
    p->error = err;
}

void parse_free(struct parse *p) {
    if(!p) return;
    ns_free(&p->globals);
    ts_free(&p->types);
}

#define ERRBUF_SIZE 1024
static char err_buf[ERRBUF_SIZE];

#define EXPECT(ttype) do{\
    if(ignore_nl) while(token_stream_peek(p->ts).type == TOKEN_NEWLINE) token_stream_next(p->ts);\
    t = token_stream_next(p->ts);\
    if(t.type != ttype && !(ttype == TOKEN_NEWLINE && t.type == TOKEN_EOF)){\
        token_stream_rewind(p->ts);\
        snprintf(err_buf, ERRBUF_SIZE, "Expected token %s, got %s [%s,%i]", token_type_str[ttype], token_type_str[t.type], __FILE__, __LINE__);\
        return err_buf;\
    }\
}while(0)

#define MAYBE(ttype)\
    if(ignore_nl) while(token_stream_peek(p->ts).type == TOKEN_NEWLINE) token_stream_next(p->ts);\
    t = token_stream_peek(p->ts);\
    if(t.type == ttype) token_stream_next(p->ts);\
    if(t.type == ttype)\

#define ERRF(str, ...) do{\
    snprintf(err_buf, ERRBUF_SIZE, str, __VA_ARGS__);\
    return err_buf;\
}while(0)

static char *parse_expr(struct parse *p) {
    assert(p);
    token_stream_mark(p->ts);

    struct token t;
    while(t = token_stream_next(p->ts), t.type == TOKEN_NEWLINE);
    switch(t.type) {
    case TOKEN_NUM: p->expr.type = EXPR_NUM; p->expr.num = t; return NULL;
    //TODO: implement expression parsing
    default: assert(0); //NOT IMPLEMENTED
    }

    token_stream_unmark(p->ts);
    return NULL;
}

static char *parse_include(struct parse *p) {
    assert(p);
    struct token t;
    token_stream_mark(p->ts);

    bool ignore_nl = false;
    EXPECT(TOKEN_INCLUDE);
    EXPECT(TOKEN_STR_ESC);

    char *path = token_str(t);
    assert(path);

    char *ident = NULL;
    MAYBE(TOKEN_IDENT){
        ident = token_str(t);
    } else {
        //Essentially basename
        char *s = strrchr(path, '/');
        if(s) ident = &s[1];
        else ident = path;
    }

    EXPECT(TOKEN_NEWLINE);
    token_stream_unmark(p->ts);

    ns_set(&p->globals, ident, (struct val){VAL_MODULE, .mod={path}});

    return NULL;
}

static char *parse_type_expr(struct parse *p);

#define BUF_MAX 10

//Parse struct definition in curly braces '{...}'
//Fills p->type with parsed members and TYPE_STRUCT
static char *parse_struct_members(struct parse *p) {
    struct token t;

    token_stream_mark(p->ts);
    p->type.type = TYPE_ERR;

    char *idents[BUF_MAX];
    struct type types[BUF_MAX];
    int mem_n = 0;

    bool ignore_nl = false;
    EXPECT(TOKEN_LCURL);
    ignore_nl = true;

    for(;;){
        MAYBE(TOKEN_RCURL) break;

ident:  assert(mem_n < BUF_MAX);
        EXPECT(TOKEN_IDENT);
        idents[mem_n] = token_str(t);
        types[mem_n++] = (struct type){TYPE_NONE};

        MAYBE(TOKEN_COMMA) goto ident;

        char *err = parse_type_expr(p);
        if(err) {
            token_stream_rewind(p->ts);
            return err;
        }
        for(int i = mem_n-1; i>=0 && types[i].type == TYPE_NONE; i--)
            types[i] = p->type;

        MAYBE(TOKEN_RCURL) break;
        EXPECT(TOKEN_SEMICOLON);
    }

    token_stream_unmark(p->ts);
    p->type.type = TYPE_STRUCT;
    p->type.idents = NULL;
    p->type.types = NULL;
    p->type.mem_n = mem_n;

    if(mem_n > 0) {
        p->type.idents = malloc(sizeof(*idents) * mem_n);
        assert(p->type.idents);
        memcpy(p->type.idents, idents, sizeof(*idents) * mem_n);

        p->type.types = malloc(sizeof(*types) * mem_n);
        assert(p->type.types);
        memcpy(p->type.types, types, sizeof(*types) * mem_n);
    }

    return NULL;
}

//Parse struct definition in curly braces '{...}'
//Fills p->type with parsed members and TYPE_ENUM
static char *parse_enum_members(struct parse *p) {
    struct token t;
    char *err;

    //TODO: EXPECT should follow error handling routine rather than dump memory?

    token_stream_mark(p->ts);
    p->type.type = TYPE_ERR;

    char *opts[BUF_MAX];
    struct expr vals[BUF_MAX];
    int opts_n = 0;

    bool ignore_nl = false;
    EXPECT(TOKEN_LCURL);
    ignore_nl = true;

    MAYBE(TOKEN_RCURL); else {
        for(;;) {
            assert(opts_n < BUF_MAX);
            EXPECT(TOKEN_IDENT);
            opts[opts_n] = token_str(t);

            MAYBE(TOKEN_ASSIGN) {
                err = parse_expr(p);
                if(err) goto err;
                vals[opts_n++] = p->expr;
            } else {
                vals[opts_n++].type = EXPR_NONE;
            }

            MAYBE(TOKEN_COMMA); else break;
        }

        MAYBE(TOKEN_RCURL)
            p->type.enum_type = NULL;
        else {
            err = parse_type_expr(p);
            if(err) goto err;
            p->type.enum_type = type_alloc(p->type);
            EXPECT(TOKEN_RCURL);
        }
    }

    token_stream_unmark(p->ts);

    p->type.type = TYPE_ENUM;
    p->type.opts = NULL;
    p->type.vals = NULL;
    p->type.opts_n = opts_n;

    if(opts_n > 0) {
        p->type.opts = malloc(sizeof(*opts) * opts_n);
        assert(p->type.opts);
        memcpy(p->type.opts, opts, sizeof(*opts) * opts_n);

        p->type.vals = malloc(sizeof(*vals) * opts_n);
        assert(p->type.vals);
        memcpy(p->type.vals, vals, sizeof(*vals) * opts_n);
    }

    return NULL;

err:
    token_stream_rewind(p->ts);
    free(opts); free(vals);
    p->type.type = TYPE_ERR;
    return err;
}

static char *parse_type_expr(struct parse *p) {
    token_stream_mark(p->ts);
    struct token t = token_stream_next(p->ts);

    bool ignore_nl = false;
    p->type.type = TYPE_ERR;

    switch(t.type) {

        //Parse primitive or ident
        case TOKEN_IDENT: {
            enum type_primative pt;
            for(pt = 0; pt < TYPE_NUM; pt++)
                if( t.len == strlen(type_primative_str[pt])
                        && strncmp(type_primative_str[pt], t.str, t.len) == 0)
                    break;

            if(pt != TYPE_NUM) {
                p->type.type = TYPE_PRIMATIVE;
                p->type.primative = pt;
                break;
            }

            p->type.type = TYPE_IDENT;
            p->type.ident = token_str(t);
            p->type.mod = NULL;

            MAYBE(TOKEN_RARR) {
                EXPECT(TOKEN_IDENT);
                p->type.mod = p->type.ident;
                p->type.ident = token_str(t);
            }

            break;
        }

        case TOKEN_MUL:
            parse_type_expr(p);
            p->type.of = type_alloc(p->type);
            p->type.type = TYPE_PTR;
            break;

        case TOKEN_LBRA:
            p->type.n = -1;
            MAYBE(TOKEN_NUM) {
                char *s = token_str(t);
                p->type.n = atoi(s);
                free(s);
            }
            EXPECT(TOKEN_RBRA);

            parse_type_expr(p);
            p->type.of = type_alloc(p->type);
            p->type.type = TYPE_ARRAY;

            break;

        case TOKEN_FUNC: {
            EXPECT(TOKEN_LPAREN);

            //Parse args
            struct type args[BUF_MAX];
            int args_n = 0;

            for(;;){
                MAYBE(TOKEN_RPAREN) break;
                if(args_n > 0) EXPECT(TOKEN_COMMA);

                parse_type_expr(p);
                assert(args_n < BUF_MAX);
                args[args_n++] = p->type;
            }

            //Parse returns
            struct type ret[BUF_MAX];
            int ret_n = 0;

            MAYBE(TOKEN_LPAREN) {
                for(;;){
                    MAYBE(TOKEN_RPAREN) break;
                    if(ret_n > 0) EXPECT(TOKEN_COMMA);

                    parse_type_expr(p);
                    assert(ret_n < BUF_MAX);
                    ret[ret_n++] = p->type;
                }
            } else {
                parse_type_expr(p);
                ret[ret_n++] = p->type;
            }

            p->type.type = TYPE_FUNC;
            p->type.args = NULL;
            p->type.args_n = args_n;
            p->type.ret = NULL;
            p->type.ret_n = ret_n;

            if(args_n > 0) {
                p->type.args = malloc(sizeof(*args) * args_n);
                assert(p->type.args);
                memcpy(p->type.args, args, sizeof(*args) * args_n);
            }

            if(ret_n > 0) {
                p->type.ret = malloc(sizeof(*ret) * ret_n);
                assert(p->type.ret);
                memcpy(p->type.ret, ret, sizeof(*ret) * ret_n);
            }

            break;
        }

        //Parse anonymous struct
        //'struct { ... }'
        case TOKEN_STRUCT: {
            char *err = parse_struct_members(p);
            if(err) {
                token_stream_rewind(p->ts);
                return err;
            }
            break;
        }

        //Parse anonymous enum
        //'enum { ... }'
        case TOKEN_ENUM: {
            char *err = parse_enum_members(p);
            if(err) {
                token_stream_rewind(p->ts);
                return err;
            }
            break;
        }

        default:
            ERRF("Unexpected token %s while parsing type", token_type_str[t.type]);
    }

    token_stream_unmark(p->ts);

    return NULL;
}

static char *parse_typedef(struct parse *p) {
    struct token t;
    token_stream_mark(p->ts);

    bool ignore_nl = false;
    EXPECT(TOKEN_TYPEDEF);
    EXPECT(TOKEN_IDENT);

    char *ident = token_str(t);
    assert(ident);

    char *err = parse_type_expr(p);
    if(err) {
        token_stream_rewind(p->ts);
        return err;
    }

    EXPECT(TOKEN_NEWLINE);
    token_stream_unmark(p->ts);

    ts_set(&p->types, ident, p->type);
    p->type.type = TYPE_NONE;

    return NULL;
}

static char *parse_struct(struct parse *p) {
    struct token t;
    token_stream_mark(p->ts);

    bool ignore_nl = false;
    EXPECT(TOKEN_STRUCT);
    EXPECT(TOKEN_IDENT);

    char *ident = token_str(t);
    assert(ident);

    char *err = parse_struct_members(p);
    if(err) {
        token_stream_rewind(p->ts);
        return err;
    }

    EXPECT(TOKEN_NEWLINE);
    token_stream_unmark(p->ts);

    ts_set(&p->types, ident, p->type);
    p->type.type = TYPE_NONE;

    return NULL;
}

static char *parse_enum(struct parse *p) {
    struct token t;
    token_stream_mark(p->ts);

    bool ignore_nl = false;
    EXPECT(TOKEN_ENUM);
    EXPECT(TOKEN_IDENT);

    char *ident = token_str(t);
    assert(ident);

    char *err = parse_enum_members(p);
    if(err) {
        token_stream_rewind(p->ts);
        return err;
    }

    EXPECT(TOKEN_NEWLINE);
    token_stream_unmark(p->ts);

    ts_set(&p->types, ident, p->type);
    p->type.type = TYPE_NONE;

    return NULL;
}

//Parse entire stream, calling error for every error encountered.
//Returns number of errors
int parse(struct parse *p) {
    int errnum = 0;

    for(;;) {
        while(token_stream_peek(p->ts).type == TOKEN_NEWLINE)
            token_stream_next(p->ts);

        if(token_stream_peek(p->ts).type == TOKEN_EOF)
            break;

        char *err = parse_include(p);
        if(err) err = parse_typedef(p);
        if(err) err = parse_struct(p);
        if(err) err = parse_enum(p);

        //TODO: parse other top level constructs

        if(err){
            p->error(p->ts, token_stream_next(p->ts), err);
            while(token_stream_next(p->ts).type != TOKEN_NEWLINE);
            errnum++;
        }
    }

    return errnum;
}
