/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Felipe Carlin $
   $Notice: Copyright ? 2022 Felipe Carlin $
   ======================================================================== */

#include "corsac_parser.h"

inline ast_node *
NewNode(ast_node_type NodeType, token *Token)
{
    ast_node *Node = calloc(1, sizeof(ast_node));
    Node->NodeType = NodeType;

    Node->Token = Token;
    
    return Node;
}

inline ast_node *
NewNumber(uint32 Value, token *Token)
{
    ast_node *Node = NewNode(ASTNodeType_Number, Token);
    Node->NumericalValue = Value;

    return Node;
}

inline ast_node *
NewBinaryNode(ast_node_type NodeType, ast_node *LeftHandSide, ast_node *RightHandSide, token *Token)
{
    ast_node *Result = NewNode(NodeType, Token);
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


// TODO(felipe): Remove globals.
global_variable object GlobalVariablesHead = {0};
global_variable object *GlobalVariables = &GlobalVariablesHead;


// Find a local variable by name.
internal object *
GetVariable(token *Token)
{
    object *Result = 0;
    
    for(object *Variable = GlobalVariablesHead.Next;
        Variable;
        Variable = Variable->Next)
    {
        if(StringLength(Variable->Name) == Token->Length && !StringCompare(Token->Location, Variable->Name, Token->Length))
        {
            Result = Variable;
        }
    }
    
    return Result;
}

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
        Result = NewNode(ASTNodeType_Variable, Token);
        
        // NOTE(felipe): Check if variable already exists.
        object *Variable = GetVariable(Token);
        if(!Variable)
        {
            Variable = calloc(1, sizeof(object));
            Variable->Name = StringDuplicate(Token->Location, Token->Length);

            GlobalVariables->Next = Variable;
            GlobalVariables = Variable;
        }
        
        Result->Variable = Variable;
        
        *Rest = Token->Next;
    }
    else if(Token->TokenType == TokenType_Number)
    {
        Result = NewNode(ASTNodeType_Number, Token);
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

// Unary = ("+" | "-"| "*" | "&") Unary
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
        Result = NewNode(ASTNodeType_Negate, Token);
        Result->LeftHandSide = Unary(Token->Next, Rest);
    }
    else if(TokenIs(Token, "*"))
    {
        Result = NewNode(ASTNodeType_Dereference, Token);
        Result->LeftHandSide = Unary(Token->Next, Rest);
    }
    else if(TokenIs(Token, "&"))
    {
        Result = NewNode(ASTNodeType_Address, Token);
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
        token *Start = Token;
        
        if(TokenIs(Token, "*"))
        {
            Result = NewBinaryNode(ASTNodeType_Multiply, Result, Unary(Token->Next, &Token), Start);
        }
        else if(TokenIs(Token, "/"))
        {
            Result = NewBinaryNode(ASTNodeType_Divide, Result, Unary(Token->Next, &Token), Start);
        }
        else
        {
            *Rest = Token;
            break;
        }
    }
    
    return Result;
}

variable_type *GlobalTypeInt = &(variable_type){TypeKind_Int};

inline bool32 TypeIsInteger(variable_type *Type)
{
    bool32 Result = Type->Kind == TypeKind_Int;
    return Result;
}

inline variable_type *
PointerTo(variable_type *Base)
{
  variable_type *Type = calloc(1, sizeof(variable_type));
  Type->Kind = TypeKind_Pointer;
  Type->Base = Base;
  return Type;
}

