#if !defined(CORSAC_IR_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Felipe Carlin $
   $Notice: Copyright © 2022 Felipe Carlin $
   ======================================================================== */

/*
  ModRM byte:
  Mod  reg  r/m
  00   000  000
  
  - mod:
  00 [reg]
  01 [reg + disp8]
  10 [reg + disp32]
  11 reg
  
  - reg:
  <operand_register>
  
  - r/m:
  <operand_register> disp
*/

typedef enum symbol_flags
{
    SymbolFlag_Unresolved = 0,
    SymbolFlag_Local,
    SymbolFlag_Global,

    SymbolFlag_Ignore,
} symbol_flags;

typedef struct ir_symbol
{
    char *Name;
    symbol_flags Flags;
    uint32 Offset;
    
    uint32 InstructionIndex;
} ir_symbol;

typedef enum operand_type
{
    OperandType_Null,
    
    OperandType_Register,
    OperandType_RegisterMemory,
    
    OperandType_Immediate,
    
    OperandType_Address,  // For numerical addresses.
    OperandType_Symbol,
} operand_type;

typedef enum operand_register
{
    Operand_Rax,
    Operand_Rbx,
    Operand_Rcx,
    Operand_Rdx,
    
    Operand_Rsi,
    Operand_Rdi,
    
    Operand_Rsp,
    Operand_Rbp,
} operand_register;

typedef struct operand
{
    operand_type Type;
    
    union
    {
        struct
        {
            operand_register Register;
            uint32 Offset;
        };
        uint64 Immediate;
        uint64 Address;
        ir_symbol *Symbol;
    };
} operand;

typedef enum operation
{
    Op_Null,
    
    Op_Move,
    Op_Lea,

    Op_Push,
    Op_Pop,
    
    Op_Add,
    Op_Sub,
    Op_Mul,
    Op_Div,

    Op_Negate,
    
    Op_Jump,
    Op_JumpEqual,
    Op_Call,
    Op_Ret,
    
    Op_Compare,
    Op_SetEqual,
    Op_SetNotEqual,
    Op_SetLess,
    Op_SetLessEqual,
    
    Op_ConvertQToO,
} operation;

typedef struct instruction
{
    operation Operation;
    
    operand Operands[2];
} instruction;

typedef struct ir_section
{
    char *Name;
    
    uint32 Count;
    instruction *Instructions;
    
    uint32 SymbolCount;
    ir_symbol *Symbols;
} ir_section;

//
// x86
//

typedef struct mod_rm_byte
{
    uint8 RM     : 3;
    uint8 Reg    : 3;
    uint8 Mod    : 2;
} mod_rm_byte;

//
//
//

typedef struct symbol
{
    char *Name;
    symbol_flags Flags;
    
    uint32 OffsetInSection;
} symbol;

typedef struct section
{
    char *Name;
    
    uint32 SymbolCount;
    symbol *Symbols;

    uint32 BufferLength;
    uint8 *Buffer;
} section;

//
// ms coff
//

enum image_file_machine
{
    COFF_MACHINE_UNKNOWN   = 0x0,
    COFF_MACHINE_AM33      = 0x1d3,
    COFF_MACHINE_AMD64     = 0x8664,
    COFF_MACHINE_ARM       = 0x1c0,
    COFF_MACHINE_ARM64     = 0xaa64,
    COFF_MACHINE_ARMNT     = 0x1c4,
    COFF_MACHINE_EBC       = 0xebc,
    COFF_MACHINE_I386      = 0x14c,
    COFF_MACHINE_IA64      = 0x200,
    COFF_MACHINE_M32R      = 0x9041,
    COFF_MACHINE_MIPS16    = 0x266,
    COFF_MACHINE_MIPSFPU   = 0x366,
    COFF_MACHINE_MIPSFPU16 = 0x466,
    COFF_MACHINE_POWERPC   = 0x1f0,
    COFF_MACHINE_POWERPCFP = 0x1f1,
    COFF_MACHINE_R4000     = 0x166,
    COFF_MACHINE_RISCV32   = 0x5032,
    COFF_MACHINE_RISCV64   = 0x5064,
    COFF_MACHINE_RISCV128  = 0x5128,
    COFF_MACHINE_SH3       = 0x1a2,
    COFF_MACHINE_SH3DSP    = 0x1a3,
    COFF_MACHINE_SH4       = 0x1a6,
    COFF_MACHINE_SH5       = 0x1a8,
    COFF_MACHINE_THUMB     = 0x1c2,
    COFF_MACHINE_WCEMIPSV2 = 0x169,
};

#pragma pack(push, 1)
typedef struct coff_header
{
    uint16 Machine;
    
    uint16 NumberOfSections;
    int32 TimeDateStamp;
    
    int32 PointerToSymbolTable;
    int32 NumberOfSymbols;
    
    uint16 SizeOfOptionalHeader;
    
    uint16 Characteristics;
} coff_header;

typedef struct coff_section_header
{
    char Name[8];

    uint32 VirtualSize;
    uint32 VirtualAddress;

    uint32 SizeOfRawData;
    uint32 PointerToRawData;

    uint32 PointerToRelocation;
    uint32 PointerToLinenumbers;
    uint16 NumberOfRelocations;
    uint16 NumberOfLinenumbers;

    uint32 Flags;
} coff_section_header;

typedef struct coff_symbol_type
{
    uint8 LSB;
    uint8 MSB;
} coff_symbol_type;

typedef struct coff_symbol_table_entry
{
    union
    {
        char Name[8];
        struct
        {
            uint32 Unused0_;
            uint32 NameOffset;
        };
    };

    uint32 Value;

    uint16 SectionNumber;
    coff_symbol_type Type;
    uint8 StorageClass;

    uint8 NumberOfAuxSymbols;
} coff_symbol_table_entry;
#pragma pack(pop)

#define CORSAC_IR_H
#endif
