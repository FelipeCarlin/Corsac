/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Felipe Carlin $
   $Notice: Copyright © 2022 Felipe Carlin $
   ======================================================================== */

#include "corsac_ir.h"

global_variable memory_arena *GlobalFileArena;
global_variable uint32 GlobalDepth;

internal uint32
UniqueNumber()
{
    local_persist uint32 Number = 0;
    ++Number;
    
    return Number;
}

internal void
PushString(memory_arena *Arena, char *String, ...)
{
    va_list Arguments;
    va_start(Arguments, String);
    
    uint32 Lenght = vsnprintf(0, 0, String, Arguments) + 1;
    Assert((Arena->Size - Arena->Used) > Lenght);
    
    vsnprintf((char *)Arena->Memory + Arena->Used, Lenght, String, Arguments);
    
    Arena->Used += Lenght - 1;
    
    va_end(Arguments);
}

global_variable ir_section *GlobalText;

internal instruction *
NewInstruction(ir_section *Instructions, operation Op)
{
    instruction *Result = 0;
    if(Instructions->Instructions)
    {
        Result = Instructions->Instructions + Instructions->Count;
        Result->Operation = Op;
    }

    ++Instructions->Count;
    
    return Result;
}

internal ir_symbol *
NewSymbol(ir_section *Section, char *Name, ...)
{
    va_list Args;
    va_start(Args, Name);
    
    uint32 Lenght = vsnprintf(0, 0, Name, Args) + 1;
    
    ir_symbol *Result = 0;
    if(Section->Symbols)
    {
        // TODO(felipe): remove malloc
        char *Buffer = malloc(Lenght);
        vsprintf_s(Buffer, Lenght, Name, Args);
        
        Result = Section->Symbols + Section->SymbolCount;
        Result->Name = Buffer;
    }
    
    ++Section->SymbolCount;
    
    return Result;
}

internal ir_symbol *
NewSymbolEx(memory_arena *Arena, ir_section *Section, symbol_flags Flags, char *Name, ...)
{
    va_list Args;
    va_start(Args, Name);
    
    uint32 Lenght = vsnprintf(0, 0, Name, Args) + 1;
    
    ir_symbol *Result = 0;
    if(Section->Symbols)
    {
        // TODO(felipe): remove malloc
        char *Buffer = malloc(Lenght);
        vsprintf_s(Buffer, Lenght, Name, Args);
        
        Result = Section->Symbols + Section->SymbolCount;
        Result->Name = Buffer;
        Result->Flags = Flags;
        Result->InstructionIndex = Section->Count;
    }
    
    ++Section->SymbolCount;
    
    return Result;    
}

inline operand *
GetNextOperand(instruction *Instruction)
{
    operand *Operand = 0;
    for(uint32 Index = 0;
        Index < ArrayCount(Instruction->Operands);
        ++Index)
    {
        operand *Test = Instruction->Operands + Index;
        
        if(Test->Type == OperandType_Null)
        {
            Operand = Test;
            break;
        }
    }
    
    return Operand;
}

internal void
AddOperandRegister(ir_section *Instructions, operand_register Register)
{
    Assert(Instructions->Count);
    
    if(Instructions->Instructions)
    {
        instruction *CurrentInst = Instructions->Instructions + Instructions->Count - 1;
        
        operand *Operand = GetNextOperand(CurrentInst);
        Assert(Operand);
        
        Operand->Type = OperandType_Register;
        Operand->Register = Register;
    }
}

internal void
AddOperandRegisterMemory(ir_section *Instructions, operand_register Register)
{
    Assert(Instructions->Count);

    if(Instructions->Instructions)
    {
        instruction *CurrentInst = Instructions->Instructions + Instructions->Count - 1;
        
        operand *Operand = GetNextOperand(CurrentInst);
        Assert(Operand);
        
        Operand->Type = OperandType_RegisterMemory;
        Operand->Register = Register;
    }
}

internal void
AddOperandRegisterMemoryOffset(ir_section *Instructions, operand_register Register, uint32 Offset)
{
    Assert(Instructions->Count);

    if(Instructions->Instructions)
    {
        instruction *CurrentInst = Instructions->Instructions + Instructions->Count - 1;
        
        operand *Operand = GetNextOperand(CurrentInst);
        Assert(Operand);
    
        Operand->Type = OperandType_RegisterMemory;
        Operand->Register = Register;
        Operand->Offset = Offset;
    }
}

internal void
AddOperandImmediate(ir_section *Instructions, uint64 Immediate)
{
    Assert(Instructions->Count);

    if(Instructions->Instructions)
    {
        instruction *CurrentInst = Instructions->Instructions + Instructions->Count - 1;
        
        operand *Operand = GetNextOperand(CurrentInst);
        Assert(Operand);
        
        Operand->Type = OperandType_Immediate;
        Operand->Immediate = Immediate;
    }
}

