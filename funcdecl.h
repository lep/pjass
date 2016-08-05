#ifndef FUNCDECL_H
#define FUNCDECL_H

#include "typeandname.h"
#include "paramlist.h"

struct funcdecl {
  char *name;
  int isconst;
  struct paramlist *p;
  const struct typenode *ret;
};

struct funcdecl *newfuncdecl(void);

#endif
