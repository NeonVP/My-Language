#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    // Загружаем AST из файла с подробным сообщением об ошибке
    char error_buffer[256] = {};
    codegen->tree = TreeLoadFromFile( input_file, error_buffer, sizeof(error_buffer) );
    if ( !codegen->tree ) {
        if ( error_buffer[0] != '\0' ) {
            PRINT_ERROR( "Failed to load AST from file '%s': %s", input_file, error_buffer );
        } else {
            PRINT_ERROR( "Failed to load AST from file: %s", input_file );
        }
        CodeGenDtor( &codegen );
        return 1;
    }

    PRINT( "AST loaded successfully" );

    // Генерируем ассемблерный код
    GenerateCode( codegen );

    PRINT( "Code generation successful" );

    CodeGenDtor( &codegen );
    return 0;
}
