#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DebugUtils.h"
#include "UtilsRW.h"
#include "frontend/Parser.h"

#include "Tree.h"

static void SkipSpaces( const char **pos ) {
    while ( **pos && isspace( (unsigned char)**pos ) ) {
        ( *pos )++;
    }
}

static void SkipComments( const char **pos ) {
    while ( **pos ) {
        SkipSpaces( pos );

        if ( **pos == '/' && ( *pos )[1] == '/' ) {
            while ( **pos && **pos != '\n' ) {
                ( *pos )++;
            }
        } else {
            break;
        }
    }
}

static Node_t *TokenNumber( const char **cur_pos ) {
    my_assert( cur_pos && *cur_pos, "Null pointer on `cur_pos`" );

    TreeData_t value = {};
    value.type = NODE_NUMBER;

    int read_bytes = 0;
    sscanf( *cur_pos, "%d%n", &( value.data.number ), &read_bytes );
    ( *cur_pos ) += read_bytes;

    PRINT( "Number: %d", value.data.number );

    return NodeCreate( value, NULL );
}

static Node_t *TokenVariable( const char **cur_pos ) {
    my_assert( cur_pos && *cur_pos, "Null pointer on `cur_pos`" );

    TreeData_t value = {};
    value.type = NODE_VARIABLE;

    int read_bytes = 0;
    char buffer[128] = {};
    sscanf( *cur_pos, "%127s%n", buffer, &read_bytes );
    value.data.variable = strdup( buffer );
    ( *cur_pos ) += read_bytes;

    PRINT( "Variable: `%s`", value.data.variable );

    return NodeCreate( value, NULL );
    ;
}

static bool MatchOperation( const char *str, OperationType *out_op, size_t *out_len ) {
    bool found = false;

#define CHECK_OP( token_str, op_enum, is_custom, nargs, ... )                                                     \
    if ( !found && strncmp( str, token_str, strlen( token_str ) ) == 0 ) {                                   \
        *out_op = op_enum;                                                                                   \
        *out_len = strlen( token_str );                                                                      \
        found = true;                                                                                        \
        PRINT( "Operation: `%s`", token_str );                                                                 \
    }

    INIT_OPERATIONS( CHECK_OP );
#undef CHECK_OP

    return found;
}

static Node_t *ReadToken( const char **pos ) {
    my_assert( pos && *pos, "Null pointer on `pos`" );

    SkipSpaces( pos );
    SkipComments( pos );

    PRINT( "Cur. position: \n'`%s`", *pos );

    const char *start = *pos;

    OperationType op = OP_NOPE;
    size_t op_len = 0;
    if ( MatchOperation( start, &op, &op_len ) ) {
        *pos = start + op_len;
        TreeData_t data = MakeOperation( op );
        return NodeCreate( data, NULL );
    } else if ( isdigit( (unsigned char)**pos ) || **pos == '.' ) {
        return TokenNumber( pos );
    } else if ( isalpha( (unsigned char)**pos ) ) {
        return TokenVariable( pos );
    }

    PRINT_ERROR( "Lexical error: unexpected character '%c' at: \"%.20s\"\n", **pos, start );
    ( *pos )++;
    return NULL;
}

Node_t **LexicalAnalyze( Parser_t *parser ) {
    my_assert( parser, "Null pointer on `parser`" );

    PRINT( "Start lexical analization" );

    TokenArray_t tokens = TokenArrayCreate();

    char *buffer = ReadToBuffer( parser->input_filename );
    if ( !buffer ) {
        PRINT_ERROR( "Fail to read source from file `%s`", parser->input_filename );
        return NULL;
    }
    PRINT( "Succesful reading to buffer" );

    const char *pos = buffer;
    while ( *pos ) {
        Node_t *token = ReadToken( &pos );
        if ( !token ) {
            TokenArrayDestroy( &tokens );
            return NULL;
        }

        if ( !TokenArrayPushBack( &tokens, token ) ) {
            NodeDelete( token, NULL, NULL );
            TokenArrayDestroy( &tokens );
            return NULL;
        }
    }

    parser->tokens = tokens;

    free( buffer );

    PRINT( "Finish lexixal analization" );

    return tokens.data;
}
