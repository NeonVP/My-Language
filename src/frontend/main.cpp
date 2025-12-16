#include "DebugUtils.h"
#include "frontend/Parser.h"

int main( int argc, char **argv ) {
    Parser_t *parser = ParserCtor( argc, argv );
    LexicalAnalyze( parser );

    

    ParserDtor( &parser );

    return 0;
}