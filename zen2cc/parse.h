#pragma once

#include "token.h"

struct parse_state {

};


void parse_init(struct parse_state *p);
void parse_free(struct parse_state *p);
char * parse_token(struct parse_state *p, struct token t);
void parse_state_print(struct parse_state *p);
