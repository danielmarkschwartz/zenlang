#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "type.h"

char *type_primative_str[TYPE_NUM] = {
    "void",
    "int", "int8", "int16", "int32", "int64",
    "uint", "uint8", "uint16", "uint32", "uint64",
    "float", "float16", "float32", "float64",
};

struct type *type_alloc(struct type t) {
    struct type *ret = malloc(sizeof *ret);
    assert(ret);
    *ret = t;
    return ret;
}

void type_free(struct type *t) {
    if(t == NULL) return;

    switch(t->type) {
    case TYPE_PRIMATIVE: break;
    case TYPE_IDENT: free(t->ident); free(t->mod); break;
    case TYPE_PTR: case TYPE_ARRAY: type_free(t->of); free(t->of); break;

    case TYPE_FUNC:
        for(int i = 0; i < t->args_n; i++) type_free(&t->args[i]);
        for(int i = 0; i < t->ret_n; i++) type_free(&t->ret[i]);
        free(t->args);
        free(t->ret);
        break;

    case TYPE_STRUCT:
        for(int i = 0; i < t->mem_n; i++) {
             free(t->idents[i]);
             type_free(&t->types[i]);
        }
        free(t->types); free(t->idents);
        break;

    case TYPE_ENUM:
        for(int i = 0; i < t->opts_n; i++) {
             free(t->opts[i]);
             expr_free(&t->vals[i]);
        }
        free(t->vals);
        free(t->opts);
        type_free(t->enum_type);
        free(t->enum_type);
        break;

    case TYPE_NONE: case TYPE_ERR: break;
    default: assert(0);
    }
}

void type_print(struct type *t) {

loop:
    switch(t->type){
        case TYPE_PRIMATIVE:
            assert(t->primative >= 0 && t->primative < TYPE_NUM);
            printf("PRIMITIVE %s", type_primative_str[t->primative]);
            break;
        case TYPE_IDENT:
            if(t->mod)
                printf("IDENT '%s'->'%s'", t->mod, t->ident);
            else printf("IDENT '%s'", t->ident);
            break;
        case TYPE_PTR:
            printf("PTR to "); t = t->of; goto loop;
        case TYPE_ARRAY:
            if(t->n >= 0) printf("ARRAY [%i] of ", t->n);
            else printf("ARRAY of ");
            t = t->of; goto loop;
        case TYPE_FUNC:
            printf("FUNC (");
            for(int i = 0; i<t->args_n; i++){
                if(i>0) printf(", ");
                type_print(&t->args[i]);
            }

            if(t->ret_n > 1) printf(") (");
            else printf(") ");

            for(int i = 0; i<t->ret_n; i++){
                if(i>0) printf(", ");
                type_print(&t->ret[i]);
            }

            if(t->ret_n > 1) printf(")");

            break;
        case TYPE_STRUCT:
            printf("STRUCT {\n");
            for(int i = 0; i < t->mem_n; i++) {
                printf("\t%s ", t->idents[i]);
                type_print(&t->types[i]);
                printf("\n");
            }
            printf("}");
            break;
        case TYPE_ENUM:
            printf("ENUM {\n");
            for(int i = 0; i < t->opts_n; i++) {
                if(t->vals[i].type == EXPR_NONE) printf("\t%s\n", t->opts[i]);
                else printf("\t%s = ", t->opts[i]), expr_print(&t->vals[i]), printf("\n");
            }
            if(t->enum_type && t->enum_type->type != TYPE_NONE) {
                printf("\tTYPE: ");
                type_print(t->enum_type);
                printf("\n");
            }
            printf("}");
            break;
        case TYPE_ERR: case TYPE_NONE: break;
        default: assert(0);
    }
}

