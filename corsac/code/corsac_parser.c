
typedef enum ast_node_type
{
    ASTNodeType_Number,      // Integer
    
    ASTNodeType_Negate,      // Unary -

    ASTNodeType_Add,         // Binary addition
    ASTNodeType_Sub,         // Binary subtraction
    ASTNodeType_Multiply,    // Binary multiply
    ASTNodeType_Divide,      // Binary divide
} ast_node_type;

typedef struct ast_node
{
    ast_node_type NodeType;
    
    struct ast_node *Next;
    struct ast_node *LeftHandSide;
    struct ast_node *RightHandSide;
    
    uint64 NumericalValue;
    
    // NOTE(felipe): Representative token for nicer error logging.
    token *Token;
} ast_node;

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

// Primary = Number
internal ast_node *
Primary(token *Token, token **Rest)
{
    ast_node *Result = 0;

    if(Token->TokenType == TokenType_Number)
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

// Expression = Multiply ("+" Multiply | "-" Multiply)*
internal ast_node *
Expression(token *Token, token **Rest)
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

char *NodeTypes[] =
{
    "Number",
    
    "Negate",
    
    "Add   ",
    "Sub   ",
    "Mul   ",
    "Div   ",
};

uint32 LastDepth = 0;

internal void
PrintASTNode(ast_node *Node, uint32 Depth)
{
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
            
            printf("\n");
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
    
    ast_node *Nodes = Expression(Token, &Token);
    
    // DEBUG: Print AST nodes.
    printf("\nAST\n");
    for(ast_node *Node = Nodes;
        Node;
        Node = Node->Next)
    {
        PrintASTNode(Node, 1);
    }
}
