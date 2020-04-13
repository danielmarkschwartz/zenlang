
#pragma once

#include "token.h"

enum type_type {
    TYPE_NONE = -2,
    TYPE_ERR = -1,

    TYPE_PRIMATIVE = 0,     //Any explicit non-compound type (int, float, etc)
    TYPE_IDENT,             //An identifier in type namespace (mytype or module->mytype)
    TYPE_PTR,               //Pointer to
    TYPE_ARRAY,             //Array of
    TYPE_FUNC,              //Function
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
            struct type **args, **ret;
            int args_n, ret_n;
        };
    };
};

void type_print(struct type *t);

#define TS_INITIAL_CAP 8

struct ts {
    char **key;
    struct type *val;
    int c, n;
};

void ts_init(struct ts *ts);
void ts_free(struct ts *ts);
void ts_set(struct ts *ts, char *key, struct type val);
struct type *ts_get(struct ts *ts, char *key);

enum val_type {
    VAL_MODULE
};

struct val_module {
    char *path;
};

struct val {
    enum val_type type;

    union {
        struct val_module mod;
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

typedef void (*error_func)(struct token_stream *ts, struct token, char*);

struct parse{
    struct token_stream *ts;
    struct ns globals;
    struct ts types;
    error_func error;

    struct type type;
};

void parse_init(struct parse *p, struct token_stream *ts, error_func err);
void parse_free(struct parse *p);

int parse(struct parse *p);
