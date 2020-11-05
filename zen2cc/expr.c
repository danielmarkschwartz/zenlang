#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "expr.h"
#include "type.h"

void expr_free(struct expr *e) {
    assert(e);
    switch(e->type) {
    case EXPR_NONE: break;
    case EXPR_NUM: break;
    case EXPR_STR: break;
    case EXPR_IDENT: break;
    case EXPR_POSTINC: expr_free(e->l); break;
    case EXPR_POSTDEC: expr_free(e->l); break;
    case EXPR_FCALL: expr_free(e->f);
                     for(int i=0; i<e->args_n; i++)
                         expr_free(e->args);
                     e->args_n = 0;
                     free(e->args);
                     break;
    case EXPR_ARRSUB:expr_free(e->l); expr_free(e->r); break;
    case EXPR_SACC:expr_free(e->l); expr_free(e->r); break;
    case EXPR_TACC:type_free(&e->tacc.t); expr_free(e->tacc.m); break;
    case EXPR_COMP_LIT: type_free(&e->t);
                     for(int i=0; i<e->vals_n; i++)
                         expr_free(e->vals);
                     e->vals_n = 0;
                     free(e->vals);
                     break;

    }
}

void expr_print(struct expr *e) {
    assert(e);

    switch(e->type) {
    case EXPR_NONE:     return;
    case EXPR_STR:      printf("STR %.*s", e->lit.len, e->lit.str); return;
    case EXPR_IDENT:    printf("IDENT %.*s", e->lit.len, e->lit.str); return;
    case EXPR_NUM:      printf("NUM %.*s", e->lit.len, e->lit.str); return;
    case EXPR_POSTINC:  expr_print(e->l); printf(" ++"); return;
    case EXPR_POSTDEC:  expr_print(e->l); printf(" --"); return;
    case EXPR_FCALL:
        expr_print(e->f);
        printf("(");
        for(int i=0; i < e->args_n; i++) {
            if(i>0) printf(", ");
            expr_print(&e->args[i]);
        }
        printf(")");
        return;
    case EXPR_ARRSUB:
        expr_print(e->l); printf("["); expr_print(e->r); printf("]"); return;
    case EXPR_SACC: expr_print(e->l); printf(" SACC "); expr_print(e->r); return;
    case EXPR_TACC: type_print(&e->tacc.t); printf(" TACC "); expr_print(e->tacc.m); return;
    case EXPR_COMP_LIT:
        printf("(");
        type_print(&e->t);
        printf("){");
        for(int i=0; i < e->vals_n; i++) {
            if(i>0) printf(", ");
            expr_print(&e->vals[i]);
        }
        printf("}");
        return;
    default:
        printf("Reached DEFAULT %i\n", e->type);
    }

    assert(0); //Should not be reached
}

struct expr *expr_alloc(struct expr e) {
    struct expr *ret = malloc(sizeof *ret);
    assert(ret);
    *ret = e;
    return ret;
}
