#include <stdio.h>
#include <string.h>
#include "common.h"
#include "token.h"
#include "parse.h"

void print_err(struct token_stream *ts, struct token t, char *msg) {
    int row, col;
    token_pos(ts, t, &row, &col);
    fprintf(stderr, "ERROR [%i:%i] %s\n", row, col, msg);
}

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

    struct token_stream ts;
    if(!token_stream_init(&ts, filename)) {
        fprintf(stderr, "ERR: Could not open file \"%s\"\n", filename);
        return 2;
    }

    if(output == TOKENS) {
        struct token t;

        do {
            t = token_stream_next(&ts);
            token_print(&ts, t);
        } while(t.type != TOKEN_ERR && t.type != TOKEN_EOF);

        return 0;
    }

    struct parse p;
    parse_init(&p, &ts, print_err);
    int errnum = parse(&p);
    if(errnum) printf("GOT %i ERRORS\n", errnum);

    printf("\nGlobal namespace\n");
    if(output == PARSE) {
        struct ns ns = p.globals;
        for(int i = 0; i < ns.n; i++) {
            printf("%s: ", ns.key[i]);
            switch(ns.val[i].type){
            case VAL_MODULE: printf("MODULE '%s'\n", ns.val[i].mod.path);
            }
        }
    }

    parse_free(&p);

    return 0;
}
