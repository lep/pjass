#include <stdlib.h>

#include "paramlist.h"


struct paramlist *newparamlist()
{
    struct paramlist *tl = calloc(sizeof(struct paramlist), 1);
    tl->head = NULL;
    tl->tail = &tl->head;
    return tl;
}

void addParam(struct paramlist *pl, struct typeandname *tan)
{
    tan->next = *(pl->tail);
    *(pl->tail) = tan;
}



