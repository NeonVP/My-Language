#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/stat.h>

#include "DebugUtils.h"
#include "Tree.h"
#include "UtilsRW.h"

const uint32_t fill_color = 0xb6b4b4;

#define OPERATIONS_STRINGS( token_str, op_enum, is_custom, nargs, string ) string,

static const char *operations_txt[] = { INIT_OPERATIONS( OPERATIONS_STRINGS ) };

#undef OPERATIONS_STRINGS

// Forward declarations for TreeLoadFromFile
static Node_t* NodeLoadRecursively( const char **pos );
static void SkipSpaces( const char **pos );
static bool MatchString( const char **pos, const char *str );
static int FindOperationType( const char *str );

Tree_t *TreeCtor() {
    Tree_t *new_tree = (Tree_t *)calloc( 1, sizeof( *new_tree ) );
    assert( new_tree && "Memory allocation error" );

    return new_tree;
}

void TreeDtor( Tree_t **tree, void ( *clean_function )( TreeData_t value, Tree_t *tree ) ) {
    my_assert( tree, "Null pointer on pointer on `tree`" );
    if ( *tree == NULL )
        return;

    NodeDelete( ( *tree )->root, *tree, clean_function );

    free( *tree );
    *tree = NULL;
}

Node_t *NodeCreate( const TreeData_t field, Node_t *parent ) {
    Node_t *new_node = (Node_t *)calloc( 1, sizeof( *new_node ) );
    assert( new_node && "Memory allocation error" );

    new_node->value = field;
    new_node->parent = parent;

    new_node->left = NULL;
    new_node->right = NULL;

    return new_node;
}

Node_t *NodeLeftCreate( const TreeData_t value, Node_t *parent ) {
    Node_t *node = NodeCreate( value, parent );
    parent->left = node;

    return node;
}

Node_t *NodeRightCreate( const TreeData_t value, Node_t *parent ) {
    Node_t *node = NodeCreate( value, parent );
    parent->right = node;

    return node;
}

void NodeDelete( Node_t *node, Tree_t *tree, void ( *clean_function )( TreeData_t value, Tree_t *tree ) ) {
    if ( !node ) {
        return;
    }

    if ( node->parent ) {
        if ( node->parent->left == node )
            node->parent->left = NULL;
        else
            node->parent->right = NULL;
    }

    if ( node->left )
        NodeDelete( node->left, tree, clean_function );

    if ( node->right )
        NodeDelete( node->right, tree, clean_function );

    if ( clean_function )
        clean_function( node->value, tree );

    // Free variable strings
    if ( node->value.type == NODE_VARIABLE && node->value.data.variable ) {
        free( node->value.data.variable );
    }

    free( node );
}

Node_t *NodeCopy( Node_t *node ) {
    if ( !node )
        return NULL;

    Node_t *new_node = NodeCreate( node->value, NULL );
    assert( new_node && "Memory allocation error" );

    new_node->parent = NULL;

    new_node->left = NodeCopy( node->left );
    if ( new_node->left )
        new_node->left->parent = new_node;

    new_node->right = NodeCopy( node->right );
    if ( new_node->right )
        new_node->right->parent = new_node;

    return new_node;
}

