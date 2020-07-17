#ifndef TREE_H
#define TREE_H

#include <stdint.h>

struct treenode {
    void *value;
    uint32_t key;
    struct treenode *left;
    struct treenode *right;
};

struct tree {
    struct treenode *root;
};

void tree_init(struct tree *t);
void* tree_lookup(struct tree *t, uint32_t key);
void tree_put(struct tree *t, uint32_t key, void *value);

#endif
