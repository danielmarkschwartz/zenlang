#include <assert.h>
#include "stream.h"

bool stream_init_file(struct stream *s, char *path) {
    assert(s); assert(path);
    FILE *f = fopen(path, "r");
    if(!f) return false;

    s->f = f;
    s->line = 1;
    s->col = 1;
    s->pcol = 0;
    s->pchar = EOF;

    return true;
}

int stream_getc(struct stream *s) {
    assert(s);
    int c = fgetc(s->f);
    s->pchar = c;
    if(c == EOF) return c;


    if(c == '\n') {
        s->line ++;
        s->pcol = s->col;
        s->col = 1;
    } else {
        s->col ++;
    }

    return c;
}

int stream_ungetc(struct stream *s) {
    assert(s);
    assert(s->pchar != 0);

    if( ungetc(s->pchar, s->f) == EOF)
        return false;

    if(s->pchar == '\n') {
        s->col = s->pcol;
        s->line --;
    } else {
        s->col --;
    }

    s->pchar = EOF;
    return true;
}