#ifdef _SIMPLIFIED_DUMP
#else
static uint32_t my_crc32_ptr( const void *ptr ) {
    uintptr_t val = (uintptr_t)ptr;
    uint32_t crc = 0xFFFFFFFF;

    for ( size_t i = 0; i < sizeof( val ); i++ ) {
        uint8_t byte = ( val >> ( i * 8 ) ) & 0xFF;
        crc ^= byte;
        for ( int j = 0; j < 8; j++ ) {
            if ( crc & 1 )
                crc = ( crc >> 1 ) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return crc ^ 0xFFFFFFFF;
}
#endif

#define DOT_PRINT( format, ... ) fprintf( dot_stream, format, ##__VA_ARGS__ );

static void NodeDumpRecursively( const Node_t *node, FILE *dot_stream );
static void NodeInitDot( const Node_t *node, FILE *dot_stream );
static void NodeBondInitDot( const Node_t *node, FILE *dot_stream );

void NodeGraphicDump( const Node_t *node, const char *image_path_name, ... ) {
    if ( !node || !image_path_name ) {
        PRINT_ERROR( "Null pointer on `node` %s:%d \n", __FILE__, __LINE__ );
        return;
    }

    char dot_path[MAX_LEN_PATH] = {};
    char svg_path[MAX_LEN_PATH + 5] = {};

    va_list args;
    va_start( args, image_path_name );
    vsnprintf( dot_path, MAX_LEN_PATH, image_path_name, args );
    va_end( args );

    snprintf( svg_path, MAX_LEN_PATH + 5, "%s.svg", dot_path );

    FILE *dot_stream = fopen( dot_path, "w" );
    assert( dot_stream && "File opening error" );

    fprintf( dot_stream, "digraph {\n\tsplines=line;\n" );
    NodeDumpRecursively( node, dot_stream );
    fprintf( dot_stream, "}\n" );

    fclose( dot_stream );

    char cmd[MAX_LEN_PATH * 3] = {};
    snprintf( cmd, sizeof( cmd ), "dot -Tsvg %s -o %s", dot_path, svg_path );
    system( cmd );
}

static void NodeDumpRecursively( const Node_t *node, FILE *dot_stream ) {
    if ( node == NULL ) {
        return;
    }

    NodeInitDot( node, dot_stream );
    NodeBondInitDot( node, dot_stream );
}

static void NodeInitDot( const Node_t *node, FILE *dot_stream ) {
#ifdef _SIMPLIFIED_DUMP
    DOT_PRINT( "\tnode_%lX [style=filled, ", (uintptr_t)node );
    switch ( node->value.type ) {
        case NODE_NUMBER:
            DOT_PRINT( "fillcolor=\"#5DADE2\", label=\"%d\"]; \n", node->value.data.number );
            break;
        case NODE_VARIABLE:
            DOT_PRINT( "fillcolor=\"#82E0AA\", label=\"`%s`\"]; \n", node->value.data.variable );
            break;
        case NODE_OPERATION:
            DOT_PRINT( "fillcolor=\"#F5B041\", label=\"%s\"]; \n",
                       node->value.data.operation >= 0 ? operations_txt[node->value.data.operation]
                                                       : "NOPE" );
            break;

        case NODE_UNKNOWN:
        default:
            DOT_PRINT( "fillcolor=\"#ff3737b9\", label=\"?\"]; \n" );
            break;
    }
#else
    DOT_PRINT( "\tnode_%lX [shape=plaintext; style=filled; color=black; "
               "fillcolor=\"#%X\"; label=< \n",
               (uintptr_t)node, fill_color );

    DOT_PRINT( "\t<TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\" "
               "ALIGN=\"CENTER\"> \n" );

    DOT_PRINT( "\t\t<TR> \n" );
    DOT_PRINT( "\t\t\t<TD PORT=\"idx\" BGCOLOR=\"#%X\">idx=0x%lX</TD> \n", my_crc32_ptr( node ),
               (uintptr_t)node );
    DOT_PRINT( "\t\t</TR> \n" );

    DOT_PRINT( "\t\t<TR> \n" );
    DOT_PRINT( "\t\t\t<TD PORT=\"parent\" BGCOLOR=\"#%X\">parent=0x%lX</TD> \n", my_crc32_ptr( node->parent ),
               (uintptr_t)node->parent );
    DOT_PRINT( "\t\t</TR> \n" );

    DOT_PRINT( "\t\t<TR> \n" );

    switch ( node->value.type ) {
        case NODE_NUMBER:
            DOT_PRINT( "\t\t\t<TD PORT=\"type\">type=NUMBER</TD> \n" );
            DOT_PRINT( "\t\t</TR> \n\t\t<TR> \n" );
            DOT_PRINT( "\t\t\t<TD PORT=\"value\">value=%d</TD> \n", node->value.data.number );
            break;
        case NODE_VARIABLE:
            DOT_PRINT( "\t\t\t<TD PORT=\"type\">type=VARIABLE</TD> \n" );
            DOT_PRINT( "\t\t</TR> \n\t\t<TR> \n" );
            DOT_PRINT( "\t\t\t<TD PORT=\"value\">value=`%s`</TD> \n", node->value.data.variable );
            break;
        case NODE_OPERATION: {
            DOT_PRINT( "\t\t\t<TD PORT=\"type\">type=OPERATION</TD> \n" );
            DOT_PRINT( "\t\t</TR> \n\t\t<TR> \n" );
            DOT_PRINT( "\t\t\t<TD PORT=\"value\">value=%s</TD> \n",
                       node->value.data.operation >= 0 ? operations_txt[node->value.data.operation]
                                                       : "NOPE" );
            break;
        }
        case NODE_UNKNOWN:
        default:
            DOT_PRINT( "<TD PORT=\"type\" BGCOLOR=\"#FF0000\">type=UNKOWN</TD> \n" );
            break;
    }

    DOT_PRINT( "\t\t</TR> \n" );

    DOT_PRINT( "\t\t<TR> \n" );
    DOT_PRINT( "\t\t\t<TD> \n" );
    DOT_PRINT( "\t\t\t\t<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"2\" "
               "ALIGN=\"CENTER\"> \n" );
    DOT_PRINT( "\t\t\t\t\t<TR> \n" );

    DOT_PRINT( "\t\t\t\t\t\t<TD PORT=\"left\" BGCOLOR=\"#%X\" "
               "ALIGN=\"CENTER\">%lX</TD> \n",
               ( node->left == 0 ? fill_color : my_crc32_ptr( node->left ) ), (uintptr_t)node->left );

    DOT_PRINT( "\t\t\t\t\t\t<TD><FONT POINT-SIZE=\"10\">│</FONT></TD> \n" );

    DOT_PRINT( "\t\t\t\t\t\t<TD PORT=\"right\" BGCOLOR=\"#%X\" "
               "ALIGN=\"CENTER\">%lX</TD> \n",
               ( node->right == 0 ? fill_color : my_crc32_ptr( node->right ) ), (uintptr_t)node->right );

    DOT_PRINT( "\t\t\t\t\t</TR> \n" );
    DOT_PRINT( "\t\t\t\t</TABLE> \n" );
    DOT_PRINT( "\t\t\t</TD> \n" );
    DOT_PRINT( "\t\t</TR> \n" );

    DOT_PRINT( "\t</TABLE> \n" );
    DOT_PRINT( "\t>]; \n" );
#endif
}

static void NodeBondInitDot( const Node_t *node, FILE *dot_stream ) {
#ifdef _SIMPLIFIED_DUMP
    if ( node->left ) {
        DOT_PRINT( "\tnode_%lX -> node_%lX;\n", (uintptr_t)node, (uintptr_t)node->left );
        NodeDumpRecursively( node->left, dot_stream );
    }
    if ( node->right ) {
        DOT_PRINT( "\tnode_%lX -> node_%lX;\n", (uintptr_t)node, (uintptr_t)node->right );
        NodeDumpRecursively( node->right, dot_stream );
    }
#else
    if ( node->left ) {
        DOT_PRINT( "\tnode_%lX:left:s->node_%lX\n", (uintptr_t)node, (uintptr_t)node->left );
        NodeDumpRecursively( node->left, dot_stream );
    }

    if ( node->right ) {
        DOT_PRINT( "\tnode_%lX:right:s->node_%lX\n", (uintptr_t)node, (uintptr_t)node->right );
        NodeDumpRecursively( node->right, dot_stream );
    }
#endif
}

#undef DOT_PRINT

static void NodeSaveRecursively( const Node_t *node, FILE *file_stream ) {
    if ( node == NULL ) {
        fprintf( file_stream, "nil" );
        return;
    }

    fprintf( file_stream, "( " );

    switch ( node->value.type ) {
        case NODE_NUMBER:
            fprintf( file_stream, "%d ", node->value.data.number );
            break;
        case NODE_VARIABLE:
            fprintf( file_stream, "\"%s\" ", node->value.data.variable );
            break;
        case NODE_OPERATION:
            fprintf( file_stream, "%s ", operations_txt[node->value.data.operation] );
            break;
        case NODE_UNKNOWN:
        default:
            fprintf( file_stream, "? " );
            break;
    }

    if ( node->left )
        NodeSaveRecursively( node->left, file_stream );
    else
        fprintf( file_stream, "nil " );

    if ( node->right )
        NodeSaveRecursively( node->right, file_stream );
    else
        fprintf( file_stream, "nil " );
    fprintf( file_stream, ") " );
}

void TreeSaveToFile( const Tree_t *tree, const char *filename ) {
    if ( !tree || !filename ) {
        PRINT_ERROR( "Null pointer on `tree` or `filename`" );
        return;
    }

    FILE *file_stream = fopen( filename, "w" );
    if ( !file_stream ) {
        PRINT_ERROR( "Fail to open file `%s`", filename );
        return;
    }

    NodeSaveRecursively( tree->root, file_stream );

    fclose( file_stream );
}

// ========== TreeLoadFromFile implementation ==========

static void SkipSpaces( const char **pos ) {
    while ( **pos && isspace( (unsigned char)**pos ) ) {
        (*pos)++;
    }
}

static bool MatchString( const char **pos, const char *str ) {
    size_t len = strlen( str );
    if ( strncmp( *pos, str, len ) == 0 ) {
        *pos += len;
        return true;
    }
    return false;
}

static int FindOperationType( const char *str ) {
    for ( int i = 0; i < sizeof(operations_txt) / sizeof(operations_txt[0]); i++ ) {
        if ( strcmp( str, operations_txt[i] ) == 0 ) {
            return i;
        }
    }
    return OP_NOPE;
}

static Node_t* NodeLoadRecursively( const char **pos ) {
    SkipSpaces( pos );

    // Check for 'nil'
    if ( MatchString( pos, "nil" ) ) {
        return NULL;
    }

    // Expect '('
    if ( **pos != '(' ) {
        PRINT_ERROR( "Expected '(' at position, got '%c'", **pos );
        return NULL;
    }
    (*pos)++;

    SkipSpaces( pos );

    // Create node
    TreeData_t data = {};

    // Parse node value
    if ( **pos == '"' ) {
        // Variable (string in quotes)
        (*pos)++; // skip opening quote
        const char *start = *pos;
        while ( **pos && **pos != '"' ) {
            (*pos)++;
        }
        if ( **pos != '"' ) {
            PRINT_ERROR( "Unterminated string" );
            return NULL;
        }
        
        size_t len = *pos - start;
        data.type = NODE_VARIABLE;
        data.data.variable = (char*)calloc( len + 1, sizeof(char) );
        strncpy( data.data.variable, start, len );
        data.data.variable[len] = '\0';
        
        (*pos)++; // skip closing quote
    }
    else if ( isdigit( (unsigned char)**pos ) || **pos == '-' ) {
        // Number
        data.type = NODE_NUMBER;
        char *end;
        data.data.number = strtol( *pos, &end, 10 );
        *pos = end;
    }
    else {
        // Operation (read until space)
        const char *start = *pos;
        while ( **pos && !isspace( (unsigned char)**pos ) && **pos != ')' ) {
            (*pos)++;
        }
        
        size_t len = *pos - start;
        char op_str[64] = {};
        strncpy( op_str, start, len < 63 ? len : 63 );
        
        int op_type = FindOperationType( op_str );
        if ( op_type == OP_NOPE ) {
            PRINT_ERROR( "Unknown operation: %s", op_str );
            return NULL;
        }
        
        data.type = NODE_OPERATION;
        data.data.operation = op_type;
    }

    Node_t *node = NodeCreate( data, NULL );
    if ( !node ) {
        PRINT_ERROR( "Failed to create node" );
        if ( data.type == NODE_VARIABLE && data.data.variable ) {
            free( data.data.variable );
        }
        return NULL;
    }

    SkipSpaces( pos );

    // Parse left child
    node->left = NodeLoadRecursively( pos );
    if ( node->left ) {
        node->left->parent = node;
    }

    SkipSpaces( pos );

    // Parse right child
    node->right = NodeLoadRecursively( pos );
    if ( node->right ) {
        node->right->parent = node;
    }

    SkipSpaces( pos );

    // Expect ')'
    if ( **pos != ')' ) {
        PRINT_ERROR( "Expected ')' at position, got '%c'", **pos );
        NodeDelete( node, NULL, NULL );
        return NULL;
    }
    (*pos)++;

    return node;
}

Tree_t* TreeLoadFromFile( const char *filename ) {
    if ( !filename ) {
        PRINT_ERROR( "Null pointer on `filename`" );
        return NULL;
    }

    char *buffer = ReadToBuffer( filename );
    if ( !buffer ) {
        PRINT_ERROR( "Failed to read file `%s`", filename );
        return NULL;
    }

    Tree_t *tree = TreeCtor();
    if ( !tree ) {
        PRINT_ERROR( "Failed to create tree" );
        free( buffer );
        return NULL;
    }

    const char *pos = buffer;
    tree->root = NodeLoadRecursively( &pos );

    free( buffer );

    if ( !tree->root ) {
        PRINT_ERROR( "Failed to parse tree from file" );
        TreeDtor( &tree, NULL );
        return NULL;
    }

    PRINT( "Successfully loaded tree from file `%s`", filename );
    return tree;
}
