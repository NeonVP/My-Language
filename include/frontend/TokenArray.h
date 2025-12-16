#ifndef TOKEN_ARRAY_H
#define TOKEN_ARRAY_H

#include "Tree.h"

typedef struct {
    Node_t **data;
    size_t size;
    size_t capacity;
} TokenArray_t;

TokenArray_t TokenArrayCreate();
void TokenArrayDestroy( TokenArray_t *arr );

bool TokenArrayPushBack( TokenArray_t *arr, Node_t *token );

size_t TokenArraySize( const TokenArray_t *arr );

#endif // TOKEN_ARRAY_H