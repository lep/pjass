#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "tree.h"


void tree_init(struct tree *t){
    t->root = NULL;
}
void* tree_lookup(struct tree *t, uint32_t key){
    struct treenode *cur = t->root;
    
    while(cur != NULL){
        if( key == cur->key ){
            return cur->value;
        }else if(key < cur->key ){
            cur = cur->left;
        }else {
            cur = cur->right;
        }
    }
    return NULL;
}

void tree_put(struct tree *t, uint32_t key, void *value){
    struct treenode *new =  malloc(sizeof(struct treenode));
    new->key = key;
    new->value = value;
    new->left = NULL;
    new->right = NULL;
    
    struct treenode *cur = t->root;
    if(t->root == NULL){
        t->root = new;
        return;
    }
    
    while(true){
        if( key == cur->key ){
            cur->value = value;
            return;
            
        }else if( key < cur->key ){
            if(cur->left == NULL){
                cur->left = new;
                return;
            }else {
                cur = cur->left;
            }
        }else {
            if(cur->right == NULL){
                cur->right = new;
                return;
            }else {
                cur = cur->right;
            }
        }
    }
}

