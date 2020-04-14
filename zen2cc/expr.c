#include <assert.h>
#include <stdio.h>

#include "expr.h"

void expr_free(struct expr *e) {
    assert(e);
    switch(e->type) {
    case EXPR_NONE: break;
    case EXPR_NUM: break;
    }
}

void expr_print(struct expr *e) {
    switch(e->type) {
    case EXPR_NONE: return;
    case EXPR_NUM: printf("%.*s", e->num.len, e->num.str); return;
    }
    assert(0); //Should not be reached
}
