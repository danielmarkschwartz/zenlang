#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parse.h"

char *type_primative_str[TYPE_NUM] = {
    "void",
    "int", "int8", "int16", "int32", "int64",
    "uint", "uint8", "uint16", "uint32", "uint64",
    "float", "float16", "float32", "float64",
};

void ts_init(struct ts *ts) {
    assert(ts);
    ts->key = malloc(TS_INITIAL_CAP * sizeof *ts->key);
    ts->val = malloc(TS_INITIAL_CAP * sizeof *ts->val);
    ts->c = TS_INITIAL_CAP;
    ts->n = 0;
}

struct type *type_alloc(struct type t) {
    struct type *ret = malloc(sizeof *ret);
    assert(ret);
    *ret = t;
    return ret;
}

void type_free(struct type *t, int level) {
    switch(t->type) {
    case TYPE_PRIMATIVE: break;
    case TYPE_IDENT: free(t->ident); free(t->mod); break;
    case TYPE_PTR: case TYPE_ARRAY:
        type_free(t->of, level+1);
        if(level) free(t);
        break;
    case TYPE_FUNC:
        for(int i = 0; i < t->args_n; i++) type_free(t->args[i], level+1);
        for(int i = 0; i < t->ret_n; i++) type_free(t->ret[i], level+1);
        break;
    case TYPE_STRUCT:
        for(int i = 0; i < t->mem_n; i++) {
            type_free(t->types[i],level+1); free(t->idents[i]);
        }
        free(t->types); free(t->idents);
        break;
    case TYPE_ENUM:
        for(int i = 0; i < t->opts_n; i++) {
             free(t->opts[i]);
        }
        free(t->vals);
        free(t->opts);
        break;
    default: assert(0);
    }
}

void type_print(struct type *t) {

loop:
    switch(t->type){
        case TYPE_PRIMATIVE:
            assert(t->primative >= 0 && t->primative < TYPE_NUM);
            printf("PRIMITIVE %s", type_primative_str[t->primative]);
            break;
        case TYPE_IDENT:
            if(t->mod)
                printf("IDENT '%s'->'%s'", t->mod, t->ident);
            else printf("IDENT '%s'", t->ident);
            break;
        case TYPE_PTR:
            printf("PTR to "); t = t->of; goto loop;
        case TYPE_ARRAY:
            if(t->n >= 0) printf("ARRAY [%i] of ", t->n);
            else printf("ARRAY of ");
            t = t->of; goto loop;
        case TYPE_FUNC:
            printf("FUNC (");
            for(int i = 0; i<t->args_n; i++){
                if(i>0) printf(", ");
                type_print(t->args[i]);
            }

            if(t->ret_n > 1) printf(") (");
            else printf(") ");

            for(int i = 0; i<t->ret_n; i++){
                if(i>0) printf(", ");
                type_print(t->ret[i]);
            }

            if(t->ret_n > 1) printf(")");

            break;
        case TYPE_STRUCT:
            printf("STRUCT {\n");
            for(int i = 0; i < t->mem_n; i++) {
                printf("\t%s ", t->idents[i]);
                type_print(t->types[i]);
                printf("\n");
            }
            printf("}");
            break;
        case TYPE_ENUM:
            printf("ENUM {\n");
            for(int i = 0; i < t->opts_n; i++) {
                if(t->vals[i].type == EXPR_NONE) printf("\t%s\n", t->opts[i]);
                else printf("\t%s = %.*s\n", t->opts[i], t->vals[i].num.len, t->vals[i].num.str);
            }
            if(t->enum_type && t->enum_type->type != TYPE_NONE) {
                printf("\tTYPE: ");
                type_print(t->enum_type);
                printf("\n");
            }
            printf("}");
            break;
        case TYPE_ERR: case TYPE_NONE:
        default: assert(0);
    }
}

void ts_free(struct ts *ts) {
    if(ts == NULL) return;
    for(int i = 0; i < ts->n; i++) {
        free(ts->key[i]);
        type_free(&ts->val[i], 0);
    }
    free(ts->key);
    free(ts->val);
    ts->c = 0;
    ts->n = 0;
}

static int ts_find(struct ts *ts, char *key) {
    assert(ts); assert(key);

    for(int i = 0; i < ts->n; i++)
        if(strcmp(ts->key[i], key) == 0)
            return i;

    return -1;
}

void ts_set(struct ts *ts, char *key, struct type val) {
    assert(ts); assert(key);

    int i = ts_find(ts, key);
    if(i < 0) {
        if(ts->c >= ts->n) {
            int new_c = ts->c * 2;
            if(new_c < TS_INITIAL_CAP) new_c = TS_INITIAL_CAP;

            ts->key = reallocf(ts->key, new_c * sizeof *ts->key);
            ts->val = reallocf(ts->val, new_c * sizeof *ts->val);
            assert(ts->key); assert(ts->val);

            ts->c = new_c;
        }

        i = ts->n++;
    }

    ts->key[i] = strdup(key);
    ts->val[i] = val;
}

