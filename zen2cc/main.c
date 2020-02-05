#include <stdio.h>
#include "common.h"
#include "token.h"

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Expected 1 argument <testfile>");
        return 1;
    }

    char *filename = argv[1];
    struct stream s;
    if(!stream_init_file(&s, filename)) {
        fprintf(stderr, "ERR: Could not open file \"%s\"\n", filename);
        return 2;
    }

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
