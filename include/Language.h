#ifndef LANGUAGE_H
#define LANGUAGE_H

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
    macros( ":=",      OP_ADVERT,       NonCustomOp, 0, ":="     ) \
    macros( "=",       OP_ASSIGN,       NonCustomOp, 0, "="      ) \
    macros( "+",       OP_ADD,          NonCustomOp, 0, "+"      ) \
    macros( "-",       OP_SUB,          NonCustomOp, 0, "-"      ) \
    macros( "*",       OP_MUL,          NonCustomOp, 0, "*"      ) \
    macros( "/",       OP_DIV,          NonCustomOp, 0, "/"      ) \
    macros( "^",       OP_POW,          NonCustomOp, 0, "^"      ) \
    macros( "sqrt",    OP_SQRT,         NonCustomOp, 0, "sqrt"   ) \
    macros( "input",   OP_IN,           NonCustomOp, 0, "input"  ) \
    macros( "print",   OP_OUT,          CustomOp,    1, "print"  ) \
    macros( "if",      OP_IF,           CustomOp,    1, "if"     ) \
    macros( "else",    OP_ELSE,         NonCustomOp, 0, "else"   ) \
    macros( "while",   OP_WHILE,        CustomOp,    1, "while"  ) \
    macros( "func",    OP_FUNC,         NonCustomOp, 0, "func"   ) \
    macros( "call",    OP_CALL,         NonCustomOp, 0, "call"   ) \
    macros( "return",  OP_RETURN,       NonCustomOp, 0, "return" ) \
    macros( "main",    OP_MAIN,         NonCustomOp, 0, "main"   ) \
    macros( ",",       OP_COMMA,        PseudoOp,    0, ","      ) \
    macros( ";",       OP_SEMICOLON,    PseudoOp,    0, ";"      ) \
    macros( "(",       OP_OPEN_PARENT,  PseudoOp,    0, "("      ) \
    macros( ")",       OP_CLOSE_PARENT, PseudoOp,    0, ")"      ) \
    macros( "{",       OP_OPEN_BRACE,   PseudoOp,    0, "{"      ) \
    macros( "}",       OP_CLOSE_BRACE,  PseudoOp,    0, "}"      )

#endif
