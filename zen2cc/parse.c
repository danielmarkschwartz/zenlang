#include <assert.h>
#include "common.h"
#include "parse.h"

void parse_init(struct parse_state *p) {
    assert(p);
    p->status[0] = PARSE_ROOT;
    p->status_i = 0;
}
void parse_free(struct parse_state *p) {
    if(p == NULL) return;
}

#define ERR_LEN 256
char err[ERR_LEN];

#define bad_token(exp) do{\
    p->status[p->status_i] = PARSE_ERR;\
    snprintf(err, ERR_LEN,\
            "line %i col %i - Unexpected token %s (%s), expected %s",\
            t.line, t.col, token_type_str[t.type], t.value ? t.value : "", (exp));\
    n->type = NODE_NONE;\
    return err;\
}while(0)

#define status_append(s) do{\
    assert(++p->status_i < PARSE_MAX_DEPTH);\
    p->status[p->status_i] = (s);\
    n->type = NODE_NONE;\
    return NULL;\
}while(0)

#define status_set(s) do{\
    p->status[p->status_i] = (s);\
    n->type = NODE_NONE;\
    return NULL;\
}while(0)

#define status_yield(s, P, N) do{\
    p->status[p->status_i] = (s);\
    n->type = (P);\
    size_t NN = (N);\
    assert(NN <= PARSE_NODE_MAX_TOKEN);\
    assert(NN <= p->stack_i);\
    for(int i = 0; i < NN; i++)\
        n->t[i] = p->stack[p->stack_i + i - NN];\
    p->stack_i -= NN;\
    return NULL;\
}while(0)

#define status_set_return(s) do{\
    p->status[p->status_i] = (s);\
    n->type = NODE_NONE;\
}while(0)

#define _status_pop(s, N) do{\
    size_t NN = (N);\
    assert(p->status_i > 0);\
    assert(NN <= PARSE_NODE_MAX_TOKEN);\
    assert(NN <= p->stack_i);\
    p->status_i--;\
    n->type = (s);\
    for(int i = 0; i < NN; i++)\
        n->t[i] = p->stack[p->stack_i + i - NN];\
    p->stack_i -= NN;\
}while(0)

#define status_pop(s, N) do{\
    _status_pop(s,N);\
}while(0); continue

#define status_pop_consume(s, N) do{\
    _status_pop(s,N);\
    return NULL;\
}while(0)

#define status_begin(s, r, P, N) do{\
    p->status[p->status_i] = (r);\
    assert(++p->status_i < PARSE_MAX_DEPTH);\
    p->status[p->status_i] = (s);\
    size_t NN = (N);\
    assert(NN <= PARSE_NODE_MAX_TOKEN);\
    assert(NN <= p->stack_i);\
    n->type = (P);\
    for(int i = 0; i < NN; i++)\
        n->t[i] = p->stack[p->stack_i + i - NN];\
    p->stack_i -= NN;\
    return NULL;\
}while(0)

#define status_append_token(t) do{assert(p->stack_i < PARSE_MAX_DEPTH); p->stack[p->stack_i++] = (t);}while(0)

