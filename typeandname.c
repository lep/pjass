#include <string.h>
#include <stdlib.h>

#include "typeandname.h"

struct typeandname *newtypeandname(const struct typenode *ty, const char *name)
{
    struct typeandname *tan = calloc(sizeof(struct typeandname), 1);
    tan->ty = ty;
    tan->name = strdup(name);
    tan->next = NULL;
    return tan;
}

struct typenode *newtypenode(const char *typename, const struct typenode *superclass)
{
    struct typenode *result;
    result = aligned_alloc(8, sizeof(struct typenode));
    result->typename = strdup(typename);
    result->superclass = superclass;
    return result;
}



const struct typenode *getPrimitiveAncestor(const struct typenode *cur)
{
    while (getTypePtr(cur)->superclass)
        cur = getTypePtr(cur)->superclass;
    return cur;
}

int isDerivedFrom(const struct typenode *cur, const struct typenode *base)
{
    for(; getTypePtr(cur); cur = getTypePtr(cur)->superclass){
        if(typeeq(cur, base)){
            return 1;
        }
    }
    return 0;
}

struct typenode* mkretty(const struct typenode *ty, uint8_t ret)
{
    uintptr_t tagMask = (8-1);
    uintptr_t pointerMask = ~tagMask;
    uintptr_t ptr = (uintptr_t)ty;
    ret = ret & tagMask;
    return (struct typenode*)((ptr & pointerMask) | ret);
}

uint8_t getTypeTag(const struct typenode *ty)
{
    uintptr_t tagMask = (8-1);
    uintptr_t ptr = (uintptr_t)ty;
    return (uint8_t)(ptr & tagMask);
}

struct typenode* getTypePtr(const struct typenode *ty)
{
    uintptr_t tagMask = (8-1);
    uintptr_t pointerMask = ~tagMask;
    uintptr_t ptr = (uintptr_t)ty;
    return (struct typenode*)(ptr & pointerMask);
}

int typeeq(const struct typenode *a, const struct typenode *b)
{
    return getTypePtr(a) == getTypePtr(b);
}

