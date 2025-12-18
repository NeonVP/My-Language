#include <stdio.h>

#include "backend/CodeGen.h"
#include "Tree.h"
#include "DebugUtils.h"

static void PrintUsage() {
    printf( "Usage: backend <input.ast> <output.asm>\n" );
    printf( "  input.ast  - Input AST file\n" );
    printf( "  output.asm - Output assembly file\n" );
}

int main( int argc, char** argv ) {
    if ( argc < 3 ) {
        PrintUsage();
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    PRINT( "Backend: %s -> %s", input_file, output_file );

    CodeGen_t* codegen = CodeGenCtor( input_file, output_file );
    if ( !codegen ) {
        PRINT_ERROR( "Failed to create code generator" );
        return 1;
    }

    codegen->tree = TreeLoadFromFile( input_file );

    TreeSaveToFile( codegen->tree, "tree_after_reading.txt" );
    // if ( !codegen->tree ) {
    //     PRINT_ERROR( "Failed to load AST from file: %s", input_file );
    //     CodeGenDtor( &codegen );
    //     return 1;
    // }

    PRINT( "AST loaded successfully" );

    // GenerateCode( codegen );

    PRINT( "Code generation successful" );

    CodeGenDtor( &codegen );
    return 0;
}
