#include <stdlib.h>

#include "funcdecl.h"

struct funcdecl *newfuncdecl()
{
    struct funcdecl *fd = calloc(sizeof(struct funcdecl), 1);
    fd->name = NULL;
    fd->p = NULL;
    fd->ret = NULL;
    return fd;
}
