#ifndef FUNCDECL_H
#define FUNCDECL_H

#include <stdbool.h>

#include "typeandname.h"
#include "paramlist.h"

struct funcdecl {
  char *name;
  bool isconst;
  bool isnative;
  struct paramlist *p;
  const struct typenode *ret;
};

struct funcdecl *newfuncdecl(void);

#endif
