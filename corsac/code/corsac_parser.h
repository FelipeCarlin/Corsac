#if !defined(CORSAC_PARSER_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Felipe Carlin $
   $Notice: Copyright © 2022 Felipe Carlin $
   ======================================================================== */

typedef enum object_type
{
    ObjectType_Null,
    ObjectType_Variable,
    ObjectType_Function,
} object_type;

typedef enum object_storage
{
    ObjectStorage_Null,
    ObjectStorage_Local,
    ObjectStorage_Global,
} object_storage;

// NOTE(felipe): Functions, Variables.
typedef struct object
{
    char *Name;

    object_type Type;
    object_storage Storage;
    
    struct object *Next;
    
    // Variable
    uint32 StackBaseOffset;

    // Function
    struct ast_node *Body;
    struct object *LocalVariables;
    uint32 StackSize;
} object;

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
    ASTNodeType_Block,                   // { ... }
    
    ASTNodeType_Variable,                // Variable
//    ASTNodeType_Function,                // Function definition

    ASTNodeType_Return,                  // "return" statement
    ASTNodeType_If,                      // "if" statement
    ASTNodeType_For,                     // "for" statement 
    
    ASTNodeType_Count,                   // Internal use, boundry checking
} ast_node_type;

typedef struct ast_node
{
    ast_node_type NodeType;
    
    struct ast_node *Next;
    struct ast_node *LeftHandSide;
    struct ast_node *RightHandSide;

    // NOTE(felipe): Representative token for nicer error logging.
    token *Token;
    
    struct ast_node *Body;
    
    // "if" / "for" statement
    struct ast_node *Init;
    struct ast_node *Condition;
    struct ast_node *Increment;
    struct ast_node *Then;
    struct ast_node *Else;
    
    // Node Number
    uint64 NumericalValue;

    // Node Variable
    object *Variable;    
} ast_node;

typedef struct program
{
    // NOTE(felipe): All nodes should be functions.
    object *Objects;
} program;

#define CORSAC_PARSER_H
#endif
