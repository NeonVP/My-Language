#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "frontend/Frontend.h"
#include "DebugUtils.h"


static void TreeSaveNode( const Node_t *node, FILE *stream, bool *error ) {
    if ( !node ) {
        fprintf( stream, "nil" );
        return;
    }

    fprintf( stream, "( " );

    switch ( node->value.type ) {
        case NODE_NUMBER:
            fprintf( stream, "%lg ", node->value.data.number );
            break;
        case NODE_VARIABLE:
            fprintf( stream, "%c ", node->value.data.variable );
            break;
        case NODE_OPERATION:
            fprintf( stream, "%s ", operations_txt[node->value.data.operation] );
            break;

        case NODE_UNKNOWN:
        default:
            *error = true;
            return;
    }

    TreeSaveNode( node->left, stream, error );
    if ( *error )
        return;

    fprintf( stream, " " );

    TreeSaveNode( node->right, stream, error );
    if ( *error )
        return;

    fprintf( stream, " )" );
}

void TreeSaveToFile( const Tree_t *tree, const char *filename ) {
    my_assert( tree, "Null pointer on tree" );
    my_assert( filename, "Null pointer on filename" );

    FILE *file_with_base = fopen( filename, "w" );
    my_assert( file_with_base, "Failed to open file for writing tree" );

    bool error = false;
    TreeSaveNode( tree->root, file_with_base, &error );
    if ( error )
        PRINT_ERROR( "Error writing a tree to a file!!!\n" );

    int result = fclose( file_with_base );
    assert( !result && "Error while closing file with tree" );

    if ( !error )
        fprintf( stdout, "The tree was saved in %s\n", filename );
}

static void CleanSpace( char **position ) {
    while ( isspace( **position ) )
        ( *position )++;
}

#define OP_IS_IT( str, name, ... )                                                                           \
    if ( strncmp( *current_position, str, strlen( str ) ) == 0 ) {                                           \
        read_bytes = strlen( str );                                                                          \
        PRINT( "Parse operation: %s \n", str );                                                              \
        return name;                                                                                         \
    }

static OperationType IsItOperation( char **current_position ) {
    CleanSpace( current_position );

    int read_bytes = 0;

    INIT_OPERATIONS( OP_IS_IT )

    return OP_NOPE;
}

#undef OP_IS_IT

static TreeData_t NodeParseValue( char **current_position ) {
    my_assert( current_position, "Null pointer on `current position`" );

    TreeData_t value = {};
    CleanSpace( current_position );

    int read_bytes = 0;
    if ( sscanf( *current_position, " %lf%n", &( value.data.number ), &read_bytes ) >= 1 ) {
        PRINT( "Parse number: %lg \n", value.data.number );
        value.type = NODE_NUMBER;
        ( *current_position ) += read_bytes;
        return value;
    }
    PRINT( "It's not a number \n" );

    OperationType op = IsItOperation( current_position );
    if ( op != OP_NOPE ) {
        value.type = NODE_OPERATION;
        value.data.operation = op;
        return value;
    }
    PRINT( "It's not an operation \n" );

    if ( isalpha( **current_position ) ) {
        PRINT( "Parse variable: %c \n", **current_position )
        value.type = NODE_VARIABLE;
        value.data.variable = **current_position;
        ( *current_position )++;
        return value;
    }
    PRINT( "It's not a variable \n" );

    value.type = NODE_UNKNOWN;
    PRINT( COLOR_BRIGHT_RED "[ERROR] It's unclear what \n" );
    return value;
}

#ifdef _DEBUG
#define INTERMIDIATE_DUMP( node, ... )                                                                       \
    Tree_t temp_tree = {};                                                                                   \
    temp_tree.root = node;                                                                                   \
    temp_tree.current_position = tree->current_position;                                                     \
    temp_tree.logging.log_file = tree->logging.log_file;                                                     \
    temp_tree.logging.img_log_path = tree->logging.img_log_path;                                             \
    temp_tree.image_number = tree->image_number++;                                                           \
    TreeDump( &temp_tree, ##__VA_ARGS__ );                                                                   \
    tree->logging.log_file = temp_tree.logging.log_file;
#else
#define INTEDMIDIATE_DUMP( ... )
#endif

static Node_t *TreeParseNode( char **current_position, bool *error, Node_t *parent ) {
    PRINT( "\n" );
    PRINT( "Сurrent position status: `%s` \n", *current_position );

    CleanSpace( current_position );

    if ( **current_position == '(' ) {
        ( *current_position )++;

        CleanSpace( current_position );
        TreeData_t value = NodeParseValue( current_position );
        if ( value.type == NODE_UNKNOWN ) {
            *error = true;
            return NULL;
        }

        Node_t *node = NodeCreate( value, parent );
        if ( !node ) {
            *error = true;
            return NULL;
        }

        CleanSpace( current_position );
        node->left = TreeParseNode( current_position, error, node );
        if ( *error ) {
            NodeDelete( node, NULL, NULL );
            return NULL;
        }
        if ( node->left )
            node->left->parent = node;

        CleanSpace( current_position );
        node->right = TreeParseNode( current_position, error, node );
        if ( *error ) {
            NodeDelete( node, NULL, NULL );
            return NULL;
        }
        if ( node->right )
            node->right->parent = node;

        CleanSpace( current_position );
        if ( **current_position != ')' ) {
            *error = true;
            return NULL;
        }
        ( *current_position )++;

        PRINT( "Сurrent position status after parsing new node: `%s` \n", *current_position );
        // INTERMIDIATE_DUMP( node, "After parsing new node" );

        return node;
    } else if ( strncmp( *current_position, "nil", 3 ) == 0 ) {
        *current_position += 3;
        PRINT( "Сurrent position status after parsing new node: `%s` \n", *current_position );
        return NULL;
    } else {
        *error = true;
        return NULL;
    }
}

Tree_t *TreeReadFromBuffer( char *buffer ) {
    my_assert( buffer, "Null pointer on `buffer`" );

    Tree_t *tree = TreeCtor();

    char *current_position = buffer;
    bool error = false;
    tree->root = TreeParseNode( &current_position, &error, NULL );

    if ( error ) {
        PRINT_ERROR( "База не считалась корректно. \n" );
        return NULL;
    } else {
        PRINT( "База считалась корректно. \n" );
        return tree;
    }
}