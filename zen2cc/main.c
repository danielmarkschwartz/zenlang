#include <stdio.h>
#include "common.h"
#include "token.h"

int main(int argc, char **argv) {
    printf("Zen Language to C Compiler\n");

    struct stream s;
    if(!stream_init_file(&s, "test.zen"))
        fprintf(stderr, "ERR: Could not open file\n");

    struct token t;
    for(;;) {
        t = token_next(&s);
        token_print(t);
        token_free(t);
        if(t.type == TOKEN_ERR || t.type == TOKEN_EOF)
            break;
    }

    return 0;
}
