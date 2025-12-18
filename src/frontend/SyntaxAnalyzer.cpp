#include <stdlib.h>
#include <string.h>

#include "DebugUtils.h"
#include "Tree.h"
#include "UtilsRW.h"
#include "frontend/Parser.h"

// ===== Grammar (РБНФ / EBNF-ish) =====
// Grammar     ::= Program
// Program     ::= FunctionList
// FunctionList ::= Function { Function }
// Function    ::= "main" "(" ")" Block
//              | "func" Variable "(" ParamList? ")" Block
// ParamList   ::= Variable { "," Variable }
// OPSeq       ::= OP { OP }                // OP+
// OP          ::= Block
//              | Assignment ";"
//              | Expression ";"
//              | IfStmt
//              | WhileStmt
//              | ReturnStmt ";"
// Block       ::= "{" OPSeq? "}" [ ";" ]
// Assignment  ::= Variable ( ":=" | "=" ) Expression
// ReturnStmt  ::= "return" Expression
// IfStmt      ::= "if" "(" Expression ")" OP [ "else" OP ] [ ";" ]
// WhileStmt   ::= "while" "(" Expression ")" OP [ ";" ]
// Expression  ::= Term { ("+" | "-") Term }*
// Term        ::= Pow { ("*" | "/") Pow }*
// Pow         ::= Unary { "^" Unary }
// Unary       ::= "sqrt" "(" Expression ")"
//              | Primary
// Primary     ::= Number
//              | "input" "(" ")"
//              | "print" "(" Expression ")"
//              | "call" Variable "(" ArgList? ")"
//              | Variable
//              | "(" Expression ")"
// ArgList     ::= Expression { "," Expression }

static bool AtEnd( Parser_t *parser, size_t index ) { return index >= parser->tokens.size; }

static bool MatchToken( Parser_t *parser, size_t index, OperationType op ) {
    if ( AtEnd( parser, index ) )
        return false;
    Node_t *tok = parser->tokens.data[index];
    return tok->value.type == NODE_OPERATION && (OperationType)tok->value.data.operation == op;
}

static bool MatchVariable( Parser_t *parser, size_t index ) {
    if ( AtEnd( parser, index ) )
        return false;
    Node_t *tok = parser->tokens.data[index];
    return tok->value.type == NODE_VARIABLE;
}