char * parse_token(struct parse_state *p, struct parse_node *n, struct token t){
    assert(p);
    assert(n);

    for(;;) {
        //printf("PARSING %i\n", p->status[p->status_i]);
        switch(p->status[p->status_i]) {
            //Document top level
            case PARSE_ROOT:
                switch(t.type) {
                    case TOKEN_TYPEDEF: status_append(PARSE_TYPEDEF);
                    case TOKEN_FUNC: status_append(PARSE_FUNC);
                    case TOKEN_STRUCT: status_append(PARSE_STRUCT);
                    case TOKEN_ENUM: status_append(PARSE_ENUM);
                    case TOKEN_CONST: status_append(PARSE_CONST);
                    case TOKEN_LET: status_append(PARSE_LET);
                    case TOKEN_INCLUDE: status_append(PARSE_INCLUDE);
                    case TOKEN_EOF:
                    case TOKEN_NEWLINE: n->type = NODE_NONE; return NULL;
                    default: bad_token("typdef, func, struct, enum, const, let, or include");
                } break;

            //^include " package " ident? $
            //           ^
            case PARSE_INCLUDE:
                if(t.type != TOKEN_STR_ESC) bad_token("include path");
                status_append_token(t);
                status_set(PARSE_INCLUDE_IDENT);

            //^include " package " ident? $
            //                     ^
            case PARSE_INCLUDE_IDENT:
                if(t.type != TOKEN_IDENT && t.type != TOKEN_NEWLINE && t.type != TOKEN_EOF)
                    bad_token("expected ; or include identifier");
                else if(t.type == TOKEN_IDENT) {
                    status_append_token(t);
                    status_set(PARSE_INCLUDE_NEWLINE);
                } else if(t.type == TOKEN_NEWLINE || t.type == TOKEN_EOF)
                    status_pop_consume(NODE_INCLUDE, 1);

            //^include " package " ident? $
            //                            ^
            case PARSE_INCLUDE_NEWLINE:
                if(t.type != TOKEN_NEWLINE && t.type != TOKEN_EOF)
                    bad_token("; at end of include statement");
                status_pop_consume(NODE_INCLUDE_DEFINE, 2);

            //^typedef ident type_expr$
            //         ^
            case PARSE_TYPEDEF:
                if(t.type != TOKEN_IDENT)
                    bad_token("type identifier");

                status_append_token(t);
                status_begin(PARSE_TYPE_EXPR, PARSE_TYPEDEF_FINAL, NODE_TYPEDEF_BEGIN, 1);

            //^typedef ident type_expr$
            //                        ^
            case PARSE_TYPEDEF_FINAL:
                if(t.type != TOKEN_NEWLINE && t.type != TOKEN_EOF)
                    bad_token("newline after typedef");
                status_pop_consume(NODE_TYPEDEF_END,0);

            //ident follow_ident
            //func ( type_args ) follow_all
            //struct { type_args } follow_all
            //^
            case PARSE_TYPE_EXPR:
                switch(t.type) {
                    case TOKEN_IDENT:
                        status_append_token(t);
                        status_yield(PARSE_TYPE_EXPR_FOLLOW_IDENT, NODE_TYPE_IDENT, 1);
                    case TOKEN_FUNC:
                        status_set(PARSE_TYPE_EXPR_FUNC);
                    case TOKEN_STRUCT:
                        status_set(PARSE_TYPE_EXPR_STRUCT);

                    default: bad_token("type expression");
                }

            //ident -> ident follow_ident
            //ident [] follow_any
            //ident [ expr ] follow_any
            //ident * follow_any
            //      ^
            case PARSE_TYPE_EXPR_FOLLOW_IDENT:
                switch(t.type) {
                    case TOKEN_RARR:
                        status_set(PARSE_TYPE_EXPR_ACCESS);
                    case TOKEN_LBRA:
                        status_begin(PARSE_EXPR, PARSE_TYPE_EXPR_ARRAY_FINAL,
                                     NODE_TYPE_ARRAY_BEGIN, 0);
                    case TOKEN_MUL:
                        status_yield(PARSE_TYPE_EXPR_FOLLOW_ANY, NODE_TYPE_POINTER, 0);
                    default: break;
                };
                p->status_i--;
                continue;

            //type_expr []
            //type_expr [ expr ]
            //type_expr *
            //          ^
            case PARSE_TYPE_EXPR_FOLLOW_ANY:
                switch(t.type) {
                    case TOKEN_LBRA:
                        status_begin(PARSE_EXPR, PARSE_TYPE_EXPR_ARRAY_FINAL,
                                     NODE_TYPE_ARRAY_BEGIN, 0);
                    case TOKEN_MUL:
                        status_yield(PARSE_TYPE_EXPR_FOLLOW_ANY, NODE_TYPE_POINTER, 0);
                    default: break;
                };
                p->status_i--;
                continue;

            //ident -> ident
            //         ^
            case PARSE_TYPE_EXPR_ACCESS:
                if(t.type != TOKEN_IDENT)
                    bad_token("type identifier after ->");
                status_append_token(t);
                status_yield(PARSE_TYPE_EXPR_FOLLOW_ANY, NODE_TYPE_ACCESS, 1);

            // type_expr [ expr? ]
            //                   ^
            case PARSE_TYPE_EXPR_ARRAY_FINAL:
                if(t.type != TOKEN_RBRA)
                    bad_token("]");
                status_pop_consume(NODE_TYPE_ARRAY_END, 0);

            case PARSE_EXPR:
                switch(t.type) {
                    case TOKEN_NUM:
                        status_append_token(t);
                        status_pop_consume(NODE_NUM, 1);
                    default: break;
                }
                p->status_i--;
                continue;

            case PARSE_TYPE_EXPR_FUNC:
                assert(0);
                //TODO

            case PARSE_TYPE_EXPR_STRUCT:
                assert(0);
                //TODO



            default: printf("BAD PARSE STATE %i\n", p->status[p->status_i]); assert(0); //Should not be reached
        }
    }

    return NULL;
}

#define print_space() for(int i = 0; i < level; i++) printf("  ")

void parse_node_print(struct parse_node *node) {
    assert(node);
    static int level = 0;
    switch(node->type) {
        case NODE_NONE: print_space(); printf("...\n"); break;
        case NODE_INCLUDE:
            print_space();
            printf("include %s;\n", node->t[0].value); break;
        case NODE_INCLUDE_DEFINE:
            print_space();
            printf("include %s as %s;\n", node->t[0].value, node->t[1].value); break;
        case NODE_TYPEDEF_BEGIN:
            print_space();
            printf("typedef %s\n", node->t[0].value); level++; break;
        case NODE_TYPEDEF_END:
            level--;
            print_space();
            printf("typedef_end\n"); break;
        case NODE_TYPE_IDENT:
            print_space();
            printf("type_ident %s\n", node->t[0].value); break;
        case NODE_TYPE_ACCESS:
            print_space();
            printf("type_access -> %s\n", node->t[0].value); break;
        case NODE_TYPE_POINTER:
            print_space();
            printf("type_pointer \n"); break;
        case NODE_TYPE_ARRAY_BEGIN:
            print_space();
            printf("type_array_begin \n");
            level++;
            break;
        case NODE_TYPE_ARRAY_END:
            level--;
            print_space();
            printf("type_array_end \n"); break;

        case NODE_NUM:
            print_space();
            printf("number %s\n", node->t[0].value); break;

        default:
            printf("BAD NODE: %i\n", node->type);
    }
}
