#include <assert.h>
#include "ast.h"

/* Attempts to parse next token with given parser state, and sets return to next ast node if any. Returns number of tokens consumed (1 or 0). If token is not consumed, call again with same token until return is 1. Once TOKEN_EOF is consumed, parser is closed and will need to be re-initialized.
 * */
int ast_parse(struct parser *p, struct token t, struct ast_node *ret) {
    assert(p);

    switch(p->state) {
    case PARSE_TOP:
        //TODO: implement:
        ;
    }

    return 0;
}
