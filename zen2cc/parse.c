#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parse.h"


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
    p->ts = ts;
    p->error = err;
}

void parse_free(struct parse *p) {
    if(!p) return;
    ns_free(&p->globals);
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
        //TODO: parse other top level constructs
        if(err){
            p->error(p->ts, token_stream_next(p->ts), err);
            while(token_stream_next(p->ts).type != TOKEN_NEWLINE);
            errnum++;
        }
    }

    return errnum;
}
