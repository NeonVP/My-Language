#include "frontend/Parser.h"

int main( int argc, char** argv ) {
    Parser_t* parser = ParserCtor( argc, argv);
    
    ParserDtor( &parser );

    return 0;
}