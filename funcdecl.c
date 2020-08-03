#include <stdlib.h>

#include "funcdecl.h"

struct funcdecl *newfuncdecl()
{
    struct funcdecl *fd = calloc(sizeof(struct funcdecl), 1);
    fd->name = NULL;
    fd->p = NULL;
    fd->ret = NULL;
    fd->isnative = false;
    fd->isconst = false;
    return fd;
}
