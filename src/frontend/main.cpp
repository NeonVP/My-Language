#include "frontend/Parser.h"

int main( int argc, char **argv ) {
    Parser_t *parser = ParserCtor( argc, argv );

    Parse( parser );

    ParserDtor( &parser );
    return 0;
}