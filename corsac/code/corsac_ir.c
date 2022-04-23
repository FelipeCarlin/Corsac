/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Felipe Carlin $
   $Notice: Copyright © 2022 Felipe Carlin $
   ======================================================================== */

global_variable uint32 GlobalDepth;

internal void
GeneratePush(void)
{
    printf("  push rax\n");
    GlobalDepth++;
}

internal void
GeneratePop(char *Argument)
{
    printf("  pop %s\n", Argument);
    GlobalDepth--;
}

// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
inline uint32
AlignTo(int32 Value, uint32 Align)
{
    uint32 Result = (Value + Align - 1) / Align * Align;
    return Result;
}

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
inline void
GenerateAddress(ast_node *Node)
{
    if(Node->NodeType == ASTNodeType_Variable)
    {
        printf("  lea rax, [rbp + %d]\n", Node->Variable->StackBaseOffset);
    }
    else
    {
        Error("not an lvalue");
    }
}

// Generate code for a given node.
internal void
GenerateExpression(ast_node *Node)
{
    switch(Node->NodeType)
    {
        case ASTNodeType_Number:
        {
            // TODO(felipe): Make numerical value storing consistent.
            printf("  mov rax, %d\n", (uint32)Node->NumericalValue);
        } break;
        
        case ASTNodeType_Negate:
        {
            GenerateExpression(Node->LeftHandSide);
            printf("  neg rax\n");
        } break;
        
        case ASTNodeType_Variable:
        {
            GenerateAddress(Node);
            printf("  mov rax, [rax]\n");
        } break;
        
        case ASTNodeType_Assign:
        {
            GenerateAddress(Node->LeftHandSide);
            GeneratePush();
            GenerateExpression(Node->RightHandSide);
            GeneratePop("rdi");
            
            printf("  mov [rdi], rax\n");
        } break;
        
        default:
        {
            GenerateExpression(Node->RightHandSide);
            GeneratePush();
            GenerateExpression(Node->LeftHandSide);
            GeneratePop("rdi");

            switch(Node->NodeType)
            {
                case ASTNodeType_Add:
                {
                    printf("  add rax, rdi\n");
                } break;
                
                case ASTNodeType_Sub:
                {
                    printf("  sub rax, rdi\n");
                } break;
                
                case ASTNodeType_Multiply:
                {
                    printf("  imul rax, rdi\n");
                } break;
                
                case ASTNodeType_Divide:
                {
                    printf("  cqo\n");
                    printf("  idiv rdi\n");
                } break;
                
                case ASTNodeType_Equal:
                case ASTNodeType_NotEqual:
                case ASTNodeType_LessThan:
                case ASTNodeType_LessEqual:
                {
                    printf("  cmp rax, rdi\n");

                    if(Node->NodeType == ASTNodeType_Equal)
                    {
                        printf("  sete al\n");
                    }
                    else if(Node->NodeType == ASTNodeType_NotEqual)
                    {
                        printf("  setne al\n");
                    }
                    else if(Node->NodeType == ASTNodeType_LessThan)
                    {
                        printf("  setl al\n");
                    }
                    else if(Node->NodeType == ASTNodeType_LessEqual)
                    {
                        printf("  setle al\n");
                    }
                    
                    printf("  movzb al, rax\n");
                } break;

                default:
                {
                    ErrorInToken(Node->Token, "invalid expression");
                } break;
            }
        } break;
    }
}

internal void
GenerateStatement(ast_node *Node)
{
    switch(Node->NodeType)
    {
        case ASTNodeType_Expression_Statement:
        {
            GenerateExpression(Node->LeftHandSide);
        } break;

        default:
        {
            ErrorInToken(Node->Token, "invalid statement");
        } break;
    }
}

#if 0
// Assign offsets to local variables.
internal void
AssignLvarOffsets(program *Program)
{
    uint32 Offset = 0;
    
    for(object *Variable = Program->Variables;
         Variable;
         Variable = Variable->Next)
    {
        Offset += 8;
        Variable->StackBaseOffset = -Offset;
    }
    
    Program->StackSize = AlignTo(Offset, 16);
}

internal void
GenerateIR(program *Program)
{
    AssignLvarOffsets(Program);
    
    printf("\nIR\n\n");
    
    printf("  section .text\n");
    printf("  global main\n");
    printf("main:\n");
    
    // Prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", Program->StackSize);
    
    for(ast_node *Node = Program->Nodes;
        Node;
        Node = Node->Next)
    {
        GenerateStatement(Node);
        Assert(GlobalDepth == 0);
    }
    
    printf(".L.return:\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}
#endif