internal void
AddTypeToNode(ast_node *Node)
{
    if(Node && !Node->Type)
    {
        AddTypeToNode(Node->LeftHandSide);
        AddTypeToNode(Node->RightHandSide);
        AddTypeToNode(Node->Condition);
        AddTypeToNode(Node->Then);
        AddTypeToNode(Node->Else);
        AddTypeToNode(Node->Init);
        AddTypeToNode(Node->Increment);

        for(ast_node *N = Node->Body;
            N;
            N = N->Next)
        {
            AddTypeToNode(N);
        }
        
        switch(Node->NodeType)
        {
            case ASTNodeType_Add:
            case ASTNodeType_Sub:
            case ASTNodeType_Multiply:
            case ASTNodeType_Divide:
            case ASTNodeType_Negate:
            case ASTNodeType_Assign:
            {
                Node->Type = Node->LeftHandSide->Type;
            } break;
            
            case ASTNodeType_Equal:
            case ASTNodeType_NotEqual:
            case ASTNodeType_LessThan:
            case ASTNodeType_LessEqual:
            case ASTNodeType_Variable:
            case ASTNodeType_Number:
            {
                Node->Type = GlobalTypeInt;
            } break;

            case ASTNodeType_Address:
            {
                Node->Type = PointerTo(Node->LeftHandSide->Type);
            } break;

            case ASTNodeType_Dereference:
            {
                if(Node->LeftHandSide->Type->Kind == TypeKind_Pointer)
                {
                    Node->Type = Node->LeftHandSide->Type->Base;
                }
                else
                {
                    Node->Type = GlobalTypeInt;
                }
            } break;
        }
    }
}

internal ast_node *
NewAddition(ast_node *LeftHandSide, ast_node *RightHandSide, token *Token)
{
    ast_node *Result = 0;
    
    AddTypeToNode(LeftHandSide);
    AddTypeToNode(RightHandSide);
    
    if(TypeIsInteger(LeftHandSide->Type) && TypeIsInteger(RightHandSide->Type))
    {
        Result = NewBinaryNode(ASTNodeType_Add, LeftHandSide, RightHandSide, Token);
    }
    else
    {
        if(LeftHandSide->Type->Base && RightHandSide->Type->Base)
        {
            ErrorInToken(Token, "invalid operands");
        }
        else
        {
            // Canonicalize `num + ptr` to `ptr + num`.
            if(!LeftHandSide->Type->Base && RightHandSide->Type->Base)
            {
                ast_node *Temp = LeftHandSide;
                LeftHandSide = RightHandSide;
                RightHandSide = Temp;
            }
            
            RightHandSide = NewBinaryNode(ASTNodeType_Multiply, RightHandSide, NewNumber(8, Token), Token);
            Result = NewBinaryNode(ASTNodeType_Add, LeftHandSide, RightHandSide, Token);
        }
    }
    
    return Result;
}

