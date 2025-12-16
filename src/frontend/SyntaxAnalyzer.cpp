#include <stdlib.h>
#include <string.h>

#include "DebugUtils.h"
#include "frontend/Parser.h"

// Program     = { Statement }
// Statement   = WhileStmt | IfStmt | ( Assignment | Expression ) ";"
// WhileStmt   = "while" "(" Expression ")" "{" { Statement } "}"
// IfStmt      = "if" "(" Expression ")" "{" { Statement } "}" [ "else" "{" { Statement } "}" ]
// Assignment  = Variable ( ":=" | "=" ) Expression
// Expression  = Term { ("+" | "-") Term }
// Term        = Pow { ("*" | "/") Pow }
// Pow         = Primary { "^" Primary }
// Primary     = Number
//             | Variable
//             | FunctionCall
//             | "(" Expression ")"
//
// FunctionCall = Identifier "(" [ Expression { "," Expression } ] ")"


static Node_t *GetProgram( Parser_t *parser, size_t *index );
static Node_t *GetWhileStmt( Parser_t *parser, size_t *index );
static Node_t *GetIfStmt( Parser_t *parser, size_t *index );
static Node_t *GetStatement( Parser_t *parser, size_t *index );
static Node_t *GetAssignment( Parser_t *parser, size_t *index, Node_t *var, OperationType op );
static Node_t *GetExpression( Parser_t *parser, size_t *index );
static Node_t *GetTerm( Parser_t *parser, size_t *index );
static Node_t *GetPow( Parser_t *parser, size_t *index );
static Node_t *GetPrimary( Parser_t *parser, size_t *index );
// static Node_t *GetFunctionCall( Parser_t *parser, size_t *index, const char *func_name );

static bool MatchToken( Parser_t *parser, size_t index, OperationType op );
static bool MatchVariable( Parser_t *parser, size_t index, const char *var_name );
static bool MatchNumber( Parser_t *parser, size_t index, double *out_num );
static bool AtEnd( Parser_t *parser, size_t index );

static bool AtEnd( Parser_t *parser, size_t index ) { return index >= parser->tokens.size; }

static bool MatchToken( Parser_t *parser, size_t index, OperationType op ) {
    if ( AtEnd( parser, index ) )
        return false;
    Node_t *tok = parser->tokens.data[index];
    return tok->value.type == NODE_OPERATION && (OperationType)tok->value.data.operation == op;
}

static bool MatchVariable( Parser_t *parser, size_t index, const char *var_name ) {
    if ( AtEnd( parser, index ) )
        return false;
    Node_t *tok = parser->tokens.data[index];
    return tok->value.type == NODE_VARIABLE && strcmp(tok->value.data.variable, var_name) == 0;
}

static bool MatchNumber( Parser_t *parser, size_t index, double *out_num ) {
    if ( AtEnd( parser, index ) )
        return false;
    Node_t *tok = parser->tokens.data[index];
    if ( tok->value.type == NODE_NUMBER ) {
        if ( out_num )
            *out_num = tok->value.data.number;
        return true;
    }
    return false;
}

static void ExpectToken( Parser_t *parser, size_t *index, OperationType op, const char *msg ) {
    if ( !MatchToken( parser, *index, op ) ) {
        PRINT_ERROR( "Expected %s at token %zu", msg, *index );
        abort();
    }
    ( *index )++;
}

static Node_t *GetProgram( Parser_t *parser, size_t *index ) {
    Node_t *root = NULL;
    Node_t *prev = NULL;

    while ( !AtEnd( parser, *index ) && !MatchToken( parser, *index, OP_CLOSE_BRACE ) ) {
        Node_t *stmt = GetStatement( parser, index );
        if ( !stmt )
            break;

        if ( !root ) {
            root = stmt;
        } else {
            TreeData_t seq_data = MakeOperation( OP_SEMICOLON );
            Node_t *seq = NodeCreate( seq_data, NULL );
            seq->left = prev;
            seq->right = stmt;
            if ( prev )
                prev->parent = seq;
            if ( stmt )
                stmt->parent = seq;
            if ( root == prev )
                root = seq;
            prev = seq;
        }
        prev = stmt;
    }

    return root;
}