internal void
AddOperandAddress(ir_section *Instructions, uint64 Address)
{
    Assert(Instructions->Count);

    if(Instructions->Instructions)
    {
        instruction *CurrentInst = Instructions->Instructions + Instructions->Count - 1;
        
        operand *Operand = GetNextOperand(CurrentInst);
        Assert(Operand);
        
        Operand->Type = OperandType_Address;
        Operand->Address = Address;
    }
}

internal void
AddOperandSymbol(ir_section *Instructions, ir_symbol *Symbol)
{
    Assert(Instructions->Count);
    
    if(Instructions->Instructions)
    {
        instruction *CurrentInst = Instructions->Instructions + Instructions->Count - 1;
        
        operand *Operand = GetNextOperand(CurrentInst);
        Assert(Operand);
        
        Operand->Type = OperandType_Address;
        Operand->Symbol = Symbol;
    }
}

//
//

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

internal void GenerateExpression(ast_node *Node);

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
inline void
GenerateAddress(ast_node *Node)
{
    switch(Node->NodeType)
    {
        case ASTNodeType_Variable:
        {
            NewInstruction(GlobalText, Op_Lea);
            AddOperandRegister(GlobalText, Operand_Rax);
            AddOperandRegisterMemoryOffset(GlobalText, Operand_Rbp, Node->Variable->StackBaseOffset);
            PushString(GlobalFileArena, "  lea rax, [rbp + %d]\n", (int32)Node->Variable->StackBaseOffset);
        } break;
        
        case ASTNodeType_Dereference:
        {
            GenerateExpression(Node->LeftHandSide);
        } break;
        
        default:
        {
            Error("not an lvalue");
        } break;
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
            NewInstruction(GlobalText, Op_Move);
            AddOperandRegister(GlobalText, Operand_Rax);
            AddOperandImmediate(GlobalText, Node->NumericalValue);
            PushString(GlobalFileArena, "  mov rax, %d\n", (uint32)Node->NumericalValue);
        } break;
        
        case ASTNodeType_Negate:
        {
            GenerateExpression(Node->LeftHandSide);
            
            NewInstruction(GlobalText, Op_Negate);
            AddOperandRegister(GlobalText, Operand_Rax);
            PushString(GlobalFileArena, "  neg rax\n");
        } break;
        
        case ASTNodeType_Variable:
        {
            GenerateAddress(Node);

            NewInstruction(GlobalText, Op_Move);
            AddOperandRegister(GlobalText, Operand_Rax);
            AddOperandRegisterMemory(GlobalText, Operand_Rax);
            PushString(GlobalFileArena, "  mov rax, [rax]\n");
        } break;

        case ASTNodeType_Dereference:
        {
            GenerateExpression(Node->LeftHandSide);

            NewInstruction(GlobalText, Op_Move);
            AddOperandRegister(GlobalText, Operand_Rax);
            AddOperandRegisterMemory(GlobalText, Operand_Rax);
            PushString(GlobalFileArena, "  mov rax, [rax]\n");
        } break;
        case ASTNodeType_Address:
        {
            GenerateAddress(Node->LeftHandSide);
        } break;
        
        case ASTNodeType_Assign:
        {
            GenerateAddress(Node->LeftHandSide);
            
            NewInstruction(GlobalText, Op_Push);
            AddOperandRegister(GlobalText, Operand_Rax);
            GeneratePush();
            
            GenerateExpression(Node->RightHandSide);
            
            NewInstruction(GlobalText, Op_Pop);
            AddOperandRegister(GlobalText, Operand_Rdi);
            GeneratePop("rdi");
            
            NewInstruction(GlobalText, Op_Move);
            AddOperandRegisterMemory(GlobalText, Operand_Rdi);
            AddOperandRegister(GlobalText, Operand_Rax);
            PushString(GlobalFileArena, "  mov [rdi], rax\n");
        } break;
        
        default:
        {
            GenerateExpression(Node->RightHandSide);
            
            NewInstruction(GlobalText, Op_Push);
            AddOperandRegister(GlobalText, Operand_Rax);
            GeneratePush();

            GenerateExpression(Node->LeftHandSide);

            NewInstruction(GlobalText, Op_Pop);
            AddOperandRegister(GlobalText, Operand_Rdi);
            GeneratePop("rdi");
            
            switch(Node->NodeType)
            {
                case ASTNodeType_Add:
                {
                    NewInstruction(GlobalText, Op_Add);
                    AddOperandRegister(GlobalText, Operand_Rax);
                    AddOperandRegister(GlobalText, Operand_Rdi);
                    PushString(GlobalFileArena, "  add rax, rdi\n");
                } break;
                
                case ASTNodeType_Sub:
                {
                    NewInstruction(GlobalText, Op_Sub);
                    AddOperandRegister(GlobalText, Operand_Rax);
                    AddOperandRegister(GlobalText, Operand_Rdi);
                    PushString(GlobalFileArena, "  sub rax, rdi\n");
                } break;
                
                case ASTNodeType_Multiply:
                {
                    NewInstruction(GlobalText, Op_Mul);
                    AddOperandRegister(GlobalText, Operand_Rax);
                    AddOperandRegister(GlobalText, Operand_Rdi);
                    PushString(GlobalFileArena, "  imul rax, rdi\n");
                } break;
                
                case ASTNodeType_Divide:
                {
                    NewInstruction(GlobalText, Op_ConvertQToO);
                    NewInstruction(GlobalText, Op_Div);
                    AddOperandRegister(GlobalText, Operand_Rdi);
                    PushString(GlobalFileArena, "  cqo\n");
                    PushString(GlobalFileArena, "  idiv rdi\n");
                } break;
                
                case ASTNodeType_Equal:
                case ASTNodeType_NotEqual:
                case ASTNodeType_LessThan:
                case ASTNodeType_LessEqual:
                {
                    NewInstruction(GlobalText, Op_Compare);
                    AddOperandRegister(GlobalText, Operand_Rax);
                    AddOperandRegister(GlobalText, Operand_Rdi);
                    PushString(GlobalFileArena, "  cmp rax, rdi\n");
                    
                    if(Node->NodeType == ASTNodeType_Equal)
                    {
                        NewInstruction(GlobalText, Op_SetEqual);
                        AddOperandRegister(GlobalText, Operand_Rax);
                        PushString(GlobalFileArena, "  sete al\n");
                    }
                    else if(Node->NodeType == ASTNodeType_NotEqual)
                    {
                        NewInstruction(GlobalText, Op_SetNotEqual);
                        AddOperandRegister(GlobalText, Operand_Rax);
                        PushString(GlobalFileArena, "  setne al\n");
                    }
                    else if(Node->NodeType == ASTNodeType_LessThan)
                    {
                        NewInstruction(GlobalText, Op_SetLess);
                        AddOperandRegister(GlobalText, Operand_Rax);
                        PushString(GlobalFileArena, "  setl al\n");
                    }
                    else if(Node->NodeType == ASTNodeType_LessEqual)
                    {
                        NewInstruction(GlobalText, Op_SetLessEqual);
                        AddOperandRegister(GlobalText, Operand_Rax);
                        PushString(GlobalFileArena, "  setle al\n");
                    }

                    // TODO(felipe): Sign extend and opernad size.
                    NewInstruction(GlobalText, Op_Move);
                    AddOperandRegister(GlobalText, Operand_Rax);
                    AddOperandRegister(GlobalText, Operand_Rax);
                    PushString(GlobalFileArena, "  movsx rax, al\n");
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
GenerateStatement(memory_arena *Arena, ast_node *Node)
{
    switch(Node->NodeType)
    {
        case ASTNodeType_Block:
        {
            for(ast_node *ChildNode = Node->Body;
                ChildNode;
                ChildNode = ChildNode->Next)
            {
                GenerateStatement(Arena, ChildNode);
            }
        } break;
        
        case ASTNodeType_Return:
        {
            GenerateExpression(Node->LeftHandSide);
            
            // TODO(felipe): Symbol reference.
            NewInstruction(GlobalText, Op_Jump);
            AddOperandSymbol(GlobalText, NewSymbol(GlobalText, ".L.return"));
            PushString(GlobalFileArena, "  jmp .L.return\n");
        } break;

        case ASTNodeType_If:
        {
            uint32 ID = UniqueNumber();
            GenerateExpression(Node->Condition);
            
            NewInstruction(GlobalText, Op_Compare);
            AddOperandRegister(GlobalText, Operand_Rax);
            AddOperandImmediate(GlobalText, 0);
            NewInstruction(GlobalText, Op_JumpEqual);
            AddOperandSymbol(GlobalText, NewSymbol(GlobalText, ".L.else.%d", ID));
            PushString(GlobalFileArena, "  cmp rax, 0\n");
            PushString(GlobalFileArena, "  je  .L.else.%d\n", ID);
            
            GenerateStatement(Arena, Node->Then);
            
            NewInstruction(GlobalText, Op_Jump);
            AddOperandSymbol(GlobalText, NewSymbol(GlobalText, ".L.end.%d", ID));
            PushString(GlobalFileArena, "  jmp .L.end.%d\n", ID);
            
            NewSymbolEx(Arena, GlobalText, SymbolFlag_Local, ".L.else.%d", ID);
            PushString(GlobalFileArena, ".L.else.%d:\n", ID);
            if(Node->Else)
            {
                GenerateStatement(Arena, Node->Else);
            }
            
            NewSymbolEx(Arena, GlobalText, SymbolFlag_Local, ".L.end.%d", ID);
            PushString(GlobalFileArena, ".L.end.%d:\n", ID);
        } break;
        
        case ASTNodeType_For:
        {
            uint32 ID = UniqueNumber();
            
            if(Node->Init)
            {
                GenerateStatement(Arena, Node->Init);
            }
            
            NewSymbolEx(Arena, GlobalText, SymbolFlag_Local, ".L.end.%d", ID);
            PushString(GlobalFileArena, ".L.begin.%d:\n", ID);
            if(Node->Condition)
            {
                GenerateExpression(Node->Condition);
                
                NewInstruction(GlobalText, Op_Compare);
                AddOperandRegister(GlobalText, Operand_Rax);
                AddOperandImmediate(GlobalText, 0);
                NewInstruction(GlobalText, Op_JumpEqual);
                AddOperandAddress(GlobalText, 0xffff);
                PushString(GlobalFileArena, "  cmp rax, 0\n");
                PushString(GlobalFileArena, "  je  .L.end.%d\n", ID);
            }
            
            GenerateStatement(Arena, Node->Then);
            
            if(Node->Increment)
            {
                GenerateExpression(Node->Increment);
            }

            NewInstruction(GlobalText, Op_Jump);
            AddOperandAddress(GlobalText, 0xffff);
            NewSymbolEx(Arena, GlobalText, SymbolFlag_Local, ".L.end.%d", ID);
            PushString(GlobalFileArena, "  jmp .L.begin.%d\n", ID);
            PushString(GlobalFileArena, ".L.end.%d:\n", ID);
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
PushByte(memory_arena *Arena, uint8 Byte)
{
    Assert((Arena->Size - Arena->Used) > 1);

    *((uint8 *)Arena->Memory + Arena->Used) = Byte;
    ++Arena->Used;
}

internal void
PushWord(memory_arena *Arena, uint16 Word)
{
    Assert((Arena->Size - Arena->Used) > 2);

    *(uint16 *)((uint8 *)Arena->Memory + Arena->Used) = Word;
    Arena->Used += 2;
}

internal void
PushDWord(memory_arena *Arena, uint32 DWord)
{
    Assert((Arena->Size - Arena->Used) > 4);
    
    *(uint32 *)((uint8 *)Arena->Memory + Arena->Used) = DWord;
    Arena->Used += 4;
}

typedef enum x64_register
{
    OperandRegister_RAX,
    OperandRegister_RCX,
    OperandRegister_RDX,
    OperandRegister_RBX,
        
    OperandRegister_RSP,
    OperandRegister_RBP,
        
    OperandRegister_RSI,
    OperandRegister_RDI,
        
    OperandRegister_R8,
    OperandRegister_R9,
    OperandRegister_R10,
    OperandRegister_R11,
    OperandRegister_R12,
    OperandRegister_R13,
    OperandRegister_R14,
    OperandRegister_R15,
} operand_register;
    
uint8 x64Registers[] =
{
    OperandRegister_RAX,
    OperandRegister_RBX,
    OperandRegister_RCX,
    OperandRegister_RDX,
        
    OperandRegister_RSI,
    OperandRegister_RDI,
        
    OperandRegister_RSP,
    OperandRegister_RBP,
};

internal void
MROperation(memory_arena *Arena, instruction *Instruction, uint8 Opcode)
{
    uint8 RegisterA = x64Registers[Instruction->Operands[0].Register];
    uint8 RegisterB = x64Registers[Instruction->Operands[1].Register];

    uint8 Mode = 0;
    if(Instruction->Operands[0].Type == OperandType_Register)
    {
        Mode = 0b11;
    }
    else if(Instruction->Operands[0].Type == OperandType_RegisterMemory)
    {
        Mode = 0b00;        
    }
    
    PushByte(Arena, Opcode);
    
    mod_rm_byte ModRM = {0};
    ModRM.RM = RegisterA;
    ModRM.Reg = RegisterB;
    ModRM.Mod = Mode;
    
    PushByte(Arena, *(uint8 *)&ModRM);
}

internal void
RMOperation(memory_arena *Arena, instruction *Instruction, uint8 Opcode)
{
    uint8 RegisterA = x64Registers[Instruction->Operands[0].Register];
    uint8 RegisterB = x64Registers[Instruction->Operands[1].Register];
    
    uint32 Offset = Instruction->Operands[1].Offset;
    
    uint8 Mode = 0;
    if(Instruction->Operands[1].Type == OperandType_Register)
    {
        Mode = 0b11;
    }
    else if(Instruction->Operands[1].Type == OperandType_RegisterMemory)
    {
        if(Offset == 0)
        {
            Mode = 0b00;
        }
        else
        {
            // [reg + disp32]
            Mode = 0b10;
        }
    }
    
    PushByte(Arena, Opcode);
    
    mod_rm_byte ModRM = {0};
    ModRM.Reg = RegisterA;
    ModRM.RM = RegisterB;
    ModRM.Mod = Mode;

    PushByte(Arena, *(uint8 *)&ModRM);
    
    if(Mode == 0b10)
    {
        PushDWord(Arena, Offset);
    }
}

internal void
MIOperation(memory_arena *Arena, instruction *Instruction, uint8 Opcode, uint8 RegValue)
{
    uint8 Register = x64Registers[Instruction->Operands[0].Register];
    uint32 Immediate = Instruction->Operands[1].Immediate;

    uint32 Offset = Instruction->Operands[0].Offset;
    
    uint8 Mode = 0;
    if(Instruction->Operands[0].Type == OperandType_Register)
    {
        Mode = 0b11;
    }
    else if(Instruction->Operands[0].Type == OperandType_RegisterMemory)
    {
        if(Offset == 0)
        {
            Mode = 0b00;
        }
        else
        {
            // [reg + disp32]
            Mode = 0b10;
        }
    }
    
    PushByte(Arena, Opcode);
    
    mod_rm_byte ModRM = {0};
    // TODO(felipe): This might need to be nonzero.
    ModRM.Reg = RegValue;
    ModRM.RM = Register;
    ModRM.Mod = Mode;
    
    PushByte(Arena, *(uint8 *)&ModRM);
    
    if(Mode == 0b10)
    {
        PushDWord(Arena, Offset);
    }
    
    PushDWord(Arena, Immediate);
}

internal void
GenerateIR(program *Program)
{
    // TODO(felipe): Resize arena as needed.
    memory_arena FileArena = Win32AllocateArena(Megabytes(1));
    GlobalFileArena = &FileArena;
    
    AssignLvarOffsets(Program);
    
    memory_arena MainArena = Win32AllocateArena(Megabytes(1));
    
    GlobalText = calloc(1, sizeof(ir_section));
    GlobalText->Name = ".text";
    PushString(GlobalFileArena, "  section .text\n");
    
    // TODO(felipe): Pre-initialized symbols. (externals, globals, locals).
    for(uint32 Pass = 0;
        Pass < 2;
        ++Pass)
    {
        if(Pass == 1)
        {
            GlobalText->Instructions = calloc(GlobalText->Count, sizeof(instruction));
            GlobalText->Count = 0;
            
            GlobalText->Symbols = calloc(GlobalText->SymbolCount, sizeof(ir_symbol));
            GlobalText->SymbolCount = 0;
        }
        
        NewSymbolEx(&MainArena, GlobalText, SymbolFlag_Global, "main");
        PushString(GlobalFileArena, "  global main\n");
        
        // NOTE(felipe): The loop is run for every function.
        for(object *Object = Program->Objects;
            Object;
            Object = Object->Next)
        {
            NewSymbolEx(&MainArena, GlobalText, SymbolFlag_Local, Object->Name);
            PushString(GlobalFileArena, "%s:\n", Object->Name);
            
            // Prologue
            NewInstruction(GlobalText, Op_Push);
            AddOperandRegister(GlobalText, Operand_Rbp);
            NewInstruction(GlobalText, Op_Move);
            AddOperandRegister(GlobalText, Operand_Rbp);
            AddOperandRegister(GlobalText, Operand_Rsp);
            NewInstruction(GlobalText, Op_Sub);
            AddOperandRegister(GlobalText, Operand_Rsp);
            AddOperandImmediate(GlobalText, Object->StackSize);        
            PushString(GlobalFileArena, "  push rbp\n");
            PushString(GlobalFileArena, "  mov rbp, rsp\n");
            PushString(GlobalFileArena, "  sub rsp, %d\n", Object->StackSize);
            
            for(ast_node *Node = Object->Body;
                Node;
                Node = Node->Next)
            {
                GenerateStatement(&MainArena, Node);
                Assert(GlobalDepth == 0);
            }
            
            NewSymbolEx(&MainArena, GlobalText, SymbolFlag_Local, ".L.return");
            NewInstruction(GlobalText, Op_Move);
            AddOperandRegister(GlobalText, Operand_Rsp);
            AddOperandRegister(GlobalText, Operand_Rbp);
            NewInstruction(GlobalText, Op_Pop);
            AddOperandRegister(GlobalText, Operand_Rbp);
            NewInstruction(GlobalText, Op_Ret);
            PushString(GlobalFileArena, ".L.return:\n");
            PushString(GlobalFileArena, "  mov rsp, rbp\n");
            PushString(GlobalFileArena, "  pop rbp\n");
            PushString(GlobalFileArena, "  ret\n");
        }
    }
    
    Win32WriteEntireFile("main.asm", FileArena.Memory, FileArena.Used);
    
    
    // NOTE(felipe): x64 assembler.
    uint32 SectionStart = MainArena.Used;
    uint32 SymbolIndex = 0;
    for(uint32 Index = 0;
        Index < GlobalText->Count;
        ++Index)
    {
        instruction *Instruction = GlobalText->Instructions + Index;
        operand *Operands = Instruction->Operands;
        
        while(SymbolIndex < GlobalText->SymbolCount && (GlobalText->Symbols[SymbolIndex].InstructionIndex == Index ||
                                                        GlobalText->Symbols[SymbolIndex].Flags == SymbolFlag_Unresolved))
        {
            ir_symbol *LocalSymbol = GlobalText->Symbols + SymbolIndex;
            
            if(LocalSymbol->Flags & SymbolFlag_Local)
            {
                LocalSymbol->Offset = MainArena.Used - SectionStart;
            }
            
            ++SymbolIndex;
        }
        
        switch(Instruction->Operation)
        {
            // TODO(felipe): Sign extend.
            case Op_Move:
            {
                if(Operands[0].Type == OperandType_Register &&
                   Operands[1].Type == OperandType_Register)
                {
                    MROperation(&MainArena, Instruction, 0x89);
                }
                else if(Operands[0].Type == OperandType_RegisterMemory &&
                        Operands[1].Type == OperandType_Register)
                {
                    MROperation(&MainArena, Instruction, 0x89);
                }
                else if(Operands[0].Type == OperandType_Register &&
                        Operands[1].Type == OperandType_RegisterMemory)
                {
                    RMOperation(&MainArena, Instruction, 0x8b);
                }
                else if(Operands[0].Type == OperandType_Register &&
                        Operands[1].Type == OperandType_Immediate)
                {
                    // TODO(felipe): Compress based on immediate size.
                    MIOperation(&MainArena, Instruction, 0xc7, 0);
                }
            } break;
            
            case Op_Lea:
            {
                if(Operands[0].Type == OperandType_Register &&
                   Operands[1].Type == OperandType_RegisterMemory)
                {
                    RMOperation(&MainArena, Instruction, 0x8d);
                }
            } break;
            
            case Op_Push:
            {
                if(Operands[0].Type == OperandType_Register)
                {
                    uint8 Register = x64Registers[Instruction->Operands[0].Register];
                    PushByte(&MainArena, 0x50 + Register);
                }
            } break;
            
            case Op_Pop:
            {
                if(Operands[0].Type == OperandType_Register)
                {
                    uint8 Register = x64Registers[Instruction->Operands[0].Register];
                    PushByte(&MainArena, 0x58 + Register);
                }
            } break;
            
            case Op_Add:
            {
                if(Operands[0].Type == OperandType_Register &&
                   Operands[1].Type == OperandType_Immediate)
                {
                    MIOperation(&MainArena, Instruction, 0x81, 0);
                }
            } break;
            
            case Op_Sub:
            {
                if(Operands[0].Type == OperandType_Register &&
                   Operands[1].Type == OperandType_Immediate)
                {
                    MIOperation(&MainArena, Instruction, 0x81, 5);
                }
            } break;
            
            case Op_Mul:
            {
                if(Operands[0].Type == OperandType_Register &&
                   Operands[1].Type == OperandType_Register)
                {
                    PushByte(&MainArena, 0x0f);
                    RMOperation(&MainArena, Instruction, 0xaf);
                }
            } break;

            case Op_Compare:
            {
                if(Operands[0].Type == OperandType_Register &&
                   Operands[1].Type == OperandType_Register)
                {
                    RMOperation(&MainArena, Instruction, 0x3b);
                }
            } break;

            case Op_SetEqual:
            {
                if(Operands[0].Type == OperandType_Register)
                {
                    uint8 Register = x64Registers[Instruction->Operands[0].Register];
                    
                    PushByte(&MainArena, 0x0f);
                    PushByte(&MainArena, 0x94);
                    
                    mod_rm_byte ModRM = {0};
                    // TODO(felipe): This might need to be nonzero.
                    ModRM.Reg = 0;
                    ModRM.RM = Register;
                    ModRM.Mod = 0b11;
                    
                    PushByte(&MainArena, *(uint8 *)&ModRM);
                }
            } break;
            
            case Op_Jump:
            {
                if(Operands[0].Type == OperandType_Address)
                {
                    PushByte(&MainArena, 0xe9);
                    
                    Operands[0].Symbol->Offset = MainArena.Used - SectionStart;
                    
                    PushDWord(&MainArena, 0x00);
                }
            } break;
            
            case Op_JumpEqual:
            {
                if(Operands[0].Type == OperandType_Address)
                {
                    PushByte(&MainArena, 0x0f);
                    PushByte(&MainArena, 0x84);
                    
                    Operands[0].Symbol->Offset = MainArena.Used - SectionStart;
                    
                    PushDWord(&MainArena, 0x00);
                }
            } break;
            
            case Op_Ret:
            {
                PushByte(&MainArena, 0xc3);
            } break;
            
            default:
            {
                // TODO(felipe): Invalid instruction
                PushByte(&MainArena, 0xcc);                
            } break;
        }
    }
    
    // NOTE(felipe): Resolve defined symbols
    for(uint32 Index = 0;
        Index < GlobalText->SymbolCount;
        ++Index)
    {
        ir_symbol *Symbol = GlobalText->Symbols + Index;
        
        if(Symbol->Flags == SymbolFlag_Unresolved)
        {
            ir_symbol *FoundSymbol = 0;
            for(uint32 TestIndex = 0;
                TestIndex < GlobalText->SymbolCount;
                ++TestIndex)
            {
                ir_symbol *TestSymbol = GlobalText->Symbols + TestIndex;
                if(TestSymbol != Symbol && !StringCompare(Symbol->Name, TestSymbol->Name, StringLength(TestSymbol->Name)) &&
                   (TestSymbol->Flags & SymbolFlag_Local || TestSymbol->Flags & SymbolFlag_Global))
                {
                    FoundSymbol = TestSymbol;
                    break;
                }
            }
            
            if(FoundSymbol)
            {
                // TODO(felipe): Types of relocations.
                uint32 *PatchPointer = (uint32 *)((uint8 *)MainArena.Memory + Symbol->Offset);
                int32 PatchValue = FoundSymbol->Offset - Symbol->Offset - 4;
                
                *PatchPointer = PatchValue;
            }
            else
            {
                Error("Undefined symbol: %s", Symbol->Name);
            }
        }
    }
    
    Win32WriteEntireFile("main.bin", MainArena.Memory, MainArena.Used);
    
    // NOTE(felipe): Merge Matching globals and locals.
    for(uint32 Index = 0;
        Index < GlobalText->SymbolCount;
        ++Index)
    {
        ir_symbol *Symbol = GlobalText->Symbols + Index;
                
        if(Symbol->Flags == SymbolFlag_Global)
        {
            for(uint32 TestIndex = 0;
                TestIndex < GlobalText->SymbolCount;
                ++TestIndex)
            {
                ir_symbol *TestSymbol = GlobalText->Symbols + TestIndex;
                
                if(!StringCompare(Symbol->Name, TestSymbol->Name, StringLength(TestSymbol->Name)) &&
                   TestSymbol->Flags == SymbolFlag_Local)
                {
                    Symbol->Offset = TestSymbol->Offset;
                    TestSymbol->Flags = SymbolFlag_Ignore;

                    break;
                }
            }
        }
    }
    
    section Section = {0};
    Section.Name = ".text";
    Section.BufferLength = MainArena.Used;
    Section.Buffer = MainArena.Memory;
    
    for(uint32 Index = 0;
        Index < GlobalText->SymbolCount;
        ++Index)
    {
        ir_symbol *Symbol = GlobalText->Symbols + Index;
        
        if(Symbol->Flags == SymbolFlag_Local || Symbol->Flags == SymbolFlag_Global)
        {
            ++Section.SymbolCount;            
        }
    }
    
    uint32 InsertedSymbolCount = 0; 
    Section.Symbols = malloc(Section.SymbolCount*sizeof(symbol));    
    
    for(uint32 Index = 0;
        Index < GlobalText->SymbolCount;
        ++Index)
    {
        ir_symbol *IRSymbol = GlobalText->Symbols + Index;
        
        if(IRSymbol->Flags == SymbolFlag_Local || IRSymbol->Flags == SymbolFlag_Global)
        {
            symbol *Symbol = Section.Symbols + InsertedSymbolCount++;
            
            Symbol->Name = IRSymbol->Name;
            Symbol->Flags = IRSymbol->Flags;
            Symbol->OffsetInSection = IRSymbol->Offset;
        }
    }
    
    
    //
    // Coff output
    //
    
    
    // NOTE(felipe): COFF Header.
    memory_arena CoffArena = Win32AllocateArena(Megabytes(1));
    coff_header *Header = PushStruct(&CoffArena, coff_header);
    Header->Machine = COFF_MACHINE_AMD64;
    Header->NumberOfSections = 1;
    Header->TimeDateStamp = Win32GetTime();
    Header->PointerToSymbolTable = 0;
    Header->NumberOfSymbols = Section.SymbolCount;
    Header->SizeOfOptionalHeader = 0;
    Header->Characteristics = 0;
    
    
    // NOTE(felipe): Section Headers.
    coff_section_header *SectionHeader = PushStruct(&CoffArena, coff_section_header);
    
    uint32 NameLength = StringLength(Section.Name);
    if(NameLength <= 8)
    {
        for(uint32 Index = 0;
            Index < NameLength;
            ++Index)
        {
            *(SectionHeader->Name + Index) = *(Section.Name + Index);
        }                
    }
    else
    {
//        Error(State, false, "Section name more than 8 characters long");
#if 0
        // TODO(felipe): Make this work.
        SectionHeader->Name[0] = 0;
        Entry->NameOffset = StringTableUsed + 4;
        char *Memory = (char *)PushSize(&State->StringArena, Symbol->Length + 1);
        MemCopy(Memory, Symbol->Label, Symbol->Length + 1);
        Memory[Symbol->Length] = 0;
        StringTableUsed += Symbol->Length + 1;
#endif
    }
    
    SectionHeader->VirtualSize = 0;
    SectionHeader->VirtualAddress = 0;
    SectionHeader->SizeOfRawData = Section.BufferLength;  
//    SectionHeader->PointerToRawData = 0x3c;
    SectionHeader->PointerToRelocation = 0;
    SectionHeader->PointerToLinenumbers = 0;
    SectionHeader->NumberOfRelocations = 0;
    SectionHeader->NumberOfLinenumbers = 0;
    SectionHeader->Flags = IMAGE_SCN_CNT_CODE|IMAGE_SCN_ALIGN_1BYTES|IMAGE_SCN_ALIGN_8BYTES|
        IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ;
    
    SectionHeader->PointerToRawData = CoffArena.Used;    
    uint8 *RawMemory = PushSize(&CoffArena, Section.BufferLength);
    MemCopy(RawMemory, Section.Buffer, Section.BufferLength);
    
    
    Header->PointerToSymbolTable = CoffArena.Used;
    
    // IMPORTANT(felipe): Hardcoded stuff yay!
#if 0
    coff_symbol_table_entry *Entry = PushStruct(&CoffArena, coff_symbol_table_entry);
    MemCopy(Entry->Name, ".text", 6);
    Entry->Value = 0;
    Entry->SectionNumber = 1;
    Entry->Type.MSB = 0x00;
    Entry->Type.LSB = 0x00;
    Entry->StorageClass = IMAGE_SYM_CLASS_STATIC;
    Entry->NumberOfAuxSymbols = 1;
    uint32 *Size = (uint32 *)PushStruct(&CoffArena, coff_symbol_table_entry);
    *Size = Section.BufferLength;
    
    Entry = PushStruct(&CoffArena, coff_symbol_table_entry);
    MemCopy(Entry->Name, ".absolut", 8);
    Entry->Value = 0;
    Entry->SectionNumber = -1;
    Entry->Type.MSB = 0x00;
    Entry->Type.LSB = 0x00;
    Entry->StorageClass = IMAGE_SYM_CLASS_STATIC;
    Entry->NumberOfAuxSymbols = 0;
#endif
    
    for(uint32 Index = 0;
        Index < Section.SymbolCount;
        ++Index)
    {
        symbol *Symbol = Section.Symbols + Index;
        
        coff_symbol_table_entry *Entry = PushStruct(&CoffArena, coff_symbol_table_entry);
        MemCopy(Entry->Name, Symbol->Name, StringLength(Symbol->Name));
        Entry->Value = Symbol->OffsetInSection;
        Entry->SectionNumber = 1;
        Entry->Type.MSB = 0x00;
        Entry->Type.LSB = 0x00;
        
        if(Symbol->Flags == SymbolFlag_Global)
        {
            Entry->StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
        }
        else
        {
            Entry->StorageClass = IMAGE_SYM_CLASS_STATIC;
        }
        
        Entry->NumberOfAuxSymbols = 0;
    }
    
#if 0
    // NOTE(felipe): Symbol table.
    for(uint32 Index = 0;
        Index < Section->SymbolCount;
        ++Index)
    {
        symbol_table_entry *Symbol = SymbolTable + Index;

        switch(Symbol->Type)
        {
            case SymbolType_FileName:
            {
                coff_symbol_table_entry *Entry = PushStruct(&State->ObjectArena, coff_symbol_table_entry);

                MemCopy(Entry->Name, Symbol->Name, 8);
                
                Entry->SectionNumber = Symbol->SectionNumber;
                Entry->Type.LSB = 0;
                Entry->Type.MSB = 0;
                Entry->StorageClass = IMAGE_SYM_CLASS_FILE;
                Entry->NumberOfAuxSymbols = 1;

                char *SectionEntry = (char *)PushStruct(&State->ObjectArena, coff_symbol_table_entry);

                Assert(StringLength(Symbol->Filename) < 18);
                MemCopy(SectionEntry, Symbol->Filename, StringLength(Symbol->Filename));                
            } break;

            case SymbolType_Section:
            {
                coff_symbol_table_entry *Entry = PushStruct(&State->ObjectArena, coff_symbol_table_entry);

                MemCopy(Entry->Name, Symbol->Name, 8);
                
                Entry->SectionNumber = Symbol->SectionNumber;
                Entry->Type.LSB = 0;
                Entry->Type.MSB = 0;
                Entry->StorageClass = IMAGE_SYM_CLASS_STATIC;
                Entry->NumberOfAuxSymbols = 1;

                coff_symbol_table_entry_aux_section *SectionEntry = PushStruct(&State->ObjectArena, coff_symbol_table_entry_aux_section);                
                
                SectionEntry->Length = Symbol->Length;
                SectionEntry->NumberOfRelocations = Symbol->RelocationCount;
                SectionEntry->NumberOfLinenumbers = Symbol->LineNumberCount;
                SectionEntry->CheckSum = Symbol->CheckSum;
            } break;

            default:
            {
                coff_symbol_table_entry *Entry = PushStruct(&State->ObjectArena, coff_symbol_table_entry);
            
                MemCopy(Entry->Name, Symbol->Name, 8);
                
                Entry->Value = Symbol->Offset;
                Entry->SectionNumber = Symbol->SectionNumber;
                Entry->Type.MSB = 0x00;
                Entry->Type.LSB = 0x00;
            
                if(Symbol->Type == SymbolType_External ||
                   Symbol->Type == SymbolType_Global)
                {
                    Entry->StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
                }
                else
                {
                    Entry->StorageClass = IMAGE_SYM_CLASS_STATIC;
                }
        
                Entry->NumberOfAuxSymbols = 0;
            } break;
        }
    }
#endif

    uint32 *StringTableSize = PushStruct(&CoffArena, uint32);
    *StringTableSize = 4;
    
    Win32WriteEntireFile("main.obj", CoffArena.Memory, CoffArena.Used);
}
