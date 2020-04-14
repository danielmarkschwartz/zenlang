#pragma once

#include "type.h"

#define TS_INITIAL_CAP 8

struct ts {
    char **key;
    struct type *val;
    int c, n;
};

void ts_init(struct ts *ts);
void ts_free(struct ts *ts);
void ts_set(struct ts *ts, char *key, struct type val);
struct type *ts_get(struct ts *ts, char *key);
