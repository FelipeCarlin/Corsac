#if !defined(CORSAC_PARSER_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Felipe Carlin $
   $Notice: Copyright © 2022 Felipe Carlin $
   ======================================================================== */

typedef enum ast_node_type
{
    ASTNodeType_Number,                  // Integer
    
    ASTNodeType_Negate,                  // Unary -
    
    ASTNodeType_Add,                     // Binary addition
    ASTNodeType_Sub,                     // Binary subtraction
    ASTNodeType_Multiply,                // Binary multiply
    ASTNodeType_Divide,                  // Binary divide
    
    ASTNodeType_Equal,                   // Comparison ==
    ASTNodeType_NotEqual,                // Comparison !=
    ASTNodeType_LessThan,                // Comparison <
    ASTNodeType_LessEqual,               // Comparison <=
    
    ASTNodeType_Assign,                  // =
    
    ASTNodeType_Expression_Statement,    // Expression ";"

    ASTNodeType_Variable,                // Variable
    
    ASTNodeType_Count,                   // Internal use, boundry checking.
} ast_node_type;

typedef struct ast_node
{
    ast_node_type NodeType;
    
    struct ast_node *Next;
    struct ast_node *LeftHandSide;
    struct ast_node *RightHandSide;
    
    uint64 NumericalValue;
    char VariableName;
    
    // NOTE(felipe): Representative token for nicer error logging.
    token *Token;
} ast_node;

#define CORSAC_PARSER_H
#endif