struct type *ts_get(struct ts *ts, char *key) {
    assert(ts); assert(key);

    int i = ts_find(ts, key);
    if(i < 0) return NULL;
    return &ts->val[i];
}


void ns_init(struct ns *ns) {
    assert(ns);
    ns->key = malloc(NS_INITIAL_CAP * sizeof *ns->key);
    ns->val = malloc(NS_INITIAL_CAP * sizeof *ns->val);
    ns->c = NS_INITIAL_CAP;
    ns->n = 0;
}

void ns_free(struct ns *ns) {
    if(ns == NULL) return;
    for(int i = 0; i < ns->n; i++) {
        free(ns->key[i]);
        switch(ns->val[i].type) {
        case VAL_MODULE: free(ns->val[i].mod.path); break;
        default: assert(0);
        }
    }
    free(ns->key);
    free(ns->val);
    ns->c = 0;
    ns->n = 0;
}

static int ns_find(struct ns *ns, char *key) {
    assert(ns); assert(key);

    for(int i = 0; i < ns->n; i++)
        if(strcmp(ns->key[i], key) == 0)
            return i;

    return -1;
}

void ns_set(struct ns *ns, char *key, struct val val) {
    assert(ns); assert(key);

    int i = ns_find(ns, key);
    if(i < 0) {
        if(ns->c >= ns->n) {
            int new_c = ns->c * 2;
            if(new_c < NS_INITIAL_CAP) new_c = NS_INITIAL_CAP;

            ns->key = reallocf(ns->key, new_c * sizeof *ns->key);
            ns->val = reallocf(ns->val, new_c * sizeof *ns->val);
            assert(ns->key); assert(ns->val);

            ns->c = new_c;
        }

        i = ns->n++;
    }

    ns->key[i] = strdup(key);
    ns->val[i] = val;
}

struct val *ns_get(struct ns *ns, char *key) {
    assert(ns); assert(key);

    int i = ns_find(ns, key);
    if(i < 0) return NULL;
    return &ns->val[i];
}

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

    char **idents = malloc(sizeof *idents * BUF_MAX);
    struct type **types = malloc(sizeof *types * BUF_MAX);
    assert(idents); assert(types);
    int mem_n = 0;

    bool ignore_nl = false;
    EXPECT(TOKEN_LCURL);
    ignore_nl = true;

    for(;;){
        MAYBE(TOKEN_RCURL) break;

ident:  assert(mem_n < BUF_MAX);
        EXPECT(TOKEN_IDENT);
        idents[mem_n] = token_str(t);
        types[mem_n++] = type_alloc((struct type){TYPE_NONE});

        MAYBE(TOKEN_COMMA) goto ident;

        char *err = parse_type_expr(p);
        if(err) {
            token_stream_rewind(p->ts);
            return err;
        }
        for(int i = mem_n-1; i>=0 && types[i]->type == TYPE_NONE; i--)
            *types[i] = p->type;

        MAYBE(TOKEN_RCURL) break;
        EXPECT(TOKEN_SEMICOLON);
    }

    token_stream_unmark(p->ts);
    p->type.type = TYPE_STRUCT;
    p->type.idents = idents;
    p->type.types = types;
    p->type.mem_n = mem_n;

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

    char **opts = malloc(sizeof *opts * BUF_MAX);
    struct expr *vals = malloc(sizeof *vals * BUF_MAX);
    assert(opts); assert(vals);
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
    p->type.opts = opts;
    p->type.vals = vals;
    p->type.opts_n = opts_n;

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
            struct type **args = malloc(sizeof *args * BUF_MAX);
            assert(args);
            int args_n = 0;

            for(;;){
                MAYBE(TOKEN_RPAREN) break;
                if(args_n > 0) EXPECT(TOKEN_COMMA);

                parse_type_expr(p);
                assert(args_n < BUF_MAX);
                args[args_n++] = type_alloc(p->type);
            }

            //Parse returns
            struct type **ret = malloc(sizeof *ret * BUF_MAX);
            assert(ret);
            int ret_n = 0;

            MAYBE(TOKEN_LPAREN) {
                for(;;){
                    MAYBE(TOKEN_RPAREN) break;
                    if(ret_n > 0) EXPECT(TOKEN_COMMA);

                    parse_type_expr(p);
                    assert(ret_n < BUF_MAX);
                    ret[ret_n++] = type_alloc(p->type);
                }
            } else {
                parse_type_expr(p);
                ret[ret_n++] = type_alloc(p->type);
            }

            p->type.args = args;
            p->type.args_n = args_n;
            p->type.ret = ret;
            p->type.ret_n = ret_n;
            p->type.type = TYPE_FUNC;

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