static Node_t *GetStatement( Parser_t *parser, size_t *index ) {
    if ( AtEnd( parser, *index ) )
        return NULL;

    // Check for while statement
    if ( MatchToken( parser, *index, OP_WHILE ) ) {
        return GetWhileStmt( parser, index );
    }

    // Check for if statement
    if ( MatchToken( parser, *index, OP_IF ) ) {
        return GetIfStmt( parser, index );
    }

    // Assignment: x := expr
    if ( parser->tokens.data[*index]->value.type == NODE_VARIABLE ) {
        Node_t *var = NodeCopy( parser->tokens.data[( *index )++] );
        if ( MatchToken( parser, *index, OP_ADVERT ) || MatchToken( parser, *index, OP_ASSIGN ) ) {
            OperationType assign_op = (OperationType)parser->tokens.data[*index]->value.data.operation;
            ( *index )++;
            Node_t *expr = GetExpression( parser, index );
            return GetAssignment( parser, index, var, assign_op );
        } else {
            Node_t *expr = var;
            ExpectToken( parser, index, OP_SEMICOLON, "';' after expression" );
            return expr;
        }
    }

    // Expression: expr;
    Node_t *expr = GetExpression( parser, index );
    ExpectToken( parser, index, OP_SEMICOLON, "';' after expression" );
    return expr;
}

static Node_t *GetWhileStmt( Parser_t *parser, size_t *index ) {
    // Consume 'while' token
    ( *index )++;

    // Expect '('
    ExpectToken( parser, index, OP_OPEN_PARENT, "\"(\" after while" );

    // Get condition expression
    Node_t *condition = GetExpression( parser, index );

    // Expect ')'
    ExpectToken( parser, index, OP_CLOSE_PARENT, "\")\" after while condition" );

    // Expect '{'
    ExpectToken( parser, index, OP_OPEN_BRACE, "\"{\" after while condition" );

    // Get body statements until '}'
    Node_t *body = GetProgram( parser, index );

    // Expect '}'
    ExpectToken( parser, index, OP_CLOSE_BRACE, "\"}\" after while body" );

    // Create while node
    TreeData_t while_data = MakeOperation( OP_WHILE );
    Node_t *while_node = NodeCreate( while_data, NULL );
    while_node->left = condition;
    while_node->right = body;
    if ( condition )
        condition->parent = while_node;
    if ( body )
        body->parent = while_node;

    return while_node;
}

static Node_t *GetIfStmt( Parser_t *parser, size_t *index ) {
    // Consume 'if' token
    ( *index )++;

    // Expect '('
    ExpectToken( parser, index, OP_OPEN_PARENT, "\"(\" after if" );

    // Get condition expression
    Node_t *condition = GetExpression( parser, index );

    // Expect ')'
    ExpectToken( parser, index, OP_CLOSE_PARENT, "\")\" after if condition" );

    // Expect '{'
    ExpectToken( parser, index, OP_OPEN_BRACE, "\"{\" after if condition" );

    // Get then body statements until '}'
    Node_t *then_body = GetProgram( parser, index );

    // Expect '}'
    ExpectToken( parser, index, OP_CLOSE_BRACE, "\"}\" after if then body" );

    // Check for optional 'else'
    Node_t *else_body = NULL;
    if ( MatchToken( parser, *index, OP_ELSE ) ) {
        ( *index )++; // consume 'else'

        // Expect '{'
        ExpectToken( parser, index, OP_OPEN_BRACE, "\"{\" after else" );

        // Get else body statements until '}'
        else_body = GetProgram( parser, index );

        // Expect '}'
        ExpectToken( parser, index, OP_CLOSE_BRACE, "\"}\" after else body" );
    }

    // Create if node
    TreeData_t if_data = MakeOperation( OP_IF );
    Node_t *if_node = NodeCreate( if_data, NULL );
    if_node->left = condition;
    if_node->right = NodeCreate( MakeOperation( OP_SEMICOLON ), NULL ); // connector for then and else
    if_node->right->left = then_body;
    if_node->right->right = else_body;

    if ( condition )
        condition->parent = if_node;
    if ( then_body )
        then_body->parent = if_node->right;
    if ( else_body )
        else_body->parent = if_node->right;

    return if_node;
}


static Node_t *GetAssignment( Parser_t *parser, size_t *index, Node_t *var, OperationType op ) {
    Node_t *expr = GetExpression( parser, index );
    ExpectToken( parser, index, OP_SEMICOLON, "';' after assignment" );

    TreeData_t assign_data = MakeOperation( op );
    Node_t *assign = NodeCreate( assign_data, NULL );
    assign->left = var;
    assign->right = expr;
    if ( var )
        var->parent = assign;
    if ( expr )
        expr->parent = assign;

    return assign;
}

static Node_t *GetExpression( Parser_t *parser, size_t *index ) {
    Node_t *node = GetTerm( parser, index );

    while ( !AtEnd( parser, *index ) ) {
        if ( MatchToken( parser, *index, OP_ADD ) ) {
            ( *index )++;
            Node_t *right = GetTerm( parser, index );
            TreeData_t op_data = MakeOperation( OP_ADD );
            Node_t *new_root = NodeCreate( op_data, NULL );
            new_root->left = node;
            new_root->right = right;
            if ( node )
                node->parent = new_root;
            if ( right )
                right->parent = new_root;
            node = new_root;
        } else if ( MatchToken( parser, *index, OP_SUB ) ) {
            ( *index )++;
            Node_t *right = GetTerm( parser, index );
            TreeData_t op_data = MakeOperation( OP_SUB );
            Node_t *new_root = NodeCreate( op_data, NULL );
            new_root->left = node;
            new_root->right = right;
            if ( node )
                node->parent = new_root;
            if ( right )
                right->parent = new_root;
            node = new_root;
        } else {
            break;
        }
    }

    return node;
}

