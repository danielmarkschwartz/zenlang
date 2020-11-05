#pragma once

#include "token.h"
#include "type.h"

enum expr_type {
    EXPR_NONE,                  //Empty expression
    EXPR_NUM,                   //A numeric literal
    EXPR_STR,                   //A string literal
    EXPR_IDENT,                 //An identifier

    EXPR_POSTINC,               //Postfix increment ++
    EXPR_POSTDEC,               //Postfix decrement --
    EXPR_FCALL,                 //Function call ()
    EXPR_ARRSUB,                //Array subscript []
    EXPR_SACC,                  //Structure access .
    EXPR_TACC,                  //Type info access ->
    EXPR_COMP_LIT,              //Compound literal
};

struct expr {
    enum expr_type type;
    union {
        struct token lit;
        struct {struct expr *l, *r;};
        struct {struct expr *f, *args; int args_n;};
        struct {struct type t; struct expr *vals; int vals_n;};
        struct {struct type t; struct expr *m;} tacc;
    };
};

void expr_free(struct expr *e);
void expr_print(struct expr *e);
struct expr *expr_alloc(struct expr e);
