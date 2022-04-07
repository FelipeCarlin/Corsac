#if !defined(WIN32_CORSAC_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Felipe Carlin $
   $Notice: Copyright © 2022 Felipe Carlin $
   ======================================================================== */


typedef struct loaded_file
{
    char *Filename;
    
    void *Memory;
    uint64 Size;
} loaded_file;

typedef enum token_type
{
    TokenType_Identifier,     // Identifier or keyword
    TokenType_Punctuation,    // Any other character
    
    TokenType_Number,         // Number literal
    TokenType_String,         // String literal
    
    TokenType_EOF,            // End-of-file markers
} token_type;

typedef struct token
{
    token_type TokenType;
    struct token *Next;

//    char *Filename;
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

internal void Error(char *Format, ...);
internal void ErrorInToken(token *Token, char *Format, ...);
internal void Warning(char *Format, ...);
internal void WarningInToken(token *Token, char *Format, ...);

#define WIN32_CORSAC_H
#endif
