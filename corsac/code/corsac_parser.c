
#include "corsac_parser.h"

inline ast_node *
NewNode(ast_node_type NodeType)
{
    ast_node *Node = calloc(1, sizeof(ast_node));
    Node->NodeType = NodeType;
    
    return Node;
}

inline ast_node *
NewBinaryNode(ast_node_type NodeType, ast_node *LeftHandSide, ast_node *RightHandSide)
{
    ast_node *Result = NewNode(NodeType);
    Result->LeftHandSide = LeftHandSide;
    Result->RightHandSide = RightHandSide;

    return Result;
}

// Ensure that the current token is `s`.
inline token *
AssertNext(token *Token, char *S)
{
    if(!TokenIs(Token, S))
    {
        ErrorInToken(Token, "expected '%s'", S);
    }
    
    return Token->Next;
}

internal ast_node *Expression(token *Token, token **Rest);

// Primary = "(" Expression ")"
//         | Identifier
//         | Number
internal ast_node *
Primary(token *Token, token **Rest)
{
    ast_node *Result = 0;

    if(TokenIs(Token, "("))
    {
        Result = Expression(Token->Next, &Token);
        *Rest = AssertNext(Token, ")");
    }
    else if(Token->TokenType == TokenType_Identifier)
    {
        Result = NewNode(ASTNodeType_Variable);
        Result->VariableName = *Token->Location;
        *Rest = Token->Next;
    }
    else if(Token->TokenType == TokenType_Number)
    {
        Result = NewNode(ASTNodeType_Number);
        Result->Token = Token;
        
        Result->NumericalValue = Token->NumericalValue;

        *Rest = Token->Next;
    }
    else
    {
        ErrorInToken(Token, "expected a number");
    }
    
    return Result;
}

// Unary = ("+" | "-") Unary
//       | Primary
internal ast_node *
Unary(token *Token, token **Rest)
{
    ast_node *Result = 0;
    
    if(TokenIs(Token, "+"))
    {
        Result = Unary(Token->Next, Rest);
    }
    else if(TokenIs(Token, "-"))
    {
        Result = NewNode(ASTNodeType_Negate);
        Result->LeftHandSide = Unary(Token->Next, Rest);
    }
    else
    {
        Result = Primary(Token, Rest);
    }
    
    return Result;
}

// Multiply = Unary ("*" Unary | "/" Unary)*
internal ast_node *
Multiply(token *Token, token **Rest)
{
    ast_node *Result = Unary(Token, &Token);
    
    for(;;)
    {
        if(TokenIs(Token, "*"))
        {
            Result = NewBinaryNode(ASTNodeType_Multiply, Result, Unary(Token->Next, &Token));
        }
        else if(TokenIs(Token, "/"))
        {
            Result = NewBinaryNode(ASTNodeType_Divide, Result, Unary(Token->Next, &Token));
        }
        else
        {
            *Rest = Token;
            break;
        }
    }
    
    return Result;
}

// Add = Multiply ("+" Multiply | "-" Multiply)*
internal ast_node *
Add(token *Token, token **Rest)
{
    ast_node *Result = Multiply(Token, &Token);
    
    for(;;)
    {
        if(TokenIs(Token, "+"))
        {
            Result = NewBinaryNode(ASTNodeType_Add, Result, Multiply(Token->Next, &Token));
        }
        else if(TokenIs(Token, "-"))
        {
            Result = NewBinaryNode(ASTNodeType_Sub, Result, Multiply(Token->Next, &Token));
        }
        else
        {
            *Rest = Token;
            break;
        }
    }
    
    return Result;
}

// Relational = Add ("<" Add | "<=" Add | ">" Add | ">=" Add)*
internal ast_node *
Relational(token *Token, token **Rest)
{
    ast_node *Result = Add(Token, &Token);

    for(;;)
    {
        if(TokenIs(Token, "<"))
        {
            Result = NewBinaryNode(ASTNodeType_LessThan, Result, Add(Token->Next, &Token));
        }
        else if(TokenIs(Token, "<="))
        {
            Result = NewBinaryNode(ASTNodeType_LessEqual, Result, Add(Token->Next, &Token));
        }
        if(TokenIs(Token, ">"))
        {
            Result = NewBinaryNode(ASTNodeType_LessThan, Add(Token->Next, &Token), Result);
        }
        else if(TokenIs(Token, ">="))
        {
            Result = NewBinaryNode(ASTNodeType_LessEqual, Add(Token->Next, &Token), Result);
        }
        else
        {
            *Rest = Token;
            break;
        }
    }

    return Result;
}

