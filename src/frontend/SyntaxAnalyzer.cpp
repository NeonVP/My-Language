#include <stdlib.h>
#include <string.h>

#include "DebugUtils.h"
#include "Tree.h"
#include "UtilsRW.h"
#include "frontend/Parser.h"

// ===== Grammar (РБНФ / EBNF-ish) =====
// Grammar     ::= OPSeq
// OPSeq       ::= OP { OP }                // OP+
// OP          ::= Block
//              | Assignment ";"
//              | Expression ";"
//              | IfStmt
//              | WhileStmt
// Block       ::= "{" OPSeq? "}" [ ";" ]   // ';' allowed before '}' / EOF for sequencing
// Assignment  ::= Variable ( ":=" | "=" ) Expression
// IfStmt      ::= "if" "(" Expression ")" OP [ "else" OP ] [ ";" ]
// WhileStmt   ::= "while" "(" Expression ")" OP [ ";" ]
// Expression  ::= Term { ("+" | "-") Term }*
// Term        ::= Pow { ("*" | "/") Pow }*
// Pow         ::= Primary { "^" Primary }
// Primary     ::= Number
//              | Variable
//              | "(" Expression ")"

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
static Node_t *GetOPSeq( Parser_t *parser, size_t *index, bool *error, OperationType stop_op );
static Node_t *GetOP( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetAssignment( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetIfStmt( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetWhileStmt( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetExpression( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetTerm( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetPow( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetPrimary( Parser_t *parser, size_t *index, bool *error );
static Node_t *GetBlock( Parser_t *parser, size_t *index, bool *error );

Node_t *SyntaxAnalyze( Parser_t *parser ) {
    my_assert( parser, "Null pointer on `parser`" );

    PRINT( "Start syntax analysis" );

    bool error = false;
    size_t index = 0;
    Node_t *node = GetGrammar( parser, &index, &error );

    if ( error ) {
        // Extra diagnostics: sometimes the exact SyntaxError message is not enough in logs.
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

    Node_t *seq = GetOPSeq( parser, index, error, OP_NOPE );
    if ( *error )
        return NULL;
    if ( !seq )
        SyntaxError( "Expected at least one operator" );

    return seq;
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
        return op == OP_OPEN_BRACE || op == OP_IF || op == OP_WHILE || op == OP_OPEN_PARENT;
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

            // User's specific spine logic:
            // 1st semicolon: ( ; stmt1 nil )
            // n-th semicolon: ( ; prev_seq next_stmt )
            
            // Check if this is the first semicolon in the whole program/block
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
            // Join 'if' or similar to current semicolon spine
            Node_t *next = GetOP( parser, index, error );
            if ( *error ) return NULL;

            if ( head->value.type == NODE_OPERATION && (OperationType)head->value.data.operation == OP_SEMICOLON ) {
                if ( head->right == NULL ) {
                    head->right = next;
                    if ( next ) next->parent = head;
                } else {
                    // This happens if multiple stmts exist but we run out of tokens.
                    // We'll wrap again if needed, but for user's source this is the 'if' case.
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

    // Assignment or expression statement
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

    // Build assignment tree using existing nodes
    assign_op->left = var;
    assign_op->right = expr;

    if ( var )
        var->parent = assign_op;
    if ( expr )
        expr->parent = assign_op;

    return assign_op;
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

    // Body is parsed as OP.
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

    // if (...) OP else OP
    // Connect branches through else-token.
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

    // Braces are syntax-only: they do not appear in AST. We return the inside OPSeq.
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

    // Empty blocks are not allowed by OP+ in the original grammar.
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
            ( *index )++; // consume operator

            Node_t *right = GetTerm( parser, index, error );
            if ( *error )
                return NULL;

            // Build tree using op_node
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
            ( *index )++; // consume operator

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

    Node_t *node = GetPrimary( parser, index, error );
    if ( *error )
        return NULL;

    while ( MatchToken( parser, *index, OP_POW ) ) {
        Node_t *op_node = parser->tokens.data[*index];
        ( *index )++; // consume '^'

        Node_t *right = GetPrimary( parser, index, error );
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

static Node_t *GetPrimary( Parser_t *parser, size_t *index, bool *error ) {
    my_assert( parser, "Null pointer on `parser`" );
    my_assert( index, "Null pointer on `index`" );
    my_assert( error, "Null pointer on `error`" );

    if ( AtEnd( parser, *index ) ) {
        SyntaxError( "Unexpected end of input" );
    }

    Node_t *tok = parser->tokens.data[*index];

    if ( tok->value.type == NODE_NUMBER || tok->value.type == NODE_VARIABLE ) {
        ( *index )++;
        return tok;
    }

    if ( MatchToken( parser, *index, OP_OPEN_PARENT ) ) {
        ( *index )++; // consume '('

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