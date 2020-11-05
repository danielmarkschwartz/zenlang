#pragma once

struct expr;

enum type_type {
    TYPE_NONE = -2,
    TYPE_ERR = -1,

    TYPE_PRIMATIVE = 0,     //Any explicit non-compound type (int, float, etc)
    TYPE_IDENT,             //An identifier in type namespace (mytype or module->mytype)
    TYPE_PTR,               //Pointer to ...
    TYPE_ARRAY,             //Array of ...
    TYPE_FUNC,              //Function with args ... returning ....
    TYPE_STRUCT,            //Struct of ...
    TYPE_ENUM,              //Struct of ...
};

enum type_primative {
    TYPE_VOID = 0,

    TYPE_INT,
    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_INT64,

    TYPE_UINT,
    TYPE_UINT8,
    TYPE_UINT16,
    TYPE_UINT32,
    TYPE_UINT64,

    TYPE_FLOAT,
    TYPE_FLOAT16,
    TYPE_FLOAT32,
    TYPE_FLOAT64,

    TYPE_NUM
};

extern char *type_primative_str[];

struct type {
    enum type_type type;
    union {
        enum type_primative primative;      //TYPE_PRIMATIVE
        struct {char *mod, *ident;};        //TYPE_IDENT
        struct {struct type *of; int n;};   //TYPE_PTR, TYPE_ARRAY
        struct {                            //TYPE_FUNC
            struct type *args, *ret;
            int args_n, ret_n;
        };
        struct {                            //TYPE_STRUCT
            char **idents;
            struct type *types;
            int mem_n;
        };
        struct {                            //TYPE_ENUM
            char **opts;
            struct expr *vals;
            struct type *enum_type;
            int opts_n;
        };
    };
};

void type_print(struct type *t);
void type_free(struct type *t);
struct type *type_alloc(struct type t);
