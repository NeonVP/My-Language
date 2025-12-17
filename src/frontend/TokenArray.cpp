#include <stdlib.h>
#include <string.h>

#include "Tree.h"
#include "frontend/TokenArray.h"

#define TOKEN_ARRAY_DEFAULT_CAPACITY 16
#define TOKEN_ARRAY_GROWTH_FACTOR 2

TokenArray_t TokenArrayCreate( void ) {
    TokenArray_t arr = { .data = NULL, .size = 0, .capacity = 0 };
    return arr;
}

void TokenArrayDestroy( TokenArray_t *arr ) {
    if ( !arr )
        return;

    // IMPORTANT:
    // Tokens are later re-linked into an AST by the syntax analyzer (via left/right/parent).
    // Therefore TokenArrayDestroy must NOT use NodeDelete(), because NodeDelete() recursively
    // deletes children and would double-free / UAF when the array still contains pointers to
    // nodes already freed through the AST links.
    for ( size_t i = 0; i < arr->size; i++ ) {
        Node_t *node = arr->data[i];
        if ( !node )
            continue;

        if ( node->value.type == NODE_VARIABLE )
            free( node->value.data.variable );

        free( node );
    }

    free( arr->data );
    arr->data = NULL;
    arr->size = 0;
    arr->capacity = 0;
}

static bool TokenArrayReallocate( TokenArray_t *arr, size_t new_capacity ) {
    if ( new_capacity == 0 ) {
        free( arr->data );
        arr->data = NULL;
        arr->capacity = 0;
        return true;
    }

    Node_t **new_data = (Node_t **)realloc( arr->data, new_capacity * sizeof( Node_t * ) );
    if ( !new_data ) {
        return false;
    }

    arr->data = new_data;
    arr->capacity = new_capacity;

    return true;
}

bool TokenArrayPushBack( TokenArray_t *arr, Node_t *token ) {
    if ( !arr )
        return false;

    if ( arr->size >= arr->capacity ) {
        size_t new_capacity =
            arr->capacity == 0 ? TOKEN_ARRAY_DEFAULT_CAPACITY : arr->capacity * TOKEN_ARRAY_GROWTH_FACTOR;

        if ( !TokenArrayReallocate( arr, new_capacity ) ) {
            return false;
        }
    }

    arr->data[arr->size++] = token;
    return true;
}

size_t TokenArraySize( const TokenArray_t *arr ) { return arr ? arr->size : 0; }