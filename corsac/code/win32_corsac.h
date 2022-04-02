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
    char *Name;
    
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

    char *Filename;
    
    char *Location;
    uint32 Length;

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

#define WIN32_CORSAC_H
#endif
