#ifndef PARAMLIST_H
#define PARAMLIST_H

#include "typeandname.h"

struct paramlist {
  struct typeandname *head;
  struct typeandname **tail;
};

struct paramlist *newparamlist(void);

void addParam(struct paramlist *tl, struct typeandname *tan);

#endif
