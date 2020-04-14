#pragma once

#include "type.h"
#include "expr.h"

enum val_type {
    VAL_MODULE,         //Reference to external module
    VAL_CONST,          //Constant value, with potentially unspecified type
    VAL_VAR,            //Global variable
};

struct val {
    enum val_type type;

    union {
        char *mod_path;
        struct {
            struct expr expr;
            struct type expr_type;
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
