#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parse.h"

/*
#define token_stream_mark(ts) do{for(int i=0;i<ts->mark_n;i++)printf("\t");printf("mark %s\n", __func__); token_stream_mark(ts);}while(0)
#define token_stream_unmark(ts) do{for(int i=0;i<ts->mark_n;i++)printf("\t");printf("unmark %s\n", __func__); token_stream_unmark(ts);}while(0)
#define token_stream_rewind(ts) do{for(int i=0;i<ts->mark_n;i++)printf("\t");printf("rewind %s\n", __func__); token_stream_rewind(ts);}while(0)
*/

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
    token_stream_rewind(p->ts);\
    snprintf(err_buf, ERRBUF_SIZE, str, __VA_ARGS__);\
    return err_buf;\
}while(0)

#define MUST(func) do{if((err = func(p))){token_stream_rewind(p->ts); return err;}}while(0)


#define BUF_MAX 10

static char *parse_type_expr(struct parse *p);
static char *parse_expr(struct parse *p);

static char *parse_ident(struct parse *p) {
    assert(p);
    token_stream_mark(p->ts);

    struct token t;
    char *err = NULL;
    bool ignore_nl = true;
    EXPECT(TOKEN_IDENT);
    p->expr.type = EXPR_IDENT;
    p->expr.lit = t;

    token_stream_unmark(p->ts);
    return NULL;
}

static char *parse_expr_basic(struct parse *p) {
    assert(p);
    token_stream_mark(p->ts);

    char *err = NULL;
    bool ignore_nl = true;

    struct token t;
    while(t = token_stream_next(p->ts), t.type == TOKEN_NEWLINE);
    switch(t.type) {
    case TOKEN_NUM: p->expr.type = EXPR_NUM; p->expr.lit = t; break;
    case TOKEN_STR: //fallthrough
    case TOKEN_STR_ESC: p->expr.type = EXPR_STR; p->expr.lit = t; break;
    case TOKEN_IDENT: p->expr.type = EXPR_IDENT; p->expr.lit = t; break;
    case TOKEN_LPAREN: MUST(parse_expr); EXPECT(TOKEN_RPAREN); break;
    default: token_stream_rewind(p->ts); return "Not a basic expression";
    }

    token_stream_unmark(p->ts);
    return NULL;
}

static char *parse_expr_1(struct parse *p) {
    assert(p);

    struct token t;
    bool ignore_nl = true;

    token_stream_mark(p->ts);

    int level = 0;
    char *err = parse_expr_basic(p);
    while(!err) {
        level ++;
        struct expr l = p->expr;

        while(t = token_stream_next(p->ts), t.type == TOKEN_NEWLINE);
        switch(t.type) {
            case TOKEN_INC:
                p->expr.type = EXPR_POSTINC;
                p->expr.l = expr_alloc(l);
                p->expr.r = NULL;
                break;

            case TOKEN_DEC:
                p->expr.type = EXPR_POSTDEC;
                p->expr.l = expr_alloc(l);
                p->expr.r = NULL;
                break;

            case TOKEN_DOT:
                MUST(parse_ident);
                p->expr.r = expr_alloc(p->expr);

                p->expr.type = EXPR_SACC;
                p->expr.l = expr_alloc(l);
                break;

            case TOKEN_LPAREN: {
                struct expr buf[BUF_MAX];
                int i;
                for(i = 0; 1; i++) {
                   MAYBE(TOKEN_RPAREN) break;
                   if(i>0) EXPECT(TOKEN_COMMA);

                   assert(i < BUF_MAX);

                   MUST(parse_expr);
                   buf[i] = p->expr;
                }

                if(i) {
                   p->expr.args = malloc(sizeof(struct expr) * i);
                   assert(p->expr.args);
                   memcpy(p->expr.args, buf, sizeof(struct expr) * i);
                } else {
                   p->expr.args = NULL;
                }

                p->expr.f = expr_alloc(l);
                p->expr.args_n = i;
                p->expr.type = EXPR_FCALL;

                break;
            }

            default:
               if(level > 1) {
                   token_stream_rewind(p->ts);
                   return NULL;
               } else goto parse_type_expr;
        }

        token_stream_unmark(p->ts);
        token_stream_mark(p->ts);

    }

parse_type_expr:
    token_stream_rewind(p->ts);
    token_stream_mark(p->ts);
    if(parse_type_expr(p)) return parse_expr_basic(p);

    //Handle type related expressions
    struct type type = p->type;
    while(t = token_stream_next(p->ts), t.type == TOKEN_NEWLINE);
    switch(t.type) {
        case TOKEN_RARR:        // type->ident | type accessors
            MUST(parse_ident);
            p->expr.tacc.m = expr_alloc(p->expr);
            p->expr.tacc.t = type;
            p->expr.type = EXPR_TACC;
            break;

        case TOKEN_LCURL: {     // type{...}   | initializers
            struct expr buf[BUF_MAX];
            int i;
            for(i = 0; 1; i++) {
               MAYBE(TOKEN_RCURL) break;
               if(i>0) EXPECT(TOKEN_COMMA);

               assert(i < BUF_MAX);

               //TODO: parse compound literal element
               MUST(parse_expr);
               buf[i] = p->expr;
            }

            if(i) {
               p->expr.vals = malloc(sizeof(struct expr) * i);
               assert(p->expr.vals);
               memcpy(p->expr.vals, buf, sizeof(struct expr) * i);
            } else {
               p->expr.vals = NULL;
            }

            p->expr.vals_n = i;
            p->expr.t = type;
            p->expr.type = EXPR_COMP_LIT;

            break;
        }

        default:  token_stream_rewind(p->ts);
                  return parse_expr_basic(p);
    }

    token_stream_unmark(p->ts);

    return NULL;
}

