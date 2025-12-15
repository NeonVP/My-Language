#ifndef OPERATION_H
#define OPERATION_H

enum IsCustomOp {
    PseudoOp    = -1,
    NonCustomOp =  0,
    CustomOp    =  1
};

enum NumberOfParams {
    ZERO_ARG = 0,
    ONE_ARG  = 1,
    TWO_ARGS = 2
};

#define INIT_OPERATIONS( macros ) \
    macros( ":=",      OP_ADVERT,       NonCustomOp, 0 ) \
    macros( "=",       OP_ASSIGN,       NonCustomOp, 0 ) \
    macros( "+",       OP_ADD,          NonCustomOp, 0 ) \
    macros( "-",       OP_SUB,          NonCustomOp, 0 ) \
    macros( "*",       OP_MUL,          NonCustomOp, 0 ) \
    macros( "/",       OP_DIV,          NonCustomOp, 0 ) \
    macros( "^",       OP_POW,          NonCustomOp, 0 ) \
    macros( "input",   OP_IN,           NonCustomOp, 0 ) \
    macros( "output",  OP_OUT,          CustomOp,    1 ) \
    macros( "if",      OP_IF,           CustomOp,    1 ) \
    macros( "else",    OP_ELSE,         NonCustomOp, 0 ) \
    macros( "while",   OP_WHILE,        CustomOp,    1 ) \
    macros( "func",    OP_FUNC,         NonCustomOp, 0 ) \
    macros( "call",    OP_CALL,         NonCustomOp, 0 ) \
    macros( "(",       OP_OPEN_PARENT,  PseudoOp,    0 ) \
    macros( ")",       OP_CLOSE_PARENT, PseudoOp,    0 ) \
    macros( "{",       OP_OPEN_BRACE,   PseudoOp,    0 ) \
    macros( "}",       OP_CLOSE_BRACE,  PseudoOp,    0 ) \
    macros( ";",       OP_SEMICOLON,    PseudoOp,    0 )

    
#endif