#include <stdio.h>
#include <stdlib.h>

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

    // Создаём генератор кода
    CodeGen_t* codegen = CodeGenCtor( input_file, output_file );
    if ( !codegen ) {
        PRINT_ERROR( "Failed to create code generator" );
        return 1;
    }

    // Загружаем AST из файла
    // TODO: реализовать TreeLoadFromFile
    // Пока предполагаем, что дерево уже загружено
    codegen->tree = TreeCtor();
    if ( !codegen->tree ) {
        PRINT_ERROR( "Failed to create tree" );
        CodeGenDtor( &codegen );
        return 1;
    }

    // Генерируем ассемблерный код
    GenerateCode( codegen );

    PRINT( "Code generation successful" );

    CodeGenDtor( &codegen );
    return 0;
}
