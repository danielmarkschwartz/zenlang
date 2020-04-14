#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ts.h"

void ts_init(struct ts *ts) {
    assert(ts);
    ts->key = malloc(TS_INITIAL_CAP * sizeof *ts->key);
    ts->val = malloc(TS_INITIAL_CAP * sizeof *ts->val);
    ts->c = TS_INITIAL_CAP;
    ts->n = 0;

    assert(ts->key);
    assert(ts->val);
}

void ts_free(struct ts *ts) {
    if(ts == NULL) return;
    for(int i = 0; i < ts->n; i++) {
        free(ts->key[i]);
        type_free(&ts->val[i]);
    }
    free(ts->key);
    free(ts->val);
    ts->c = 0;
    ts->n = 0;
}

static int ts_find(struct ts *ts, char *key) {
    assert(ts); assert(key);

    for(int i = 0; i < ts->n; i++)
        if(strcmp(ts->key[i], key) == 0)
            return i;

    return -1;
}

void ts_set(struct ts *ts, char *key, struct type val) {
    assert(ts); assert(key);

    int i = ts_find(ts, key);
    if(i < 0) {
        if(ts->c >= ts->n) {
            int new_c = ts->c * 2;
            if(new_c < TS_INITIAL_CAP) new_c = TS_INITIAL_CAP;

            ts->key = reallocf(ts->key, new_c * sizeof *ts->key);
            ts->val = reallocf(ts->val, new_c * sizeof *ts->val);
            assert(ts->key); assert(ts->val);

            ts->c = new_c;
        }

        i = ts->n++;
    }

    ts->key[i] = strdup(key);
    ts->val[i] = val;
}

struct type *ts_get(struct ts *ts, char *key) {
    assert(ts); assert(key);

    int i = ts_find(ts, key);
    if(i < 0) return NULL;
    return &ts->val[i];
}


