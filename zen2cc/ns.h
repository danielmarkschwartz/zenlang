#pragma once

#include "type.h"
#include "expr.h"

enum val_type {
    VAL_MODULE,         //Reference to external module
    VAL_CONST,          //Constant value
    VAL_VAR,            //Global variable
    VAL_FUNC,           //Function definition
};

struct val {
    enum val_type type;

    union {
        char *mod_path;
        struct {
            struct expr expr;
            struct type expr_type;
        };
        struct {
            char *mod, *type_ident, **args;
            struct type *args_type, *ret_type;
            int args_n, ret_n;
            struct expr func_expr;
        };
    };
};

#define NS_INITIAL_CAP 8

struct ns {
    char **key;
    struct val *val;
    int c, n;
};

void ns_init(struct ns *ns);
void ns_free(struct ns *ns);
void ns_set(struct ns *ns, char *key, struct val val);
struct val *ns_get(struct ns *ns, char *key);
