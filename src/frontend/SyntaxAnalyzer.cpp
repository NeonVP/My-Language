#include <stdlib.h>

#include "DebugUtils.h"
#include "frontend/Parser.h"

// Program     = { Statement }
// Statement   = ( Assignment | Expression ) ";"
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
static Node_t *GetStatement( Parser_t *parser, size_t *index );
static Node_t *GetAssignment( Parser_t *parser, size_t *index, Node_t *var, OperationType op );
static Node_t *GetExpression( Parser_t *parser, size_t *index );
static Node_t *GetTerm( Parser_t *parser, size_t *index );
static Node_t *GetPow( Parser_t *parser, size_t *index );
static Node_t *GetPrimary( Parser_t *parser, size_t *index );
static Node_t *GetFunctionCall( Parser_t *parser, size_t *index, const char *func_name );

static bool MatchToken( Parser_t *parser, size_t index, OperationType op );
static bool MatchVariable( Parser_t *parser, size_t index, char var_name );
static bool MatchNumber( Parser_t *parser, size_t index, double *out_num );
static bool AtEnd( Parser_t *parser, size_t index );

static bool AtEnd( Parser_t *parser, size_t index ) { return index >= parser->tokens.size; }

static bool MatchToken( Parser_t *parser, size_t index, OperationType op ) {
    if ( AtEnd( parser, index ) )
        return false;
    Node_t *tok = parser->tokens.data[index];
    return tok->value.type == NODE_OPERATION && (OperationType)tok->value.data.operation == op;
}

static bool MatchVariable( Parser_t *parser, size_t index, char var_name ) {
    if ( AtEnd( parser, index ) )
        return false;
    Node_t *tok = parser->tokens.data[index];
    return tok->value.type == NODE_VARIABLE && tok->value.data.variable == var_name;
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
            // Соединяем последовательность через OP_SEMICOLON (или отдельный узел BLOCK)
            TreeData_t seq_data = MakeOperation( OP_SEMICOLON );
            Node_t *seq = NodeCreate( seq_data, NULL );
            seq->left = prev;
            seq->right = stmt;
            if ( prev )
                prev->parent = seq;
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