static char *parse_expr(struct parse *p) {
    assert(p);
    return parse_expr_1(p);
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

    ns_set(&p->globals, ident, (struct val){VAL_MODULE, .mod_path=path});

    return NULL;
}

static char *parse_const(struct parse *p) {
    assert(p);

    char *err = NULL;
    struct token t;
    token_stream_mark(p->ts);

    bool ignore_nl = false;
    EXPECT(TOKEN_CONST);
    EXPECT(TOKEN_IDENT);

    char *ident = token_str(t);
    assert(ident);

    struct type type = {TYPE_NONE};
    if(!parse_type_expr(p)) type = p->type;

    EXPECT(TOKEN_ASSIGN);

    MUST(parse_expr);

    EXPECT(TOKEN_NEWLINE);
    token_stream_unmark(p->ts);

    ns_set(&p->globals, ident,
            (struct val){VAL_CONST, .expr=p->expr, .expr_type=type});

    return NULL;
}

static char *parse_let(struct parse *p) {
    assert(p);

    char *err = NULL;
    struct token t;
    token_stream_mark(p->ts);

    bool ignore_nl = false;
    EXPECT(TOKEN_LET);
    EXPECT(TOKEN_IDENT);

    char *ident = token_str(t);
    assert(ident);

    struct type type;
    if(parse_type_expr(p)){
        type = (struct type){TYPE_NONE};
        EXPECT(TOKEN_ASSIGN);
    } else {
        type = p->type;
        MAYBE(TOKEN_ASSIGN); else goto ret;
    }

    MUST(parse_expr);

ret:
    EXPECT(TOKEN_NEWLINE);
    token_stream_unmark(p->ts);

    ns_set(&p->globals, ident,
            (struct val){VAL_VAR, .expr=p->expr, .expr_type=type});

    return NULL;
}

//Parse struct definition in curly braces '{...}'
//Fills p->type with parsed members and TYPE_STRUCT
static char *parse_struct_members(struct parse *p) {
    char *err;
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

        MUST(parse_type_expr);

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
                MUST(parse_expr);
                vals[opts_n++] = p->expr;
            } else {
                vals[opts_n++].type = EXPR_NONE;
            }

            MAYBE(TOKEN_COMMA); else break;
        }

        MAYBE(TOKEN_RCURL)
            p->type.enum_type = NULL;
        else {
            MUST(parse_type_expr);
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
}

