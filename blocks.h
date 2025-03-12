#ifndef BLOCKS_H
#define BLOCKS_H

#include <stdbool.h>
// MSVC needs stddef for size_t
#include <stddef.h>

enum block_type {
    Function = 0,
    Loop,
    If
};
void block_push(int lineno, enum block_type type);
bool block_pop(enum block_type type, char *buf, size_t len);
void block_missing_error(char *msg, size_t len);

#endif

