#pragma once

#include "token.h"

enum expr_type {
    EXPR_NONE,                  //Empty expression
    EXPR_NUM,                   //A numeric constant
};

struct expr {
    enum expr_type type;
    union {
        struct token num;
    };
};

void expr_free(struct expr *e);
void expr_print(struct expr *e);