static char *parse_type_expr(struct parse *p) {
    char *err = NULL;

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
                MUST(parse_type_expr);
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
        case TOKEN_STRUCT: MUST(parse_struct_members); break;

        //Parse anonymous enum
        //'enum { ... }'
        case TOKEN_ENUM: MUST(parse_enum_members); break;

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

    char *err = NULL;
    MUST(parse_type_expr);

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

    char *err = NULL;
    MUST(parse_struct_members);

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

    char *err = NULL;
    MUST(parse_enum_members);

    EXPECT(TOKEN_NEWLINE);
    token_stream_unmark(p->ts);

    ts_set(&p->types, ident, p->type);
    p->type.type = TYPE_NONE;

    return NULL;
}

static char *parse_func(struct parse *p) {
    assert(p);

    char *err = NULL;

    char *args[BUF_MAX];
    struct type args_type[BUF_MAX];
    int args_n = 0;

    struct type ret_type[BUF_MAX];
    int ret_n = 0;

    char *mod = NULL, *type_ident = NULL, *ident = NULL;
    struct token t;
    token_stream_mark(p->ts);

    bool ignore_nl = false;
    EXPECT(TOKEN_FUNC);

    //Parse ident, type->ident, or mod->type->ident

    EXPECT(TOKEN_IDENT);
    ident = token_str(t);
    assert(ident);

    MAYBE(TOKEN_RARR){
        type_ident = ident;
        EXPECT(TOKEN_IDENT);
        ident = token_str(t);
        assert(ident);
    }

    MAYBE(TOKEN_RARR){
        mod = type_ident;
        type_ident = ident;
        EXPECT(TOKEN_IDENT);
        ident = token_str(t);
        assert(ident);
    }

    //Parse arguments
    EXPECT(TOKEN_LPAREN);
    for(;;) {
        MAYBE(TOKEN_RPAREN) break;

        if(args_n > 0) EXPECT(TOKEN_COMMA);

        assert(args_n < BUF_MAX);

        EXPECT(TOKEN_IDENT);
        args[args_n] = token_str(t);

        if(parse_type_expr(p))
            args_type[args_n++] = (struct type){TYPE_NONE};
        else
            args_type[args_n++] = p->type;

    }

    //Parse return value
    MAYBE(TOKEN_LPAREN) {
        for(;;) {
            MAYBE(TOKEN_RPAREN) break;

            if(ret_n > 0) EXPECT(TOKEN_COMMA);

            assert(ret_n < BUF_MAX);

            MUST(parse_type_expr);
            ret_type[ret_n++] = p->type;
        }
    } else {
        MUST(parse_type_expr);
        ret_type[ret_n++] = p->type;
    }

    MUST(parse_expr);
    struct expr expr = p->expr;

    EXPECT(TOKEN_NEWLINE);
    token_stream_unmark(p->ts);

    struct val val = {};

    val.type = VAL_FUNC;
    val.mod = mod;
    val.type_ident = type_ident;
    val.args_n = args_n;
    val.ret_n = ret_n;
    val.func_expr = expr;

    if(args_n > 0) {
        val.args = malloc(sizeof(*args) * args_n);
        assert(val.args);
        memcpy(val.args, args, sizeof(*args) * args_n);

        val.args_type = malloc(sizeof(*args_type) * args_n);
        assert(val.args_type);
        memcpy(val.args_type, args_type, sizeof(*args_type) * args_n);
    }
    if(ret_n > 0) {
        val.ret_type = malloc(sizeof(*ret_type) * ret_n);
        assert(val.ret_type);
        memcpy(val.ret_type, ret_type, sizeof(*ret_type) * ret_n);
    }

    ns_set(&p->globals, ident, val);

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
        if(err) err = parse_const(p);
        if(err) err = parse_func(p);
        if(err) err = parse_let(p);

        //TODO: improve error handling, by saving and returning the error from
        //the sub parser with the longest successful match.

        if(err){
            p->error(p->ts, token_stream_next(p->ts), err);
            while(token_stream_next(p->ts).type != TOKEN_NEWLINE);
            errnum++;
        }
    }

    return errnum;
}
