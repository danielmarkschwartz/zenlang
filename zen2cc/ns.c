#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ns.h"

void ns_init(struct ns *ns) {
    assert(ns);
    ns->key = malloc(NS_INITIAL_CAP * sizeof *ns->key);
    ns->val = malloc(NS_INITIAL_CAP * sizeof *ns->val);
    ns->c = NS_INITIAL_CAP;
    ns->n = 0;
    assert(ns->key);
    assert(ns->val);
}

void ns_free(struct ns *ns) {
    if(ns == NULL) return;
    for(int i = 0; i < ns->n; i++) {
        free(ns->key[i]);
        switch(ns->val[i].type) {
        case VAL_MODULE: free(ns->val[i].mod_path); break;
        case VAL_CONST: case VAL_VAR:
             type_free(&ns->val[i].expr_type);
             expr_free(&ns->val[i].expr);
             break;
        case VAL_FUNC:
             free(ns->val[i].mod);
             free(ns->val[i].type_ident);
             for(int j = 0; j < ns->val[i].args_n; j++) {
                 free(ns->val[i].args[j]);
                 type_free(&ns->val[i].args_type[j]);
             }
             for(int j = 0; j < ns->val[i].ret_n; j++)
                 type_free(&ns->val[i].ret_type[j]);
             free(ns->val[i].args);
             free(ns->val[i].args_type);
             free(ns->val[i].ret_type);
             expr_free(&ns->val[i].func_expr);
             break;
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

