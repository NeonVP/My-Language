#ifndef FRONTEND_H
#define FRONTEND_H

#include "Tree.h"

struct TokenArray_t {
    Node_t** data;

    size_t size;
    size_t capacity;
};

#ifdef _DEBUG
struct Log_t {
    FILE *log_file;
    char *log_path;
    char *img_log_path;
    size_t image_number;
};
#endif

// TODO: add ErrorList in Parser_t
struct Parser_t {
    TokenArray_t tokens;

    char*  buffer;

    Tree_t* tree;


    char* input_filename;
    char* output_filename;  

#ifdef _DEBUG
    struct Log_t logging;
#endif
};

Parser_t *ParserCtor( int argc, char** argv );
void ParserDtor( Parser_t** parser );

void Parse( Parser_t* parser );

// Lexical analyzer
Node_t **LexicalAnalyze( Parser_t* parser );
void FreeTokenArray( Node_t **tokens, size_t count );

// Syntax analyzer
// Node_t **SyntaxAnalyze()

#ifdef _DEBUG
void ParserDump( Parser_t *parser, const char *format_string, ... );
#endif

void TreeSaveToFile( const Tree_t *tree, const char *filename );
Tree_t *TreeReadFromBuffer( char *buffer );

TreeData_t MakeNumber( double number );
TreeData_t MakeOperation( OperationType operation );
TreeData_t MakeVariable( char variable );
Node_t *MakeNode( OperationType op, Node_t *L, Node_t *R );

int CompareDoubleToDouble( double a, double b, double eps );

#endif