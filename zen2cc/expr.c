#include <assert.h>

#include "expr.h"

void expr_free(struct expr *e) {
    assert(e);
    switch(e->type) {
    case EXPR_NONE: break;
    case EXPR_NUM: break;
    }
}

