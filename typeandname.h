#ifndef TYPEANDNAME_H
#define TYPEANDNAME_H

// mingw needs this to get a definition of _off_t
#include <stdio.h>
#include <stdint.h>

struct typenode {
  char *typename;
  const struct typenode *superclass;
};

struct typeandname {
  const struct typenode *ty;
  const char *name;
  int isarray, isconst, lineno, fn;
  struct typeandname *next;
};

struct typenode *newtypenode(const char *typename, const struct typenode *superclass);

struct typeandname *newtypeandname(const struct typenode *ty, const char *name);

const struct typenode *getPrimitiveAncestor(const struct typenode *cur);

int isDerivedFrom(const struct typenode *cur, const struct typenode *base);


struct typenode* mkretty(const struct typenode *ty, uint8_t ret);

struct typenode* getTypePtr(const struct typenode *ty);

uint8_t getTypeTag(const struct typenode *ty);

int typeeq(const struct typenode*, const struct typenode*);



#endif
