
#pragma once

#include "token.h"

#define PARSE_MAX_DEPTH 10

enum parse_status {
    PARSE_ERR = -1,
    PARSE_ROOT = 0,
    PARSE_TYPEDEF,
    PARSE_FUNC,
    PARSE_STRUCT,
    PARSE_ENUM,
    PARSE_CONST,
    PARSE_LET,
    PARSE_INCLUDE,
    PARSE_INCLUDE_IDENT,
    PARSE_INCLUDE_SEMICOLON,
};

struct parse_state {
    enum parse_status status[PARSE_MAX_DEPTH];
    size_t status_i;
    struct token stack[PARSE_MAX_DEPTH];
    size_t stack_i;
};

enum parse_node_type {
    NODE_NONE = 0,
    NODE_INCLUDE,           //Simple include statement `include "package"`
    NODE_INCLUDE_DEFINE,    //Include statement with package definition
};

#define PARSE_NODE_MAX_TOKEN 5

struct parse_node {
    enum parse_node_type type;
    struct token t[PARSE_NODE_MAX_TOKEN];
};


void parse_init(struct parse_state *p);
void parse_free(struct parse_state *p);
char * parse_token(struct parse_state *p, struct parse_node *n, struct token t);
void parse_node_print(struct parse_node *n);
