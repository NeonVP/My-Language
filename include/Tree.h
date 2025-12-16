#ifndef TREE_H
#define TREE_H

#include <stdio.h>

#include "Language.h"

#ifdef _LINUX
#include <linux/limits.h>
const size_t MAX_LEN_PATH = PATH_MAX;
#else
const size_t MAX_LEN_PATH = 256;
#endif


#define OPERATIONS_ENUM( str, name, value, ... ) \
    name = value,

enum NodeType {
    NODE_UNKNOWN = -1,

    NODE_NUMBER    = 0,
    NODE_VARIABLE  = 1,
    NODE_OPERATION = 2
};

enum OperationType {
    OP_NOPE = 0,
    
    INIT_OPERATIONS( OPERATIONS_ENUM )
};

#undef INIT_OP_ENUM

struct NodeValue {
    enum NodeType type;

    union {
        int   number; 
        char* variable; 
        int   operation; 
    } data;
};

typedef NodeValue TreeData_t;

struct Node_t {
    TreeData_t value;

    Node_t* right;
    Node_t* left;

    Node_t* parent;
};

struct Tree_t {
    Node_t* root;
};

Tree_t* TreeCtor();
void    TreeDtor( Tree_t** tree, void ( *clean_function ) ( TreeData_t value, Tree_t* tree ) );

Node_t* NodeCreate( const TreeData_t field, Node_t* parent );
void    NodeDelete( Node_t* node, Tree_t* tree, void ( *clean_function ) ( TreeData_t value, Tree_t* tree ) );

Node_t* NodeLeftCreate ( const TreeData_t field, Node_t* parent );
Node_t* NodeRightCreate( const TreeData_t field, Node_t* parent );

Node_t* NodeCopy( Node_t* node );

void NodeGraphicDump( const Node_t* node, const char* image_path_name, ... );

#endif//TREE_H