// Like `+`, `-` is overloaded for the pointer type.
internal ast_node *
NewSubtraction(ast_node *LeftHandSide, ast_node *RightHandSide, token *Token)
{
    ast_node *Result = 0;
    
    AddTypeToNode(LeftHandSide);
    AddTypeToNode(RightHandSide);

    // num - num
    if(TypeIsInteger(LeftHandSide->Type) && TypeIsInteger(RightHandSide->Type))
    {
        Result = NewBinaryNode(ASTNodeType_Sub, LeftHandSide, RightHandSide, Token);
    }
    else if(LeftHandSide->Type->Base && TypeIsInteger(RightHandSide->Type))
    {
        // ptr - num
        
        RightHandSide = NewBinaryNode(ASTNodeType_Multiply, RightHandSide, NewNumber(8, Token), Token);
        
        AddTypeToNode(RightHandSide);

        Result = NewBinaryNode(ASTNodeType_Sub, LeftHandSide, RightHandSide, Token);
        Result->Type = LeftHandSide->Type;
    }
    else if(LeftHandSide->Type->Base && RightHandSide->Type->Base)
    {
        // ptr - ptr, which returns how many elements are between the two.
        
        ast_node *Node = NewBinaryNode(ASTNodeType_Sub, LeftHandSide, RightHandSide, Token);
        Node->Type = GlobalTypeInt;
        Result = NewBinaryNode(ASTNodeType_Divide, Node, NewNumber(8, Token), Token);
    }
    else
    {
        ErrorInToken(Token, "invalid operands");
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
        token *Start = Token;
        
        if(TokenIs(Token, "+"))
        {
//            Result = NewBinaryNode(ASTNodeType_Add, Result, Multiply(Token->Next, &Token), Start);
            Result = NewAddition(Result, Multiply(Token->Next, &Token), Start);
        }
        else if(TokenIs(Token, "-"))
        {
//            Result = NewBinaryNode(ASTNodeType_Sub, Result, Multiply(Token->Next, &Token), Start);
            Result = NewSubtraction(Result, Multiply(Token->Next, &Token), Start);
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
        token *Start = Token;
        
        if(TokenIs(Token, "<"))
        {
            Result = NewBinaryNode(ASTNodeType_LessThan, Result, Add(Token->Next, &Token), Start);
        }
        else if(TokenIs(Token, "<="))
        {
            Result = NewBinaryNode(ASTNodeType_LessEqual, Result, Add(Token->Next, &Token), Start);
        }
        if(TokenIs(Token, ">"))
        {
            Result = NewBinaryNode(ASTNodeType_LessThan, Add(Token->Next, &Token), Result, Start);
        }
        else if(TokenIs(Token, ">="))
        {
            Result = NewBinaryNode(ASTNodeType_LessEqual, Add(Token->Next, &Token), Result, Start);
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
        token *Start = Token;
        
        if(TokenIs(Token, "=="))
        {
            Result = NewBinaryNode(ASTNodeType_Equal, Result, Relational(Token->Next, &Token), Start);
        }
        else if(TokenIs(Token, "!="))
        {
            Result = NewBinaryNode(ASTNodeType_NotEqual, Result, Relational(Token->Next, &Token), Start);
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
        Result = NewBinaryNode(ASTNodeType_Assign, Result, Assign(Token->Next, &Token), Token);
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
//                      | Expression ";"
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
        Result = NewNode(ASTNodeType_Expression_Statement, Token);
        Result->LeftHandSide = Expression(Token, &Token);
        
        *Rest = AssertNext(Token, ";");
    }
    
    return Result;
}

internal ast_node *CompoundStatement(token *Token, token **Rest);

// Statement = "{" Compound-Statement
//           | "return" Expression ";"
//           | "if" "(" Expression ")" Statement ("else" Statement)?
//           | "for" "(" ExpressionStatement Expression? ";" Expression? ")" Statement
//           | "while" "(" Expression ")" Statement
//           | Expresion-Statement
internal ast_node *
Statement(token *Token, token **Rest)
{
    ast_node *Result = 0;
    
    if(TokenIs(Token, "{"))
    {
        Result = CompoundStatement(Token->Next, &Token);
    }
    else if(TokenIs(Token, "return"))
    {
        Result = NewNode(ASTNodeType_Return, Token);
        Result->LeftHandSide = Expression(Token->Next, &Token);
        
        Token = AssertNext(Token, ";");
    }
    else if(TokenIs(Token, "if"))
    {
        Result = NewNode(ASTNodeType_If, Token);
        
        Token = AssertNext(Token->Next, "(");
        Result->Condition = Expression(Token, &Token);
        Token = AssertNext(Token, ")");
        
        Result->Then = Statement(Token, &Token);
        
        if(TokenIs(Token, "else"))
        {
            Result->Else = Statement(Token->Next, &Token);
        }
    }
    else if(TokenIs(Token, "for"))
    {
        Result = NewNode(ASTNodeType_For, Token);
        
        Token = AssertNext(Token->Next, "(");
        Result->Init = ExpressionStatement(Token, &Token);

        if(!TokenIs(Token, ";"))
        {
            Result->Condition = Expression(Token, &Token);
        }
        Token = AssertNext(Token, ";");

        if(!TokenIs(Token, ")"))
        {
            Result->Increment = Expression(Token, &Token);
        }
        Token = AssertNext(Token, ")");
        
        Result->Then = Statement(Token, &Token);
    }
    else if(TokenIs(Token, "while"))
    {
        Result = NewNode(ASTNodeType_For, Token);
        
        Token = AssertNext(Token->Next, "(");
        Result->Condition = Expression(Token, &Token);
        
        Token = AssertNext(Token, ")");
        Result->Then = Statement(Token, &Token);
    }    
    else
    {
        Result = ExpressionStatement(Token, &Token);
    }
    
    *Rest = Token;
    
    return Result;    
}

// Compound-Statement = Statement* "}"
internal ast_node *
CompoundStatement(token *Token, token **Rest)
{
    ast_node *Result = NewNode(ASTNodeType_Block, Token);
    
    ast_node Head = {0};
    ast_node *Current = &Head;
    
    while(!TokenIs(Token, "}"))
    {
        Current->Next = Statement(Token, &Token);
        Current = Current->Next;
    }
    
    Result->Body = Head.Next;
    *Rest = Token->Next;
    
    return Result;
}

// Function = ID "(" ")" Statement
internal object *
Function(token *Token, token **Rest)
{
    object *Result = 0;
    
    // TODO(felipe): Improve error messages.
    if(Token->TokenType == TokenType_Identifier)
    {
        Result = calloc(1, sizeof(object));
        Result->Name = StringFromToken(Token);
        Result->Type = ObjectType_Function;
        Result->Storage = ObjectStorage_Local;
        
        Token = Token->Next;
        
        Token = AssertNext(Token, "(");
        Token = AssertNext(Token, ")");
        
        Result->Body = Statement(Token, &Token);
        Result->LocalVariables = GlobalVariablesHead.Next;
        GlobalVariablesHead.Next = 0;
        GlobalVariables = &GlobalVariablesHead;
        
        *Rest = Token;
    }
    else
    {
        ErrorInToken(Token, "expected an identifier");
    }
    
    return Result;
}

// Program = Function*
internal program *
Program(token *Token, token **Rest)
{    
    program *Result = 0;
    
    object Head = {0};
    object *Current = &Head;
    
    while(Token->TokenType != TokenType_EOF)
    {
        Current->Next = Function(Token, &Token);
        Current = Current->Next;
    }

    Result = calloc(1, sizeof(program));
    Result->Objects = Head.Next;
    
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
        
    "Equal ",
    "NotEqu",
    "LessTh",
    "LessEq",

    "Derefe",
    "Addres",

    "Assign",
    
    "ExpStm",
    "Block ",
    
    "Variab",

    "Return",
    "If    ",
    "For   ",
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
        
        switch(Node->NodeType)
        {
            case ASTNodeType_Number:
            {
                printf(" (%lld)", Node->NumericalValue);
            } break;
            
            case ASTNodeType_Variable:
            {
                printf(" (%s)", Node->Variable->Name);                
            } break;
            
//            case ASTNodeType_Function:
//            {
//                printf(" (%s)", Node->FunctionName);                
//            } break;
        }
        
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
        
        if(Node->Body)
        {
            PrintASTNode(Node->Body, Depth + 1);
        }
    }
}

internal program *
ParseTokens(token *Tokens)
{
    program *Result = 0;
    
    Result = Program(Tokens, &Tokens);
    
    // DEBUG(felipe): Print functions
    printf("\nFunctions\n");
    for(object *Object = Result->Objects;
        Object;
        Object = Object->Next)
    {
        printf("- %s\n", Object->Name);
        
        for(object *Variable = Object->LocalVariables;
            Variable;
            Variable = Variable->Next)
        {
            printf("    %s\n", Variable->Name);
        }
    }
    
    // DEBUG(felipe): Print AST node tree.
    printf("\nAST\n");
    for(object *Object = Result->Objects;
        Object;
        Object = Object->Next)
    {
        printf("%s()\n", Object->Name);
        PrintASTNode(Object->Body, 1);
    }
    
    return Result;
}
