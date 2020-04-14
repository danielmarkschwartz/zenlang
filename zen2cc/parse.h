#pragma once

#include "token.h"
#include "expr.h"
#include "ts.h"
#include "ns.h"

typedef void (*error_func)(struct token_stream *ts, struct token, char*);

struct parse{
    struct token_stream *ts;
    struct ns globals;
    struct ts types;
    error_func error;

    struct type type;
    struct expr expr;
};

void parse_init(struct parse *p, struct token_stream *ts, error_func err);
void parse_free(struct parse *p);

int parse(struct parse *p);
