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
            printf("read file successfuly\n");
        }
    }
    else
    {
        Error("no input files");
    }
    
    return 0;
}
