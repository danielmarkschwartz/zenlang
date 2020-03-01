#include <stdio.h>
#include <string.h>
#include "common.h"
#include "token.h"
#include "parse.h"

int main(int argc, char **argv) {
    enum {TOKENS, PARSE, CC} output = CC;

    if(argc < 2 || argc > 3) {
        fprintf(stderr, "Expected 1 argument <testfile>\n");
        return 1;
    }

    char *filename = argv[1];

    if(argc == 3){
        filename = argv[2];
        if(strcmp(argv[1], "-t") == 0) output = TOKENS;
        else if(strcmp(argv[1], "-p") == 0) output = PARSE;
        else {
            fprintf(stderr, "Unexpected argument \"%s\"\n", argv[1]);
            return 1;
        }
    }

    struct stream s;
    if(!stream_init_file(&s, filename)) {
        fprintf(stderr, "ERR: Could not open file \"%s\"\n", filename);
        return 2;
    }

    struct token t;
    struct parse_state p;

    parse_init(&p);

    do {
        t = token_next(&s);
        if(output == TOKENS) {
            token_print(t);
            continue;
        }

        struct parse_node n;
        char *err = parse_token(&p, &n, t);
        if(err) {
            fprintf(stdout, "PERR: %s\n", err);
            break;
        }


        if(output == PARSE) {
            parse_node_print(&n);
        }
    } while(t.type != TOKEN_ERR && t.type != TOKEN_EOF);

    parse_free(&p);

    return 0;
}
