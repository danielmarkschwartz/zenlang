#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "expr.h"
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
            token_print(&ts, t); printf("\n");
        } while(t.type != TOKEN_ERR && t.type != TOKEN_EOF);

        return 0;
    }

    struct parse p;
    parse_init(&p, &ts, print_err);
    int errnum = parse(&p);
    if(errnum) printf("GOT %i ERRORS\n", errnum);

    if(output == PARSE) {
        printf("\nGlobal namespace\n");
        struct ns ns = p.globals;
        for(int i = 0; i < ns.n; i++) {
            printf("%s: ", ns.key[i]);
            switch(ns.val[i].type){
            case VAL_MODULE: printf("MODULE '%s'\n", ns.val[i].mod_path); break;
            case VAL_CONST:
                 printf("CONST "); expr_print(&ns.val[i].expr);
                 if(ns.val[i].expr_type.type != TYPE_NONE) {
                     printf(" as ");
                     type_print(&ns.val[i].expr_type);
                 }
                 printf("\n"); break;
            case VAL_VAR:
                 printf("VAR");
                 if(ns.val[i].expr.type != EXPR_NONE){
                     printf(" ");
                     expr_print(&ns.val[i].expr);
                 }
                 if(ns.val[i].expr_type.type != TYPE_NONE) {
                     printf(" as ");
                     type_print(&ns.val[i].expr_type);
                 }
                 printf("\n"); break;
            case VAL_FUNC:
                 printf("FUNC");
                 if(ns.val[i].type_ident) printf(" member of %s", ns.val[i].type_ident);
                 if(ns.val[i].mod) printf(" in module %s", ns.val[i].mod);
                 printf("(");
                 for(int j = 0; j < ns.val[i].args_n; j++) {
                    if(j > 0) printf(", ");
                    printf("%s", ns.val[i].args[j]);
                    if(ns.val[i].args_type[j].type != TYPE_NONE){
                        printf(" ");
                        type_print(&ns.val[i].args_type[j]);
                    }
                 }
                 printf(")");
                 if(ns.val[i].ret_n) printf(" (");
                 for(int j = 0; j < ns.val[i].ret_n; j++) {
                    if(j > 0) printf(", ");
                    type_print(&ns.val[i].ret_type[j]);
                 }
                 if(ns.val[i].ret_n) printf(") ");
                 else printf(" ");
                 expr_print(&ns.val[i].func_expr);
                 printf("\n");
                 break;
            default: assert(0);
            }
        }

        printf("\nGlobal typespace\n");
        struct ts ts = p.types;
        for(int i = 0; i < ts.n; i++) {
            printf("%s: ", ts.key[i]);
            type_print(&ts.val[i]);
            printf("\n");
        }
    }

    parse_free(&p);

    return 0;
}
