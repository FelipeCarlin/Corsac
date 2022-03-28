#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef int32 bool32;

#if CORSAC_SLOW
    #define Assert(X) if(!(X)) {*(int *)0 = 0;}
#else
    #define Assert(X)
#endif

struct loaded_file
{
    char *Name;
    
    void *Memory;
    uint64 Size;
};

inline uint32
SafeTruncateUInt64(uint64 Value)
{
    // TODO(felipe): Defines for maximum values.
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return Result;
}

// TODO(felipe): Remove globals
global_variable HANDLE GlobalConsole;
global_variable uint16 GlobalDefaultConsoleAttribute;

internal void
Error(char *Format, ...)
{
    va_list AP;
    va_start(AP, Format);
    
    SetConsoleTextAttribute(GlobalConsole, 12); // NOTE(felipe): Reddish
    fprintf(stderr, "error: ");
    SetConsoleTextAttribute(GlobalConsole, GlobalDefaultConsoleAttribute);
    vfprintf(stderr, Format, AP);
    fprintf(stderr, "\n");

    va_end(AP);
    exit(0);
}

internal void
Warning(char *Format, ...)
{
    va_list AP;
    va_start(AP, Format);
    
    SetConsoleTextAttribute(GlobalConsole, 6); // NOTE(felipe): Yellowish
    fprintf(stderr, "warning: ");
    SetConsoleTextAttribute(GlobalConsole, GlobalDefaultConsoleAttribute);
    vfprintf(stderr, Format, AP);
    fprintf(stderr, "\n");

    va_end(AP);
}

internal loaded_file
Win32ReadEntireFile(char *Filename)
{
    loaded_file Result = {};
    
    bool32 Failed = false;
    
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Memory = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Result.Memory)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.Memory, FileSize32, &BytesRead, 0) &&
                   (FileSize32 == BytesRead))
                {
                    // NOTE(felipe): File read succesfully.
                    Result.Name = Filename;
                    Result.Size = FileSize32;
                }
                else
                {
                    // TODO(felipe): Logging.
                    VirtualFree(Result.Memory, 0, MEM_RELEASE);
                    Result.Memory = 0;
                }
            }
            else
            {
                Failed = true;
                // TODO(felipe): Logging.
            }
        }
        else
        {
            Failed = true;
            // TODO(felipe): Logging.
        }

        CloseHandle(FileHandle);
    }
    else
    {
        Failed = true;
        // TODO(felipe): Logging.
    }
    
    if(Failed)
    {
        Error("cannot open: %s", Filename);
    }
    
    return Result;
}

enum token_type
{
    TokenType_Identifier,     // Text
    TokenType_Number,         // Number
    TokenType_Punctuation,    // Any other character
    
    TokenType_EOF,            // End-of-file markers
};

struct token
{
    token_type TokenType;
    token *Next;

    char *Location;
    uint32 Length;
};

internal int
main(int ArgumentCount, char **ArgumentVector)
{
    // NOTE(felipe): Tokenize
    char *InputFilename = ArgumentVector[1];
    
    GlobalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo = {};
    GetConsoleScreenBufferInfo(GlobalConsole, &ConsoleInfo);
    GlobalDefaultConsoleAttribute = ConsoleInfo.wAttributes;
    
    if(InputFilename)
    {
        loaded_file InputFile = Win32ReadEntireFile(InputFilename);
        
        if((char *)InputFile.Memory)
        {
            // NOTE(felipe): Tokenize file
            token Head = {};
            token *Current = &Head;
            
            char *Iterator = (char *)InputFile.Memory;
            
            Assert(Iterator);
            while(*Iterator)
            {
                // NOTE(felipe): Skip space and new lines.
                while(*Iterator &&
                      (*Iterator == ' ' || *Iterator == '\t'|| *Iterator == '\n' || *Iterator == '\r'))
                {
                    ++Iterator;
                }

                // NOTE(felipe): Ignore comments.
                if(*Iterator == '/' && *(Iterator + 1) == '/')
                {
                    ++Iterator;
                    while(*Iterator && *Iterator != '\n')
                    {
                        ++Iterator;
                    }
                }
                else if(*Iterator == '/' && *(Iterator + 1) == '*')
                {
                    bool32 FoundCommentEnd = false;
                    while(!FoundCommentEnd)
                    {
                        ++Iterator;
                        
                        if(*Iterator && *(Iterator + 1))
                        {
                            if(*Iterator == '*' && *(Iterator + 1) == '/')
                            {
                                Iterator += 2;
                                
                                FoundCommentEnd = true;
                            }
                        }
                        else
                        {
                            Error("unclosed comment");
                        }
                    }
                }
                else
                {
                    // TODO(felipe): More sane memory management!
                    // NOTE(felipe): Allocate new token.
                    Current->Next = (token *)malloc(sizeof(token));
                    Current = Current->Next;
                    *Current = {};
                
                    Current->Location = Iterator;
                
                    if((*Iterator >= 'a' && *Iterator <= 'z') ||
                       (*Iterator >= 'A' && *Iterator <= 'Z') ||
                       *Iterator == '_')
                    {
                        // NOTE(felipe): Token is an Identifier.
                        Current->TokenType = TokenType_Identifier;

                        while((*Iterator >= 'a' && *Iterator <= 'z') ||
                              (*Iterator >= 'A' && *Iterator <= 'Z') ||
                              (*Iterator >= '0' && *Iterator <= '9') ||
                              *Iterator == '_')
                        {
                            ++Iterator;
                        }
                    }
                    else if(*Iterator >= '0' && *Iterator <= '9')
                    {
                        // NOTE(felipe): Token is a number.
                        Current->TokenType = TokenType_Number;
                    
                        while(*Iterator >= '0' && *Iterator <= '9')
                        {
                            ++Iterator;
                        }
                    }
                    else
                    {
                        Current->TokenType = TokenType_Punctuation;
                    
                        ++Iterator;
                    }
                
                    Current->Length = SafeTruncateUInt64(Iterator - Current->Location);
                }
            }
            
            // NOTE(felipe): Last token is EOF.
            Current->TokenType = TokenType_EOF;
            
            
            // DEBUG: Print produced tokens.
            char *TokenTypes[] =
            {
                "Ident",
                "Numbe",
                "Punct",
                "EOF  ",
            };
            
            for(token *Token = Head.Next;
                Token;
                Token = Token->Next)
            {
                printf(" Token (%s): %.*s\n", TokenTypes[Token->TokenType], Token->Length, Token->Location);
            }
            //
        }
    }
    else
    {
        Error("no input files");
    }
    
    return 0;
}
