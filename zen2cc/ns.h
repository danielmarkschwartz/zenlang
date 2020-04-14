#pragma once

enum val_type {
    VAL_MODULE
};

struct val_module {
    char *path;
};

struct val {
    enum val_type type;

    union {
        struct val_module mod;
    };
};

#define NS_INITIAL_CAP 8

struct ns {
    char **key;
    struct val *val;
    int c, n;
};

void ns_init(struct ns *ns);
void ns_free(struct ns *ns);
void ns_set(struct ns *ns, char *key, struct val val);
struct val *ns_get(struct ns *ns, char *key);
