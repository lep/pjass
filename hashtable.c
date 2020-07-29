
#include <string.h>
#include <stdlib.h>

#include "hashtable.h"
#include "sstrhash.h"

void ht_init(struct hashtable *h, size_t size)
{
    h->count = 0;
    h->size = size;
    h->bucket = calloc(sizeof(struct hashnode), h->size);
}

void * ht_lookup(struct hashtable *h, const char *name)
{
    size_t start = hashfunc(name) & (h->size -1);
    size_t idx = (start + 1) & (h->size -1);
    for(; idx != start; idx = (idx + 1) & (h->size -1)){
        if(h->bucket[idx].name){
            if( !strcmp(h->bucket[idx].name, name)){
                return h->bucket[idx].val;
            }
        }else{
            break;
        }
    }
    return NULL;
}

static void resize(struct hashtable *h)
{
    struct hashtable newht;
    ht_init(&newht, h->size*2);
    size_t i;
    for(i = 0; i != h->size; i++){
        if(h->bucket[i].name){
            ht_put(&newht, h->bucket[i].name, h->bucket[i].val);
        }
    }
    free(h->bucket);
    h->bucket = newht.bucket;
    h->size = newht.size;
    h->count = newht.count;
}

bool ht_put(struct hashtable *h, const char *name, void *val)
{
    size_t start = hashfunc(name) & (h->size-1);
    size_t idx = (start + 1) & (h->size-1);
    for(; /*idx != start*/; idx = (idx + 1) & (h->size-1)){
        if(!h->bucket[idx].name){
            h->bucket[idx].name = name;
            h->bucket[idx].val = val;
            break;
        }else if( !strcmp(h->bucket[idx].name, name)){
            return false;
        }
    }
    h->count++;
    if(h->count*3/2 > h->size){
        resize(h);
    }
    return true;
}

void ht_clear(struct hashtable *h) {
    memset(h->bucket, 0, h->size*sizeof(struct hashnode));
    h->count = 0;
}
