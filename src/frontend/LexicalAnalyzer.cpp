#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "DebugUtils.h"
#include "frontend/Parser.h"

#include "Tree.h"

static void SkipSpaces( const char **pos ) {
    while ( **pos ) {
        if ( isspace( (unsigned char)**pos ) ) {
            ( *pos )++;
        } else if ( **pos == '/' && ( *pos )[1] == '/' ) {
            while ( **pos && **pos != '\n' )
                ( *pos )++;
        } else {
            break;
        }
    }
}

static bool MatchOperation( const char *str, OperationType *out_op, size_t *out_len ) {
    bool found = false;

#define CHECK_OP( token_str, op_enum, is_custom, nargs )                                                     \
    if ( !found && strncmp( str, token_str, strlen( token_str ) ) == 0 ) {                                   \
        *out_op = op_enum;                                                                                   \
        *out_len = strlen( token_str );                                                                      \
        found = true;                                                                                        \
    }

    INIT_OPERATIONS( CHECK_OP );
#undef CHECK_OP

    return found;
}

static Node_t *ReadToken( const char **pos ) {
    SkipSpaces( pos );
    if ( !**pos )
        return NULL;

    const char *start = *pos;

    OperationType op = OP_NOPE;
    size_t op_len = 0;
    if ( MatchOperation( start, &op, &op_len ) ) {
        *pos = start + op_len;
        TreeData_t data = MakeOperation( op );
        return NodeCreate( data, NULL );
    }

    if ( isdigit( (unsigned char)**pos ) || **pos == '.' ) {
        char *end = NULL;
        double num = strtod( start, &end );
        if ( end != start ) {
            *pos = end;
            return NodeCreate( MakeNumber( num ), NULL );
        }
    }

    if ( isalpha( (unsigned char)**pos ) ) {
        char var = **pos;
        ( *pos )++;
        return NodeCreate( MakeVariable( var ), NULL );
    }

    PRINT_ERROR( "Lexical error: unexpected character '%c' at: \"%.20s\"\n", **pos, start );
    ( *pos )++;
    return NULL;
}

Node_t **LexicalAnalyze( Parser_t *parser ) {
    if ( !parser || !parser->buffer ) {
        return NULL;
    }

    size_t token_count = 0;
    size_t capacity = 16;
    Node_t **tokens = (Node_t **)calloc( capacity, sizeof( Node_t * ) );
    if ( !tokens ) {
        return NULL;
    }

    const char *pos = parser->buffer;
    while ( *pos ) {
        Node_t *token = ReadToken( &pos );
        if ( !token ) {
            for ( size_t i = 0; i < token_count; i++ ) {
                if ( tokens[i] )
                    NodeDelete( tokens[i], NULL, NULL );
            }
            free( tokens );
            return NULL;
        }

        if ( token_count >= capacity ) {
            capacity *= 2;
            Node_t **new_tokens = (Node_t **)realloc( tokens, capacity * sizeof( Node_t * ) );
            if ( !new_tokens ) {
                for ( size_t i = 0; i < token_count; i++ ) {
                    if ( tokens[i] )
                        NodeDelete( tokens[i], NULL, NULL );
                }
                free( tokens );
                return NULL;
            }
            tokens = new_tokens;
        }

        tokens[token_count++] = token;
    }

    // Store tokens in parser structure
    parser->tokens.data = tokens;
    parser->tokens.size = token_count;
    parser->tokens.capacity = capacity;

    return tokens;
}

void FreeTokenArray( Node_t **tokens, size_t count ) {
    if ( !tokens )
        return;
    for ( size_t i = 0; i < count; i++ ) {
        if ( tokens[i] ) {
            NodeDelete( tokens[i], NULL, NULL );
        }
    }
    free( tokens );
}