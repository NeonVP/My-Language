#ifndef CODEGEN_H
#define CODEGEN_H

#include "Tree.h"
#include <stdio.h>

struct CodeGen_t {
    Tree_t* tree;
    FILE* output;
    
    char* input_filename;
    char* output_filename;
    
    int label_counter;
    int temp_var_counter;
};

CodeGen_t* CodeGenCtor( const char* input_file, const char* output_file );
void CodeGenDtor( CodeGen_t** codegen );

void GenerateCode( CodeGen_t* codegen );

#endif // CODEGEN_H
