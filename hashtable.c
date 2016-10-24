
#include <string.h>
#include <stdlib.h>

#include "hashtable.h"

static uint32_t hashfunc(const char *key)
{
//murmur3_32
	static const uint32_t c1 = 0xcc9e2d51;
	static const uint32_t c2 = 0x1b873593;
	static const uint32_t r1 = 15;
	static const uint32_t r2 = 13;
	static const uint32_t m = 5;
	static const uint32_t n = 0xe6546b64;
 
    uint32_t len = strlen(key);
	uint32_t hash = 0;
 
	const int nblocks = len / 4;
	const uint32_t *blocks = (const uint32_t *) key;
	int i;
	for (i = 0; i < nblocks; i++) {
		uint32_t k = blocks[i];
		k *= c1;
		k = (k << r1) | (k >> (32 - r1));
		k *= c2;
 
		hash ^= k;
		hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
	}
 
	const uint8_t *tail = (const uint8_t *) (key + nblocks * 4);
	uint32_t k1 = 0;
 
	switch (len & 3) {
	case 3:
		k1 ^= tail[2] << 16;
	case 2:
		k1 ^= tail[1] << 8;
	case 1:
		k1 ^= tail[0];
 
		k1 *= c1;
		k1 = (k1 << r1) | (k1 >> (32 - r1));
		k1 *= c2;
		hash ^= k1;
	}
 
	hash ^= len;
	hash ^= (hash >> 16);
	hash *= 0x85ebca6b;
	hash ^= (hash >> 13);
	hash *= 0xc2b2ae35;
	hash ^= (hash >> 16);
 
	return hash;
}

void ht_init(struct hashtable *h, size_t size)
{
    h->count = 0;
    h->size = size;
    h->bucket = calloc(sizeof(struct hashnode), h->size);
}

void * ht_lookup(struct hashtable *h, const char *name)
{
    size_t start = hashfunc(name);
    size_t idx = (start + 1) % h->size;
    for(; idx != start; idx = (idx + 1) % h->size){
        if(h->bucket[idx].name){
            if(!strcmp(h->bucket[idx].name, name)){
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
    ht_init(&newht, h->size*2 +1);
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
    if (ht_lookup(h, name) != NULL) {
        return false;
    }

    size_t start = hashfunc(name);
    size_t idx = (start + 1) % h->size;
    for(; /*idx != start*/; idx = (idx + 1) % h->size){
        if(!h->bucket[idx].name){
            h->bucket[idx].name = name;
            h->bucket[idx].val = val;
            break;
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