static Node_t *GetTerm( Parser_t *parser, size_t *index ) {
    Node_t *node = GetPow( parser, index );

    while ( !AtEnd( parser, *index ) ) {
        if ( MatchToken( parser, *index, OP_MUL ) ) {
            ( *index )++;
            Node_t *right = GetPow( parser, index );
            TreeData_t op_data = MakeOperation( OP_MUL );
            Node_t *new_root = NodeCreate( op_data, NULL );
            new_root->left = node;
            new_root->right = right;
            if ( node )
                node->parent = new_root;
            if ( right )
                right->parent = new_root;
            node = new_root;
        } else if ( MatchToken( parser, *index, OP_DIV ) ) {
            ( *index )++;
            Node_t *right = GetPow( parser, index );
            TreeData_t op_data = MakeOperation( OP_DIV );
            Node_t *new_root = NodeCreate( op_data, NULL );
            new_root->left = node;
            new_root->right = right;
            if ( node )
                node->parent = new_root;
            if ( right )
                right->parent = new_root;
            node = new_root;
        } else {
            break;
        }
    }

    return node;
}

static Node_t *GetPow( Parser_t *parser, size_t *index ) {
    Node_t *node = GetPrimary( parser, index );

    while ( !AtEnd( parser, *index ) && MatchToken( parser, *index, OP_POW ) ) {
        ( *index )++;
        Node_t *right = GetPrimary( parser, index );
        TreeData_t op_data = MakeOperation( OP_POW );
        Node_t *new_root = NodeCreate( op_data, NULL );
        new_root->left = node;
        new_root->right = right;
        if ( node )
            node->parent = new_root;
        if ( right )
            right->parent = new_root;
        node = new_root;
    }

    return node;
}

static Node_t *GetPrimary( Parser_t *parser, size_t *index ) {
    if ( AtEnd( parser, *index ) ) {
        PRINT_ERROR( "Unexpected end of input" );
        abort();
    }

    Node_t *tok = parser->tokens.data[*index];

    if ( tok->value.type == NODE_NUMBER ) {
        ( *index )++;
        return NodeCopy( tok );
    }

    if ( tok->value.type == NODE_VARIABLE ) {
        char *var_name = tok->value.data.variable;
        ( *index )++;
        if ( !AtEnd( parser, *index ) && MatchToken( parser, *index, OP_OPEN_PARENT ) ) {
            // return GetFunctionCall( parser, index, var_name );
            return GetExpression( parser, index );
        }
        return NodeCopy( tok );
    }

    if ( MatchToken( parser, *index, OP_OPEN_PARENT ) ) {
        ( *index )++;
        Node_t *expr = GetExpression( parser, index );
        ExpectToken( parser, index, OP_CLOSE_PARENT, "')'" );
        return expr;
    }

    PRINT_ERROR( "Unexpected token at %zu", *index );
    abort();
}


Node_t *SyntaxAnalyze( Parser_t* parser ) {
    my_assert( parser, "Null pointer on `parser`" );

    size_t index = 0;
    return GetProgram( parser, &index );
}

// static Node_t *GetFunctionCall( Parser_t *parser, size_t *index, const char *func_name ) {
//     // func_name — это один символ (переменная)
//     OperationType op = OP_NOPE;
//     if ( strncmp( func_name, "sin", 1 ) == 0 )
//         op = OP_SIN;
//     else if ( strncmp( func_name, "cos", 1 ) == 0 )
//         op = OP_COS;
//     // ... добавьте все функции

//     ExpectToken( parser, index, OP_OPEN_PARENT, "'(' after function name" );

//     Node_t *arg1 = NULL, *arg2 = NULL;
//     if ( !MatchToken( parser, *index, OP_CLOSE_PARENT ) ) {
//         arg1 = GetExpression( parser, index );
//         if ( !MatchToken( parser, *index, OP_CLOSE_PARENT ) ) {
//             ExpectToken( parser, index, OP_COMMA, "','" );
//             arg2 = GetExpression( parser, index );
//         }
//     }

//     ExpectToken( parser, index, OP_CLOSE_PARENT, "')'" );

//     TreeData_t op_data = MakeOperation( op );
//     Node_t *call = NodeCreate( op_data, NULL );
//     call->left = arg1;
//     if ( arg1 )
//         arg1->parent = call;
//     call->right = arg2;
//     if ( arg2 )
//         arg2->parent = call;

//     return call;
// }
