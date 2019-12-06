#pragma once
#include <stdbool.h>
#include <stdio.h>

struct stream {
    FILE *f;
    int line, col;
    int pcol, pchar;
};

bool stream_init_file(struct stream *s, char *path);
int stream_getc(struct stream *s);
int stream_ungetc(struct stream *s);

