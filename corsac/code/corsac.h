#if !defined(CORSAC_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Felipe Carlin $
   $Notice: Copyright © 2022 Felipe Carlin $
   ======================================================================== */

#define internal static
#define local_persist static
#define global_variable static

#define true 1
#define false 0

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef int32 bool32;

typedef size_t memory_index;

#if CORSAC_SLOW
    #define Assert(X) if(!(X)) {*(int *)0 = 0;}
#else
    #define Assert(X)
#endif

#define InvalidCodePath Assert(!"InvalidCodePath")

#define ArrayCount(A) (sizeof(A) / sizeof((A)[0]))

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

typedef struct loaded_file
{
    char *Filename;
    
    void *Memory;
    uint64 Size;
} loaded_file;

typedef enum token_type
{
    TokenType_Identifier,
    TokenType_Punctuation,    // Any character other than (a...z, A...Z, 0...9)
    
    TokenType_Keyword,
    
    TokenType_Number,         // Number literal
    
    TokenType_EOF,            // End-of-file markers
} token_type;

typedef struct token
{
    token_type TokenType;
    struct token *Next;
    
    loaded_file *SourceFile;
    
    char *Location;
    uint32 Length;
    bool32 AtBeginningOfLine;
    
    uint64 NumericalValue;
} token;

inline uint32
SafeTruncateUInt64(uint64 Value)
{
    // TODO(felipe): Defines for maximum values.
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return Result;
}

inline void
MemCopy(void *Destination, void *Source, uint32 Size)
{
    // TODO(felipe): Max 4 Gigs.
    for(uint32 Index = 0;
        Index < Size;
        ++Index)
    {
        *((uint8 *)Destination + Index) = *((uint8 *)Source + Index);
    }
}

inline uint32
StringLength(char *String)
{
    uint32 Result = 0;
    if(String)
    {
        for(char *Character = String;
            *Character;
            ++Character)
        {
            ++Result;
        }
    }
    
    return Result;
}

inline char *
StringDuplicate(char *String, uint32 Length)
{
    char *Result = 0;
    
    char *Destination = (char *)malloc(Length + 1);
    
    if(Destination)
    {
        // NOTE(felipe): Null termination.
        Destination[Length] = 0;
        MemCopy(Destination, String, Length);
        Result = Destination;
    }
    
    return Result;
}

inline int32
StringCompare(char *StringA, char *StringB, uint32 Bytes)
{
    int32 Result = 0;

    // NOTE(felipe): chars comparison must be done un unsigned chars
    // because the difference has to be done using two's complement.
    uint8 CharA;
    uint8 CharB;

    while(Bytes > 0)
    {
        CharA = (uint8) *StringA++;
        CharB = (uint8) *StringB++;
        if(CharA != CharB)
        {
            Result = CharA - CharB;
            break;
        }
        
        if(CharA == '\0')
        {
            break;
        }
        
        --Bytes;
    }
    
    return Result;
}

#define CORSAC_H
#endif
