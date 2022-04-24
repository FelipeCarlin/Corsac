/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Felipe Carlin $
   $Notice: Copyright © 2022 Felipe Carlin $
   ======================================================================== */

global_variable memory_arena *GlobalFileArena;
global_variable uint32 GlobalDepth;

internal void
PushString(memory_arena *Arena, char *String, ...)
{
    va_list Arguments;
    va_start(Arguments, String);
    
    uint32 Lenght = vsnprintf(0, 0, String, Arguments) + 1;
    Assert((Arena->Size - Arena->Used) > Lenght);    

    vsnprintf((char *)Arena->Memory + Arena->Used, Lenght, String, Arguments);

    Arena->Used += Lenght;
    
    va_end(Arguments);
}

internal void
GeneratePush(void)
{
    PushString(GlobalFileArena, "  push rax\n");
    GlobalDepth++;
}

internal void
GeneratePop(char *Argument)
{
    PushString(GlobalFileArena, "  pop %s\n", Argument);
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
        PushString(GlobalFileArena, "  lea rax, [rbp + %d]\n", Node->Variable->StackBaseOffset);
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
            PushString(GlobalFileArena, "  mov rax, %d\n", (uint32)Node->NumericalValue);
        } break;
        
        case ASTNodeType_Negate:
        {
            GenerateExpression(Node->LeftHandSide);
            PushString(GlobalFileArena, "  neg rax\n");
        } break;
        
        case ASTNodeType_Variable:
        {
            GenerateAddress(Node);
            PushString(GlobalFileArena, "  mov rax, [rax]\n");
        } break;
        
        case ASTNodeType_Assign:
        {
            GenerateAddress(Node->LeftHandSide);
            GeneratePush();
            GenerateExpression(Node->RightHandSide);
            GeneratePop("rdi");
            
            PushString(GlobalFileArena, "  mov [rdi], rax\n");
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
                    PushString(GlobalFileArena, "  add rax, rdi\n");
                } break;
                
                case ASTNodeType_Sub:
                {
                    PushString(GlobalFileArena, "  sub rax, rdi\n");
                } break;
                
                case ASTNodeType_Multiply:
                {
                    PushString(GlobalFileArena, "  imul rax, rdi\n");
                } break;
                
                case ASTNodeType_Divide:
                {
                    PushString(GlobalFileArena, "  cqo\n");
                    PushString(GlobalFileArena, "  idiv rdi\n");
                } break;
                
                case ASTNodeType_Equal:
                case ASTNodeType_NotEqual:
                case ASTNodeType_LessThan:
                case ASTNodeType_LessEqual:
                {
                    PushString(GlobalFileArena, "  cmp rax, rdi\n");

                    if(Node->NodeType == ASTNodeType_Equal)
                    {
                        PushString(GlobalFileArena, "  sete al\n");
                    }
                    else if(Node->NodeType == ASTNodeType_NotEqual)
                    {
                        PushString(GlobalFileArena, "  setne al\n");
                    }
                    else if(Node->NodeType == ASTNodeType_LessThan)
                    {
                        PushString(GlobalFileArena, "  setl al\n");
                    }
                    else if(Node->NodeType == ASTNodeType_LessEqual)
                    {
                        PushString(GlobalFileArena, "  setle al\n");
                    }
                    
                    PushString(GlobalFileArena, "  movzb al, rax\n");
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
        case ASTNodeType_Block:
        {
            for(ast_node *ChildNode = Node->Body;
                ChildNode;
                ChildNode = ChildNode->Next)
            {
                GenerateStatement(ChildNode);
            }
        } break;
        
        case ASTNodeType_Return:
        {
            GenerateExpression(Node->LeftHandSide);
            PushString(GlobalFileArena, "  jmp .L.return\n");
        } break;
        
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

// Assign offsets to local variables.
internal void
AssignLvarOffsets(program *Program)
{
    for(object *Object = Program->Objects;
        Object;
        Object = Object->Next)
    {
        uint32 Offset = 0;
        
        for(object *Variable = Object->LocalVariables;
            Variable;
            Variable = Variable->Next)
        {
            Offset += 8;
            Variable->StackBaseOffset = -Offset;
        }
        
        Object->StackSize = AlignTo(Offset, 16);
    }
}

internal void
GenerateIR(program *Program)
{
    // TODO(felipe): Resize arena as needed.
    memory_arena FileArena = Win32AllocateArena(Megabytes(1));
    GlobalFileArena = &FileArena;
    
    AssignLvarOffsets(Program);
    
    PushString(GlobalFileArena, "  section .text\n");
    PushString(GlobalFileArena, "  global main\n");
    PushString(GlobalFileArena, "main:\n");
    
    for(object *Object = Program->Objects;
        Object;
        Object = Object->Next)
    {
        // Prologue
        PushString(GlobalFileArena, "  push rbp\n");
        PushString(GlobalFileArena, "  mov rbp, rsp\n");
        PushString(GlobalFileArena, "  sub rsp, %d\n", Object->StackSize);
        
        for(ast_node *Node = Object->Body;
            Node;
            Node = Node->Next)
        {
            GenerateStatement(Node);
            Assert(GlobalDepth == 0);
        }
        
        PushString(GlobalFileArena, ".L.return:\n");
        PushString(GlobalFileArena, "  mov rsp, rbp\n");
        PushString(GlobalFileArena, "  pop rbp\n");
        PushString(GlobalFileArena, "  ret\n");
    }
    
    Win32WriteEntireFile("main.asm", FileArena.Memory, FileArena.Used);
}
