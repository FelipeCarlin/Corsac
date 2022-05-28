/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Felipe Carlin $
   $Notice: Copyright © 2022 Felipe Carlin $
   ======================================================================== */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#include <windows.h>

#include "corsac.h"
#include "win32_corsac.h"

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
ErrorInToken(token *Token, char *Format, ...)
{
    va_list AP;
    va_start(AP, Format);
    
    char *Line = Token->Location;
    char *End = Token->Location;
    
    if(Token->TokenType != TokenType_EOF)
    {
        while(Token->SourceFile->Memory < Line && Line[-1] != '\n' && Line[-1] != '\r')
        {
            --Line;
        }
        
        while(*End && *End != '\n' && *End != '\r')
        {
            ++End;
        }
    }
    else
    {
        Line = (uint8 *)Token->SourceFile->Memory + Token->SourceFile->Size - 1;
        while(*Line && Line > Token->SourceFile->Memory && *Line != '\n' && *Line != '\r')
        {
            --Line;
        }
        
        End = (uint8 *)Token->SourceFile->Memory + Token->SourceFile->Size;
    }
    
    SetConsoleTextAttribute(GlobalConsole, 12); // NOTE(felipe): Reddish
    uint32 Indent = fprintf(stderr, "error: ");
    SetConsoleTextAttribute(GlobalConsole, GlobalDefaultConsoleAttribute);
    
    Indent += fprintf(stderr, "%s: ", Token->SourceFile->Filename);
    
    int32 Position = (uint32)((Token->Location ? Token->Location - Line : End - Line) + Indent);
    fprintf(stderr, "%.*s\n%*s^ ", (int32)(End - Line), Line, Position, "");
    
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

internal void
WarningInToken(token *Token, char *Format, ...)
{
    va_list AP;
    va_start(AP, Format);
    
    char *Line = Token->Location;
    while(Token->SourceFile->Memory < Line && Line[-1] != '\n' && Line[-1] != '\r')
    {
        --Line;
    }
    char *End = Token->Location;
    while(*End && *End != '\n' && *End != '\r')
    {
        ++End;
    }
    
    SetConsoleTextAttribute(GlobalConsole, 6); // NOTE(felipe): Yellowish
    uint32 Indent = fprintf(stderr, "warning: ");
    SetConsoleTextAttribute(GlobalConsole, GlobalDefaultConsoleAttribute);
    
    Indent += fprintf(stderr, "%s: ", Token->SourceFile->Filename);
    
    int32 Position = (uint32)(Token->Location - Line + Indent);
    fprintf(stderr, "%.*s\n%*s^ ", (int32)(End - Line), Line, Position, "");
    
    vfprintf(stderr, Format, AP);
    fprintf(stderr, "\n");
    
    va_end(AP);
}

internal uint32
Win32GetTime()
{
    uint32 Result = 0;
    
    time_t Time = time(0);
    Result = (uint32)Time;
    
    return Result;
}

internal loaded_file
Win32ReadEntireFile(char *Filename)
{
    loaded_file Result = {0};
    
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
                    Result.Filename = Filename;
                    Result.Size = FileSize32;
                }
                else
                {
                    VirtualFree(Result.Memory, 0, MEM_RELEASE);
                    Result.Memory = 0;
                }
            }
        }

        CloseHandle(FileHandle);
    }
    
    return Result;
}

internal bool32
Win32WriteEntireFile(char *Filename, void *Memory, memory_index MemorySize)
{
    bool32 Result = false;
    
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            // NOTE(felipe): File read succesfully.
            Result = (BytesWritten == MemorySize);
        }
        else
        {
            Assert(!"Heyy");
            // TODO(felipe): Logging.
        }

        CloseHandle(FileHandle);
    }
    else
    {
        // TODO(felipe): Logging.
    }

    return Result;
}

typedef struct memory_arena
{
    void *Memory;
    memory_index Size;
    memory_index Used;
} memory_arena;
#define PushStruct(Arena, Type) (Type*)PushSize_(Arena, sizeof(Type))
#define PushArray(Arena, Count, Type) (Type*)PushSize_(Arena, (Count)*sizeof(Type))
#define PushSize(Arena, Size) PushSize_(Arena, Size)
inline void* PushSize_(memory_arena* Arena, memory_index Size)
{
    Assert((Arena->Used + Size) <= Arena->Size);
    void *Result = (uint8 *)Arena->Memory + Arena->Used;
    Arena->Used += Size;
    return Result;
}

internal memory_arena
Win32AllocateArena(memory_index Size)
{
    memory_arena Arena = {0};
    
    Arena.Memory = VirtualAlloc(0, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    Arena.Size = Size;
    Arena.Used = 0;
    
    return Arena;
}

#include "corsac.c"

internal int
main(int ArgumentCount, char **ArgumentVector)
{
    char *InputFilename = ArgumentVector[1];
    
    GlobalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo = {0};
    GetConsoleScreenBufferInfo(GlobalConsole, &ConsoleInfo);
    GlobalDefaultConsoleAttribute = ConsoleInfo.wAttributes;
    
    CorsacMain(InputFilename);
    
    SetConsoleTextAttribute(GlobalConsole, 2); // NOTE(felipe): Cyan
    fprintf(stdout, "\nsuccess\n");
    SetConsoleTextAttribute(GlobalConsole, GlobalDefaultConsoleAttribute);
    
    return 0;
}
