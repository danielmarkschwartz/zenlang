#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parse.h"

char *type_primative_str[TYPE_NUM] = {
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

void ts_free(struct ts *ts) {
    if(ts == NULL) return;
    for(int i = 0; i < ts->n; i++) {
        free(ts->key[i]);
        switch(ts->val[i].type) {
        case TYPE_VOID: case TYPE_PRIMATIVE: break;
        default: assert(0);
        }
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
    t = token_stream_next(p->ts);\
    if(t.type != ttype){\
        token_stream_rewind(p->ts);\
        snprintf(err_buf, ERRBUF_SIZE, "Expected token %s", token_type_str[ttype]);\
        return err_buf;\
    }\
}while(0)

#define MAYBE(ttype)\
    t = token_stream_peek(p->ts);\
    if(t.type == ttype) token_stream_next(p->ts);\
    if(t.type == ttype)\

#define ERRF(str, ...) do{\
    snprintf(err_buf, ERRBUF_SIZE, str, __VA_ARGS__);\
    return err_buf;\
}while(0)

static char *parse_include(struct parse *p) {
    struct token t;
    token_stream_mark(p->ts);

    EXPECT(TOKEN_INCLUDE);
    EXPECT(TOKEN_STR_ESC);

    char *path = token_str(t);
    assert(path);

    token_stream_unmark(p->ts);

    char *ident = NULL;
    MAYBE(TOKEN_IDENT){
        ident = token_str(t);
    } else {
        //Essentially basename
        char *s = strrchr(path, '/');
        if(s) ident = &s[1];
        else ident = path;
    }

    ns_set(&p->globals, ident, (struct val){VAL_MODULE, .mod={path}});

    return NULL;
}

static char *parse_type_expr(struct parse *p) {
    struct token t;
    token_stream_mark(p->ts);

    p->type.type = TYPE_ERR;

    //Parse primitive type
    EXPECT(TOKEN_IDENT);

    enum type_primative pt;
    for(pt = 0; pt < TYPE_NUM; pt++)
        if( t.len == strlen(type_primative_str[pt])
            && strncmp(type_primative_str[pt], t.str, t.len) == 0)
            break;

    if(pt == TYPE_NUM)
        ERRF("Expected primitive type, but got %.*s", t.len, t.str);

    p->type.type = TYPE_PRIMATIVE;
    p->type.primative = pt;

    //TODO: implement other parse expr nodes

    return NULL;
}

static char *parse_typedef(struct parse *p) {
    struct token t;
    token_stream_mark(p->ts);

    EXPECT(TOKEN_TYPEDEF);
    EXPECT(TOKEN_IDENT);

    char *ident = token_str(t);
    assert(ident);

    char *err = parse_type_expr(p);
    if(err) {
        token_stream_rewind(p->ts);
        return err;
    }

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

        //TODO: parse other top level constructs

        if(err){
            p->error(p->ts, token_stream_next(p->ts), err);
            while(token_stream_next(p->ts).type != TOKEN_NEWLINE);
            errnum++;
        }
    }

    return errnum;
}
