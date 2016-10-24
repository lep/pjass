#ifndef HASHTABLE_H
#define HASHTABLE_H

// mingw needs this to get a definition of _off_t
#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>


struct hashnode {
  const char *name;
  void *val;
};

struct hashtable {
  size_t size;
  size_t count;
  struct hashnode *bucket;
};

void ht_init(struct hashtable *h, size_t size);
void *ht_lookup(struct hashtable *h, const char *name);
bool ht_put(struct hashtable *h, const char *name, void *val);
void ht_clear(struct hashtable *h);

#endif
