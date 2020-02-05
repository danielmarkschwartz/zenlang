#pragma once

#include "token.h"

enum ast_node_type {
    AST_NONE = 0,
    AST_INCLUDE,

};

struct ast_node {
    enum ast_node_type type;            //Ast node type
    struct token token;                 //Associated token if any
};


enum parse_state {
    PARSE_TOP = 0,
};

struct parser {
    enum parse_state state;
};

