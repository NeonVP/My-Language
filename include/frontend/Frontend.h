#ifndef FRONTEND_H
#define FRONTEND_H

#include "Tree.h"

void TreeSaveToFile( const Tree_t* tree, const char* filename );
Tree_t* TreeReadFromBuffer( char* buffer );

#endif