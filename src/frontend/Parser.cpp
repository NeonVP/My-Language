#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "DebugUtils.h"
#include "Tree.h"
#include "UtilsRW.h"
#include "frontend/Parser.h"

static void HelpPrint( const char *program_name, const char *default_input, const char *default_output ) {
    printf( "Usage: %s [-i input_file] [-o output_file]\n", program_name );
    printf( "  -i FILE   input source file (default: %s)\n", default_input );
    printf( "  -o FILE   output tree file (default: %s)\n", default_output );
    printf( "  -h        show this help\n" );
}

static bool ParseArgs( Parser_t *parser, int argc, char **argv ) {
    const char *default_input = "source.lang";
    const char *default_output = "tree.txt";

    parser->input_filename = strdup( default_input );
    parser->output_filename = strdup( default_output );

    optind = 0;

    int opt;
    while ( ( opt = getopt( argc, argv, "i:o:h" ) ) != -1 ) {
        switch ( opt ) {
            case 'i': {
                free( parser->input_filename );
                parser->input_filename = optarg ? strdup( optarg ) : NULL;
                break;
            }
            case 'o': {
                free( parser->output_filename );
                parser->output_filename = optarg ? strdup( optarg ) : NULL;
                break;
            }
            case 'h':
                HelpPrint( argv[0], default_input, default_output );
                return false;
            case '?':
            default:
                return false;
        }
    }

    return true;
}

#ifdef _DEBUG
static void DumpCtor( Log_t *logging ) {
    logging->log_path = strdup( "dump" );

    char buffer[MAX_LEN_PATH] = {};

    snprintf( buffer, MAX_LEN_PATH, "%s/images", logging->log_path );
    logging->img_log_path = strdup( buffer );

    int mkdir_result = MakeDirectory( logging->log_path );
    assert( !mkdir_result && "Error creating directory for dump" );
    mkdir_result = MakeDirectory( logging->img_log_path );
    assert( !mkdir_result && "Error creating directory for dump images" );

    snprintf( buffer, MAX_LEN_PATH, "%s/index.html", logging->log_path );
    logging->log_file = fopen( buffer, "w" );
    assert( logging->log_file && "Error opening file" );

    logging->image_number = 0;
}
#endif

Parser_t *ParserCtor( int argc, char **argv ) {
    my_assert( argv && *argv, "Null pointer on `argv`" );

    Parser_t *parser = (Parser_t *)calloc( 1, sizeof( *parser ) );
    if ( !parser ) {
        PRINT_ERROR( "Memory allocation error" );
        return NULL;
    }

    if ( !ParseArgs( parser, argc, argv ) ) {
        free( parser );
        return NULL;
    }
    PRINT( "Input file  = `%s`", parser->input_filename );
    PRINT( "Output file = `%s`", parser->output_filename );

    ON_DEBUG( DumpCtor( &( parser->logging ) ) );

    return parser;
}

#ifdef _DEBUG
static void DumpDtor( Log_t *logging ) {
    my_assert( logging, "Null pointer on `logging" );

    int fclose_result = fclose( logging->log_file );
    if ( fclose_result ) {
        PRINT_ERROR( "Fail to close file with logs \n" );
    }

    free( logging->log_path );
    free( logging->img_log_path );
}
#endif

void ParserDtor( Parser_t **parser ) {
    my_assert( parser && *parser, "Null pointer on `parse`" );

    ON_DEBUG( DumpDtor( &( ( *parser )->logging ) ); )
    TokenArrayDestroy( &( ( *parser )->tokens ) );

    free( ( *parser )->tree );
    free( ( *parser )->input_filename );
    free( ( *parser )->output_filename );

    free( *parser );
    *parser = NULL;
}

void Parse( Parser_t *parser ) {
    my_assert( parser, "Null pointer on `parser`" );

    if ( !LexicalAnalyze( parser ) ) {
        PRINT_ERROR( "Lexical analysis failed" );
        return;
    }

    Node_t *root = SyntaxAnalyze( parser );

    parser->tree = TreeCtor();
    parser->tree->root = root;

    PRINT( "root = %p", parser->tree->root );
    ParserDump( parser, "After pasring my code" );
}

#ifdef _DEBUG
#define PRINT_HTML( fmt, ... ) fprintf( parser->logging.log_file, fmt, ##__VA_ARGS__ );

static void PrintCustomMessage( Parser_t *parser, const char *format_string, va_list args ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( format_string, "Null pointer on `format_string`" );

    PRINT_HTML( "<pre>" );
    vfprintf( parser->logging.log_file, format_string, args );
    PRINT_HTML( "</pre>\n" );
}

void ParserDump( Parser_t *parser, const char *format_string, ... ) {
    my_assert( parser, "Null pointer for `parser`" );

    PRINT_HTML( "<h3>DUMP</h3>\n" );

    if ( format_string ) {
        va_list args;
        va_start( args, format_string );
        PrintCustomMessage( parser, format_string, args );
        va_end( args );
    }

    if ( parser->tree ) {
        NodeGraphicDump( parser->tree->root, "%s/image%lu.dot", parser->logging.img_log_path,
                         parser->logging.image_number );
        PRINT_HTML( "<img src=\"images/image%lu.dot.svg\" style=\"width:auto; height:400;\">\n",
                    parser->logging.image_number++ );
    }

    fflush( parser->logging.log_file );

    PRINT( "Succesful Dump" );
}

#undef PRINT_HTML
#endif