// Equality = Relational ("==" Relational | "!=" Relational)*
internal ast_node *
Equality(token *Token, token **Rest)
{
    ast_node *Result = Relational(Token, &Token);
    
    for(;;)
    {
        if(TokenIs(Token, "=="))
        {
            Result = NewBinaryNode(ASTNodeType_Equal, Result, Relational(Token->Next, &Token));
        }
        else if(TokenIs(Token, "!="))
        {
            Result = NewBinaryNode(ASTNodeType_NotEqual, Result, Relational(Token->Next, &Token));
        }
        else
        {
            *Rest = Token;
            break;
        }
    }
    
    return Result;
}

// Assign = Equality ("=" Assign)?
internal ast_node *
Assign(token *Token, token **Rest)
{
    ast_node *Result = Equality(Token, &Token);
    
    if(TokenIs(Token, "="))
    {
        Result = NewBinaryNode(ASTNodeType_Assign, Result, Assign(Token->Next, &Token));
    }
    
    *Rest = Token;

    return Result;
}

// Expression = Assign
internal ast_node *
Expression(token *Token, token **Rest)
{
    ast_node *Result = Assign(Token, Rest);

    return Result;
}

// Expression-Statement = ";"
//                      | Expression-Statement ";"
internal ast_node *
ExpressionStatement(token *Token, token **Rest)
{
    ast_node *Result = 0;
    
    if(TokenIs(Token, ";"))
    {
        ErrorInToken(Token, "blank expressions are not supported");
        
#if 0
        Result = NewNode(NodeType_Block, Token);
        *Rest = Token->Next;
#endif
    }
    else
    {
        Result = NewNode(ASTNodeType_Expression_Statement);
        Result->LeftHandSide = Expression(Token, &Token);
        
        *Rest = AssertNext(Token, ";");
    }
    
    return Result;
}

// Statement = Expresion-Statement
internal ast_node *
Statement(token *Token, token **Rest)
{
    ast_node *Result = ExpressionStatement(Token, Rest);

    return Result;    
}

// Program = Statement*
internal ast_node *
Program(token *Token, token **Rest)
{        
    ast_node Head = {0};
    ast_node *Current = &Head;
    
    while(Token->TokenType != TokenType_EOF)
    {
        Current->Next = Statement(Token, &Token);
        Current = Current->Next;
    }

    return Head.Next;
}

char *NodeTypes[] =
{
    "Number",
    
    "Negate",
    
    "Add   ",
    "Sub   ",
    "Mul   ",
    "Div   ",
        
    "Equal ",
    "NotEqu",
    "LessTh",
    "LessEq",

    "Assign",
    
    "ExpStm",

    "Variab",
};

uint32 LastDepth = 0;

internal void
PrintASTNode(ast_node *Node, uint32 Depth)
{
    Assert(ArrayCount(NodeTypes) == ASTNodeType_Count);
    
    // NOTE(felipe): It's debug code, don't worry.
    for(;
        Node;
        Node = Node->Next)
    {
        printf(" ");
        if(LastDepth > Depth)
        {
            for(uint32 I = 0;
                I < Depth;
                ++I)
            {
                printf("| ");
            }
            
            printf("\n ");
        }
        
        for(uint32 I = 0;
            I < Depth;
            ++I)
        {
            if(I == (Depth - 1))
            {
                printf("|-");
            }
            else
            {
                printf("| ");
            }
        }
        
        printf("Node: type: %s", NodeTypes[Node->NodeType]);
        
        printf("\n");
        
        LastDepth = Depth;
        
        if(Node->LeftHandSide)
        {
            PrintASTNode(Node->LeftHandSide, Depth + 1);
        }
        if(Node->RightHandSide)
        {
            PrintASTNode(Node->RightHandSide, Depth + 1);
        }
    }
}

internal void
ParseTokens(token *Tokens)
{
    token *Token = Tokens;

    ast_node *Nodes = Program(Token, &Token);
    
    // DEBUG: Print AST node tree.
    printf("\nAST\n");
    PrintASTNode(Nodes, 1);
}
