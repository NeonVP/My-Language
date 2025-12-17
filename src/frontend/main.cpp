#include "Tree.h"
#include "frontend/Parser.h"

int main( int argc, char **argv ) {
    Parser_t *parser = ParserCtor( argc, argv );

    Parse( parser );
    TreeSaveToFile( parser->tree, parser->output_filename );

    ParserDtor( &parser );
    return 0;
}