static Node_t *GetGrammar( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetProgram( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetFunction( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetParamList( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetOPSeq( Parser_t *parser, size_t *index, bool *error, OperationType stop_op );
static Node_t *GetOP( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetAssignment( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetReturnStmt( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetIfStmt( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetWhileStmt( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetExpression( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetTerm( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetPow( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetUnary( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetPrimary( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetBlock( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetArgList( Parser_t *parser, size_t *index, bool *error );

Node_t *SyntaxAnalyze( Parser_t *parser ) {
    my_assert( parser, "Null pointer on `parser`" );

    PRINT( "Start syntax analysis" );

    bool error = false;
    size_t index = 0;
    Node_t *node = GetGrammar( parser, &index, &error );

    if ( error ) {
        PRINT_ERROR( "SyntaxAnalyze failed near token %zu/%zu", index, parser->tokens.size );
        if ( !AtEnd( parser, index ) ) {
            Node_t *tok = parser->tokens.data[index];
            if ( tok->value.type == NODE_OPERATION )
                PRINT_ERROR( "Current token: OP %d", tok->value.data.operation );
            else if ( tok->value.type == NODE_NUMBER )
                PRINT_ERROR( "Current token: NUMBER %d", tok->value.data.number );
            else if ( tok->value.type == NODE_VARIABLE )
                PRINT_ERROR( "Current token: VAR %s", tok->value.data.variable ? tok->value.data.variable : "(null)" );
            else
                PRINT_ERROR( "Current token: UNKNOWN" );
        }

        PRINT_ERROR( "The expression was not considered correct." );
        TreeDtor( &( parser->tree ), NULL );
        return NULL;
    }

    if ( !AtEnd( parser, index ) ) {
        PRINT_ERROR( "Unexpected tokens at end of input" );
        return NULL;
    }

    PRINT( "The program was considered correct." );
    return node;
}

#define SyntaxError( msg )                                                                                     \
    do {                                                                                                       \
        PRINT_ERROR( "Syntax error: %s at token %zu", msg, *index );                                           \
        *error = true;                                                                                         \
        return NULL;                                                                                           \
    } while ( 0 )

static Node_t *GetGrammar( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    return GetProgram( parser, index, error );
}

static Node_t *GetProgram( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    Node_t *head = GetFunction( parser, index, error );
    if ( *error || !head )
        return head;

    while ( !AtEnd( parser, *index ) ) {
        if ( MatchToken( parser, *index, OP_FUNC ) || MatchToken( parser, *index, OP_MAIN ) ) {
            Node_t *next = GetFunction( parser, index, error );
            if ( *error )
                return NULL;

            // Соединяем функции через ;
            TreeData_t semicolon_data;
            semicolon_data.type = NODE_OPERATION;
            semicolon_data.data.operation = OP_SEMICOLON;
            Node_t *semicolon = NodeCreate( semicolon_data, NULL );

            semicolon->left = head;
            semicolon->right = next;
            if ( head )
                head->parent = semicolon;
            if ( next )
                next->parent = semicolon;

            head = semicolon;
        } else {
            break;
        }
    }

    return head;
}

static Node_t *GetFunction( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    Node_t *func_token = NULL;
    Node_t *func_name = NULL;

    if ( MatchToken( parser, *index, OP_MAIN ) ) {
        func_token = parser->tokens.data[*index];
        (*index)++;

        // main не имеет явного имени в синтаксисе, но в AST нужно ("main" nil nil)
        TreeData_t name_data;
        name_data.type = NODE_VARIABLE;
        name_data.data.variable = strdup("main");
        func_name = NodeCreate( name_data, NULL );
    } else if ( MatchToken( parser, *index, OP_FUNC ) ) {
        func_token = parser->tokens.data[*index];
        (*index)++;

        if ( !MatchVariable( parser, *index ) )
            SyntaxError( "Expected function name after 'func'" );

        func_name = parser->tokens.data[*index];
        (*index)++;
    } else {
        SyntaxError( "Expected 'func' or 'main'" );
    }

    if ( !MatchToken( parser, *index, OP_OPEN_PARENT ) )
        SyntaxError( "Expected '(' after function name" );
    (*index)++;

    Node_t *params = NULL;
    if ( !MatchToken( parser, *index, OP_CLOSE_PARENT ) ) {
        params = GetParamList( parser, index, error );
        if ( *error )
            return NULL;
    }

    if ( !MatchToken( parser, *index, OP_CLOSE_PARENT ) )
        SyntaxError( "Expected ')' after parameters" );
    (*index)++;

    Node_t *body = GetBlock( parser, index, error );
    if ( *error )
        return NULL;

    // Создаём узел с параметрами: (, func_name params)
    TreeData_t comma_data;
    comma_data.type = NODE_OPERATION;
    comma_data.data.operation = OP_COMMA;
    Node_t *comma_node = NodeCreate( comma_data, NULL );

    comma_node->left = func_name;
    comma_node->right = params;
    if ( func_name )
        func_name->parent = comma_node;
    if ( params )
        params->parent = comma_node;

    // func/main узел
    func_token->left = comma_node;
    func_token->right = body;
    if ( comma_node )
        comma_node->parent = func_token;
    if ( body )
        body->parent = func_token;

    return func_token;
}

static Node_t *GetParamList( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    if ( !MatchVariable( parser, *index ) )
        SyntaxError( "Expected variable in parameter list" );

    Node_t *head = parser->tokens.data[*index];
    (*index)++;

    while ( MatchToken( parser, *index, OP_COMMA ) ) {
        Node_t *comma_token = parser->tokens.data[*index];
        (*index)++;

        if ( !MatchVariable( parser, *index ) )
            SyntaxError( "Expected variable after ','" );

        Node_t *next_param = parser->tokens.data[*index];
        (*index)++;

        comma_token->left = head;
        comma_token->right = next_param;
        if ( head )
            head->parent = comma_token;
        if ( next_param )
            next_param->parent = comma_token;

        head = comma_token;
    }

    return head;
}

static Node_t *ConsumeOp( Parser_t *parser, size_t *index, bool *error, OperationType op, const char *msg ) {
    if ( !MatchToken( parser, *index, op ) ) {
        SyntaxError( msg );
    }
    Node_t *tok = parser->tokens.data[*index];
    ( *index )++;
    return tok;
}

static bool IsStatementStart( Parser_t *parser, size_t index ) {
    if ( AtEnd( parser, index ) )
        return false;

    Node_t *tok = parser->tokens.data[index];
    if ( tok->value.type == NODE_VARIABLE || tok->value.type == NODE_NUMBER )
        return true;

    if ( tok->value.type == NODE_OPERATION ) {
        OperationType op = (OperationType)tok->value.data.operation;
        return op == OP_OPEN_BRACE || op == OP_IF || op == OP_WHILE || 
               op == OP_OPEN_PARENT || op == OP_RETURN || op == OP_IN || op == OP_OUT;
    }

    return false;
}

static Node_t *GetOPSeq( Parser_t *parser, size_t *index, bool *error, OperationType stop_op ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    Node_t *head = GetOP( parser, index, error );
    if ( *error || !head )
        return head;

    while ( !AtEnd( parser, *index ) && !( stop_op != OP_NOPE && MatchToken( parser, *index, stop_op ) ) ) {
        if ( MatchToken( parser, *index, OP_SEMICOLON ) ) {
            Node_t *semicolon = ConsumeOp( parser, index, error, OP_SEMICOLON, "Expected ';'" );
            if ( *error )
                return NULL;

            bool is_first_wrap = ( head->value.type != NODE_OPERATION || (OperationType)head->value.data.operation != OP_SEMICOLON );
            
            if ( is_first_wrap ) {
                semicolon->left = head;
                semicolon->right = NULL;
                if ( head ) head->parent = semicolon;
                head = semicolon;
            } else {
                semicolon->left = head;
                if ( head ) head->parent = semicolon;
                head = semicolon;

                if ( IsStatementStart( parser, *index ) ) {
                    Node_t *next = GetOP( parser, index, error );
                    semicolon->right = next;
                    if ( next ) next->parent = semicolon;
                } else {
                    semicolon->right = NULL;
                }
            }
        } else if ( IsStatementStart( parser, *index ) ) {
            Node_t *next = GetOP( parser, index, error );
            if ( *error ) return NULL;

            if ( head->value.type == NODE_OPERATION && (OperationType)head->value.data.operation == OP_SEMICOLON ) {
                if ( head->right == NULL ) {
                    head->right = next;
                    if ( next ) next->parent = head;
                } else {
                    head = next; 
                }
            } else {
                head = next;
            }
        } else {
            break;
        }
    }

    return head;
}

static Node_t *GetOP( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    if ( AtEnd( parser, *index ) ) {
        return NULL;
    }

    if ( MatchToken( parser, *index, OP_OPEN_BRACE ) ) {
        return GetBlock( parser, index, error );
    }

    if ( MatchToken( parser, *index, OP_WHILE ) ) {
        return GetWhileStmt( parser, index, error );
    }

    if ( MatchToken( parser, *index, OP_IF ) ) {
        return GetIfStmt( parser, index, error );
    }

    if ( MatchToken( parser, *index, OP_RETURN ) ) {
        return GetReturnStmt( parser, index, error );
    }

    Node_t *stmt = NULL;
    if ( MatchVariable( parser, *index ) && !AtEnd( parser, *index + 1 ) &&
         ( MatchToken( parser, *index + 1, OP_ADVERT ) || MatchToken( parser, *index + 1, OP_ASSIGN ) ) ) {
        stmt = GetAssignment( parser, index, error );
    } else {
        stmt = GetExpression( parser, index, error );
    }

    return stmt;
}

static Node_t *GetAssignment( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    if ( !MatchVariable( parser, *index ) )
        SyntaxError( "Expected variable at assignment start" );

    Node_t *var = parser->tokens.data[*index];
    ( *index )++;

    if ( !( MatchToken( parser, *index, OP_ADVERT ) || MatchToken( parser, *index, OP_ASSIGN ) ) )
        SyntaxError( "Expected ':=' or '=' in assignment" );

    Node_t *assign_op = parser->tokens.data[*index];
    ( *index )++;

    Node_t *expr = GetExpression( parser, index, error );
    if ( *error )
        return NULL;

    assign_op->left = var;
    assign_op->right = expr;

    if ( var )
        var->parent = assign_op;
    if ( expr )
        expr->parent = assign_op;

    return assign_op;
}

static Node_t *GetReturnStmt( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    Node_t *return_token = ConsumeOp( parser, index, error, OP_RETURN, "Expected 'return'" );
    if ( *error )
        return NULL;

    Node_t *expr = GetExpression( parser, index, error );
    if ( *error )
        return NULL;

    return_token->left = expr;
    return_token->right = NULL;
    if ( expr )
        expr->parent = return_token;

    return return_token;
}

static Node_t *GetWhileStmt( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    Node_t *while_token = ConsumeOp( parser, index, error, OP_WHILE, "Expected 'while'" );
    if ( *error )
        return NULL;

    ConsumeOp( parser, index, error, OP_OPEN_PARENT, "Expected '(' after while" );
    if ( *error )
        return NULL;

    Node_t *condition = GetExpression( parser, index, error );
    if ( *error )
        return NULL;

    ConsumeOp( parser, index, error, OP_CLOSE_PARENT, "Expected ')' after while condition" );
    if ( *error )
        return NULL;

    Node_t *body = GetOP( parser, index, error );
    if ( *error )
        return NULL;
    if ( !body )
        SyntaxError( "Expected while body" );

    while_token->left = condition;
    while_token->right = body;
    if ( condition )
        condition->parent = while_token;
    if ( body )
        body->parent = while_token;

    return while_token;
}

static Node_t *GetIfStmt( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    Node_t *if_token = ConsumeOp( parser, index, error, OP_IF, "Expected 'if'" );
    if ( *error )
        return NULL;

    ConsumeOp( parser, index, error, OP_OPEN_PARENT, "Expected '(' after if" );
    if ( *error )
        return NULL;

    Node_t *condition = GetExpression( parser, index, error );
    if ( *error )
        return NULL;

    ConsumeOp( parser, index, error, OP_CLOSE_PARENT, "Expected ')' after if condition" );
    if ( *error )
        return NULL;

    Node_t *then_stmt = GetOP( parser, index, error );
    if ( *error )
        return NULL;
    if ( !then_stmt )
        SyntaxError( "Expected statement after if" );

    Node_t *else_token = NULL;
    Node_t *else_stmt = NULL;

    if ( MatchToken( parser, *index, OP_ELSE ) ) {
        else_token = ConsumeOp( parser, index, error, OP_ELSE, "Expected 'else'" );
        if ( *error )
            return NULL;

        else_stmt = GetOP( parser, index, error );
        if ( *error )
            return NULL;
        if ( !else_stmt )
            SyntaxError( "Expected statement after else" );
    }

    if_token->left = condition;
    if ( condition )
        condition->parent = if_token;

    if ( !else_token ) {
        if_token->right = then_stmt;
        if ( then_stmt )
            then_stmt->parent = if_token;
        return if_token;
    }

    else_token->left = then_stmt;
    else_token->right = else_stmt;
    if ( then_stmt )
        then_stmt->parent = else_token;
    if ( else_stmt )
        else_stmt->parent = else_token;

    if_token->right = else_token;
    else_token->parent = if_token;

    return if_token;
}

static Node_t *GetBlock( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    ConsumeOp( parser, index, error, OP_OPEN_BRACE, "Expected '{'" );
    if ( *error )
        return NULL;

    Node_t *body = NULL;
    if ( !MatchToken( parser, *index, OP_CLOSE_BRACE ) ) {
        body = GetOPSeq( parser, index, error, OP_CLOSE_BRACE );
        if ( *error )
            return NULL;
    }

    ConsumeOp( parser, index, error, OP_CLOSE_BRACE, "Expected '}'" );
    if ( *error )
        return NULL;

    if ( !body )
        SyntaxError( "Empty block is not allowed" );

    return body;
}

static Node_t *GetExpression( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    Node_t *node = GetTerm( parser, index, error );
    if ( *error )
        return NULL;

    while ( !AtEnd( parser, *index ) ) {
        if ( MatchToken( parser, *index, OP_ADD ) || MatchToken( parser, *index, OP_SUB ) ) {
            Node_t *op_node = parser->tokens.data[*index];
            ( *index )++;

            Node_t *right = GetTerm( parser, index, error );
            if ( *error )
                return NULL;

            op_node->left = node;
            op_node->right = right;

            if ( node )
                node->parent = op_node;
            if ( right )
                right->parent = op_node;

            node = op_node;
        } else {
            break;
        }
    }

    return node;
}

static Node_t *GetTerm( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    Node_t *node = GetPow( parser, index, error );
    if ( *error )
        return NULL;

    while ( !AtEnd( parser, *index ) ) {
        if ( MatchToken( parser, *index, OP_MUL ) || MatchToken( parser, *index, OP_DIV ) ) {
            Node_t *op_node = parser->tokens.data[*index];
            ( *index )++;

            Node_t *right = GetPow( parser, index, error );
            if ( *error )
                return NULL;

            op_node->left = node;
            op_node->right = right;

            if ( node )
                node->parent = op_node;
            if ( right )
                right->parent = op_node;

            node = op_node;
        } else {
            break;
        }
    }

    return node;
}

static Node_t *GetPow( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    Node_t *node = GetUnary( parser, index, error );
    if ( *error )
        return NULL;

    while ( MatchToken( parser, *index, OP_POW ) ) {
        Node_t *op_node = parser->tokens.data[*index];
        ( *index )++;

        Node_t *right = GetUnary( parser, index, error );
        if ( *error )
            return NULL;

        op_node->left = node;
        op_node->right = right;

        if ( node )
            node->parent = op_node;
        if ( right )
            right->parent = op_node;

        node = op_node;
    }

    return node;
}

static Node_t *GetUnary( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    // sqrt(expr)
    if ( MatchToken( parser, *index, OP_SQRT ) ) {
        Node_t *sqrt_token = parser->tokens.data[*index];
        (*index)++;

        if ( !MatchToken( parser, *index, OP_OPEN_PARENT ) )
            SyntaxError( "Expected '(' after 'sqrt'" );
        (*index)++;

        Node_t *expr = GetExpression( parser, index, error );
        if ( *error )
            return NULL;

        if ( !MatchToken( parser, *index, OP_CLOSE_PARENT ) )
            SyntaxError( "Expected ')' after sqrt expression" );
        (*index)++;

        sqrt_token->left = expr;
        sqrt_token->right = NULL;
        if ( expr )
            expr->parent = sqrt_token;

        return sqrt_token;
    }

    return GetPrimary( parser, index, error );
}

static Node_t *GetArgList( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    Node_t *head = GetExpression( parser, index, error );
    if ( *error )
        return NULL;

    while ( MatchToken( parser, *index, OP_COMMA ) ) {
        Node_t *comma_token = parser->tokens.data[*index];
        (*index)++;

        Node_t *next_arg = GetExpression( parser, index, error );
        if ( *error )
            return NULL;

        comma_token->left = head;
        comma_token->right = next_arg;
        if ( head )
            head->parent = comma_token;
        if ( next_arg )
            next_arg->parent = comma_token;

        head = comma_token;
    }

    return head;
}

static Node_t *GetPrimary( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    if ( AtEnd( parser, *index ) ) {
        SyntaxError( "Unexpected end of input" );
    }

    Node_t *tok = parser->tokens.data[*index];

    // Числа
    if ( tok->value.type == NODE_NUMBER ) {
        ( *index )++;
        return tok;
    }

    // input()
    if ( MatchToken( parser, *index, OP_IN ) ) {
        Node_t *input_token = parser->tokens.data[*index];
        (*index)++;

        if ( !MatchToken( parser, *index, OP_OPEN_PARENT ) )
            SyntaxError( "Expected '(' after 'input'" );
        (*index)++;

        if ( !MatchToken( parser, *index, OP_CLOSE_PARENT ) )
            SyntaxError( "Expected ')' after 'input'" );
        (*index)++;

        input_token->left = NULL;
        input_token->right = NULL;

        return input_token;
    }

    // print(expr)
    if ( MatchToken( parser, *index, OP_OUT ) ) {
        Node_t *print_token = parser->tokens.data[*index];
        (*index)++;

        if ( !MatchToken( parser, *index, OP_OPEN_PARENT ) )
            SyntaxError( "Expected '(' after 'print'" );
        (*index)++;

        Node_t *expr = GetExpression( parser, index, error );
        if ( *error )
            return NULL;

        if ( !MatchToken( parser, *index, OP_CLOSE_PARENT ) )
            SyntaxError( "Expected ')' after print expression" );
        (*index)++;

        print_token->left = expr;
        print_token->right = NULL;
        if ( expr )
            expr->parent = print_token;

        return print_token;
    }

    // call func(args)
    if ( MatchToken( parser, *index, OP_CALL ) ) {
        Node_t *call_token = parser->tokens.data[*index];
        (*index)++;

        if ( !MatchVariable( parser, *index ) )
            SyntaxError( "Expected function name after 'call'" );

        Node_t *func_name = parser->tokens.data[*index];
        (*index)++;

        if ( !MatchToken( parser, *index, OP_OPEN_PARENT ) )
            SyntaxError( "Expected '(' after function name" );
        (*index)++;

        Node_t *args = NULL;
        if ( !MatchToken( parser, *index, OP_CLOSE_PARENT ) ) {
            args = GetArgList( parser, index, error );
            if ( *error )
                return NULL;
        }

        if ( !MatchToken( parser, *index, OP_CLOSE_PARENT ) )
            SyntaxError( "Expected ')' after arguments" );
        (*index)++;

        call_token->left = func_name;
        call_token->right = args;
        if ( func_name )
            func_name->parent = call_token;
        if ( args )
            args->parent = call_token;

        return call_token;
    }

    // Переменные
    if ( tok->value.type == NODE_VARIABLE ) {
        ( *index )++;
        return tok;
    }

    // (expr)
    if ( MatchToken( parser, *index, OP_OPEN_PARENT ) ) {
        ( *index )++;

        Node_t *expr = GetExpression( parser, index, error );
        if ( *error )
            return NULL;

        ConsumeOp( parser, index, error, OP_CLOSE_PARENT, "Expected ')'" );
        if ( *error )
            return NULL;

        return expr;
    }

    SyntaxError( "Expected number, variable, or '('" );
    return NULL;
}
