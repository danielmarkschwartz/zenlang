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
#define status_append(s) do{ assert(++p->status_i < PARSE_MAX_DEPTH); p->status[p->status_i] = (s); n->type = NODE_NONE; return NULL;}while(0)
#define status_set(s) do{ p->status[p->status_i] = (s); n->type = NODE_NONE; return NULL;}while(0)
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
    break;\
}while(0)
#define status_pop_consume(s, N) do{\
    _status_pop(s,N);\
    return NULL;\
}while(0)

#define status_append_token(t) do{assert(p->stack_i < PARSE_MAX_DEPTH); p->stack[p->stack_i++] = (t);}while(0)

char * parse_token(struct parse_state *p, struct parse_node *n, struct token t){
    assert(p);
    assert(n);

    for(;;) {
        printf("PARSING %i\n", p->status[p->status_i]);
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

            default: printf("BAD PARSE STATE %i\n", p->status[p->status_i]); assert(0); //Should not be reached
        }
    }

    return NULL;
}

void parse_node_print(struct parse_node *node) {
    assert(node);
    switch(node->type) {
        case NODE_NONE: printf("...\n"); break;
        case NODE_INCLUDE:
            printf("include %s;\n", node->t[0].value); break;
        case NODE_INCLUDE_DEFINE:
            printf("include %s as %s;\n", node->t[0].value, node->t[1].value); break;

        default:
            printf("BAD NODE: %i\n", node->type);
    }
}
