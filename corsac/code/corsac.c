/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Felipe Carlin $
   $Notice: Copyright © 2022 Felipe Carlin $
   ======================================================================== */

#include "corsac.h"

internal uint64
StringToNumber(char *Start, uint32 Lenght)
{
    uint64 Result = 0;

    if(Lenght >= 3 && Start[0] == '0' && Start[1] == 'x')
    {
        Start += 2;
        Lenght -= 2;
        
        while(Lenght--)
        {
            Result *= 16;
            
            if(*Start >= '0' &&
               *Start <= '9')
            {
                Result += *Start - '0';
            }
            else if(*Start >= 'a' &&
                    *Start <= 'f')
            {
                Result += *Start - 'a' + 10;
            }
            else
            {
                Error("not a valid number");
            }
            
            ++Start;
        }
    }
    else
    {
        while(Lenght--)
        {
            Result *= 10;
            Result += *Start - '0';
            
            ++Start;
        }
    }
    
    return Result;
}

internal token *
Tokenize(loaded_file *File)
{
    token Head = {0};
    token *Current = &Head;

    bool32 AtBeginningOfLine = true;
    
    char *Iterator = File->Memory;
    
    Assert(Iterator);
    while(*Iterator)
    {
        // NOTE(felipe): Skip space and new lines.
        while(*Iterator &&
              (*Iterator == ' ' || *Iterator == '\t'|| *Iterator == '\n' || *Iterator == '\r'))
        {
            if(*Iterator == '\n')
            {
                AtBeginningOfLine = true;
            }
            
            ++Iterator;
        }
        
        if(*Iterator)
        {
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
            else if(*Iterator == '\\')
            {
                // TODO(felipe): If escape caracter is encountered ignore newline.
                ++Iterator;
            
                bool32 ValidSequence = false;
                if(*Iterator == '\r')
                {
                    ValidSequence = true;
                    ++Iterator;
                }
                if(*Iterator == '\n')
                {
                    ValidSequence = true;
                    ++Iterator;
                }
                
                if(!ValidSequence)
                {
                    Error("illegal escape sequence");
                }
            }
            else
            {
                // TODO(felipe): More sane memory management!
                // NOTE(felipe): Allocate new token.
                Current->Next = (token *)calloc(sizeof(token), 1);
                Current = Current->Next;
                
                Current->SourceFile = File;
                Current->Location = Iterator;
                Current->AtBeginningOfLine = AtBeginningOfLine;
                AtBeginningOfLine = false;
                
                if((*Iterator >= 'a' && *Iterator <= 'z') ||
                   (*Iterator >= 'A' && *Iterator <= 'Z') ||
                   *Iterator == '_')
                {
                    // NOTE(felipe): Token is an Identifier or Keyword.
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
                    
                    while((*Iterator >= '0' && *Iterator <= '9') ||
                          (*Iterator >= 'a' && *Iterator <= 'f') ||
                          *Iterator == 'x')
                    {
                        ++Iterator;
                    }
                    
                    Current->NumericalValue = StringToNumber(Current->Location,
                                                             SafeTruncateUInt64(Iterator - Current->Location));
                }
                else if(*Iterator == '\'')
                {
                    Current->TokenType = TokenType_Number;
                    
                    // TODO(felipe): Multi-character constant? (C99 spec. 6.4.4.4p10).
                    
                    char *Start = Iterator;
                        
                    ++Iterator;
                    if(*Iterator == '\\')
                    {
                        ++Iterator;
                        switch(*Iterator)
                        {
                            case 'r': { Current->NumericalValue = '\r'; } break;
                            case 'n': { Current->NumericalValue = '\n'; } break;
                            case 't': { Current->NumericalValue = '\t'; } break;
                                    
                            case '\'': { Current->NumericalValue = '\''; } break;
                            case '\"': { Current->NumericalValue = '\"'; } break;
                            
                            case '\\': { Current->NumericalValue = '\\'; } break;
                            
                            case '0': { Current->NumericalValue = 0; } break;
                            
                            default:
                            {
                                Error("unknown escape sequence");
                            } break;
                        }
                    
                        Iterator += 2;
                    }
                    else
                    {
                        if(*(Iterator + 1) == '\'')
                        {
                            Current->Location = Iterator + 1;
                            Current->Length = 1;
                        
                            Iterator += 2;
                        }
                        else
                        {
                            Error("invalid constant char");
                        }
                    }
                }
                else
                {
                    Current->TokenType = TokenType_Punctuation;
                    
                    // NOTE(felipe): Some forms of punctuation are
                    // more than just one character e.i. '=='.
                    if(*Iterator == '=' &&
                       (*(Iterator + 1) == '=' ||
                        *(Iterator + 1) == '<' ||
                        *(Iterator + 1) == '>'))
                    {
                        Iterator += 2;
                    }
                    else if(*Iterator == '!' &&
                            *(Iterator + 1) == '=')
                    {
                        Iterator += 2;
                    }    
                    else
                    {
                        ++Iterator;
                    }
                }
            
                Current->Length = SafeTruncateUInt64(Iterator - Current->Location);
            }
        }
    }
    
    char *Keywords[] =
        {
            "return",
            "if",
            "else",
            "for",
        };
    
    for(token *Token = Head.Next;
        Token;
        Token = Token->Next)
    {    
        for(uint32 Index = 0;
            Index < ArrayCount(Keywords);
            ++Index)
        {
            if(TokenIs(Token, Keywords[Index]))
            {
                Token->TokenType = TokenType_Keyword;
            }
        }
    }
    
    // NOTE(felipe): Last token is EOF.
    Current->Next = (token *)calloc(sizeof(token), 1);
    Current = Current->Next;
    
    Current->TokenType = TokenType_EOF;
    Current->SourceFile = File;

    return Head.Next;
}

#if 0
typedef struct string_list_element
{
    // NOTE(felipe): Size in bytes, including null char.
    uint32 Length;
    char *String;
    
    struct string_list_element *Next;
} string_list_element;

typedef struct string_list
{
    uint32 Count;
    
    string_list_element *First;
    string_list_element *Last;
} string_list;

internal void
PushString(string_list *List, char *String)
{
    string_list_element *ListElement = (string_list_element *)malloc(sizeof(string_list_element));
    
    if(!List->First)
    {
        List->First = ListElement;
        List->Last = ListElement;
    }
    else
    {
        List->Last->Next = ListElement;
        List->Last = ListElement;
    }
    
    uint32 Length = StringLength(String);
    char *NewString = StringDuplicate(String, Length);
    
    ListElement->Length = Length;
    ListElement->String = NewString;
    ++List->Count;
}

internal void
PushStringN(string_list *List, char *String, uint32 Length)
{
    string_list_element *ListElement = (string_list_element *)malloc(sizeof(string_list_element));
    
    if(!List->First)
    {
        List->First = ListElement;
        List->Last = ListElement;
    }
    else
    {
        List->Last->Next = ListElement;
        List->Last = ListElement;
    }
    
    char *NewString = StringDuplicate(String, Length);
    
    ListElement->Length = Length;
    ListElement->String = NewString;
    ++List->Count;
}
#endif

inline bool32
TokenIsCharacter(token *Token, char Test)
{
    bool32 Result = Token->Length == 1 && *Token->Location == Test;

    return Result;
}

inline bool32
TokenIs(token *Token, char *Test)
{
    bool32 Result = false;

    uint32 Length = StringLength(Test);
    if(Token->Length == Length)
    {
        Result = true;
        
        for(uint32 Index = 0;
            Index < Length;
            ++Index)
        {
            if(Token->Location[Index] != Test[Index])
            {
                Result = false;
                break;
            }
        }
    }
    
    return Result;
}

internal char *
StringFromToken(token *Token)
{
    char *Result = StringDuplicate(Token->Location, Token->Length);
    
    return Result;
}

internal char *
StringFromTokens(token *Token, token *End)
{
    // NOTE(felipe): Includes Token but not End.
    char *Result = 0;
    
    uint32 Length = 0;
    for(token *Test = Token;
        Test != End;
        Test = Test->Next)
    {
        Length += Test->Length;
    }
    
    Result = (char *)malloc(Length + 1);
    
    uint32 Index = 0;
    for(token *Test = Token;
        Test != End;
        Test = Test->Next)
    {
        for(uint32 J = 0;
            J < Test->Length;
            ++J)
        {
            Result[J] = Test->Location[J];
        }
        
        Index += Test->Length;
    }

    Result[Index] = 0;
    
    return Result;
}

internal token *
CopyToken(token *Token)
{
    token *T = (token *)malloc(sizeof(token));
    
    *T = *Token;
    T->Next = 0;
    
    return T;
}

internal token *
AppendToken(token *Token1, token *Token2)
{
    token *Result = 0;
    
    if(Token1->TokenType == TokenType_EOF)
    {
        Result = Token2;
    }
    else
    {    
        token Head = {0};
        token *Current = &Head;
    
        for (;
             Token1->TokenType != TokenType_EOF;
             Token1 = Token1->Next)
        {
            Current->Next = CopyToken(Token1);
            Current = Current->Next;
        }
    
        Current->Next = Token2;
        Result = Head.Next;
    }
    
    return Result;
}

internal token *
IncludeFile(token *Token, char *Path, token *IncludeToken)
{
    token *Result = 0;
        
    loaded_file IncludeFile = Win32ReadEntireFile(Path);
    if(IncludeFile.Memory)
    {
        token *Token2 = Tokenize(&IncludeFile);
        if(!Token2)
        {
            ErrorInToken(IncludeToken, "could not open include file: %s", Path);
        }        
        
        Result = AppendToken(Token2, Token);
    }
    else
    {
        ErrorInToken(IncludeToken, "could not open include file: %s", Path);
    }

    return Result;
}

typedef struct macro
{
    token *Identifier;
    
    // NOTE(felipe): Does include Start, does not include End.
    token *Start;
    token *End;

    struct macro *Next;
} macro;

typedef struct macro_list
{
    uint32 Count;

    macro *First;
    macro *Last;
} macro_list;

internal void
PushMacro(macro_list *List, macro *SourceMacro)
{
    macro *ListElement = (macro *)malloc(sizeof(macro));
    
    if(!List->First)
    {
        List->First = ListElement;
        List->Last = ListElement;
    }
    else
    {
        List->Last->Next = ListElement;
        List->Last = ListElement;
    }
    
    MemCopy(ListElement, SourceMacro, sizeof(macro));
    
    ++List->Count;
}

#include "corsac_parser.c"
#include "corsac_ir.c"

internal int
CorsacMain(char *InputFilename)
{
    if(InputFilename)
    {
        loaded_file InputFile = Win32ReadEntireFile(InputFilename);
        
        if((char *)InputFile.Memory)
        {
            // NOTE(felipe): Tokenize file
            token *Tokens = Tokenize(&InputFile);
            
            // NOTE(felipe): Preprocess
#if 0
            string_list IncludeDirs = {0};
            
            // TODO(felipe): This should not be hard-coded.
            // PushString(&State->IncludeDirs, ".\\include");
            PushString(&IncludeDirs, "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.19041.0\\ucrt");
            PushString(&IncludeDirs, "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Tools\\MSVC\\14.29.30133\\include");
            PushString(&IncludeDirs, "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.19041.0\\um");
#endif
            
            token *Token = Tokens;
            
            token Head = {0};
            token *Current = &Head;
            
            macro_list Macros = {0};
            
            token *LastToken = 0;
            while(Token->TokenType != TokenType_EOF)
            {
                if(!TokenIsCharacter(Token, '#'))
                {
                    bool32 ReplacedWithMacro = false;
                    for(macro *Macro = Macros.First;
                        Macro;
                        Macro = Macro->Next)
                    {
                        if(Token->Length == Macro->Identifier->Length &&
                           !StringCompare(Token->Location, Macro->Identifier->Location, Token->Length))
                        {
                            // NOTE(felipe): Found an instance of a
                            // macro, copy it.
                            token *Cursor = LastToken;
                            for (token *Iterator = Macro->Start;
                                 Iterator != Macro->End;
                                 Iterator = Iterator->Next)
                            {
                                Cursor->Next = CopyToken(Iterator);
                                Cursor = Cursor->Next;
                                
                                Current->Next = Cursor;
                                Current = Current->Next;
                            }
                            
                            Cursor->Next = Token->Next;
                            Token = Cursor->Next;
                            
                            ReplacedWithMacro = true;
//                            Error("Macro match");
                            break;
                        }
                    }
                    
                    LastToken = Token;
                    
                    if(!ReplacedWithMacro)
                    {
                        Current->Next = Token;
                        Current = Current->Next;
                        Token = Token->Next;
                    }
                }
                else
                {
                    // NOTE(felipe): This token is a preprocessor directive.
                    token *Start = Token;
                    Token = Token->Next;
                    
                    if(TokenIs(Token, "define"))
                    {
                        if(!Token->Next->AtBeginningOfLine)
                        {
                            Token = Token->Next;
                            
                            macro Macro = {0};
                            Macro.Identifier = Token;
                            
                            Token = Token->Next;                   
                            token *StringEnd = Token;
                            while(!StringEnd->AtBeginningOfLine)
                            {
                                StringEnd = StringEnd->Next;
                            }
                            
                            if(Macro.Identifier != StringEnd)
                            {
                                // NOTE(felipe): There is a string to
                                // replace in every instance of identifier.
                                
                                Macro.Start = Token;
                                Macro.End = StringEnd;
                                
                                Token = StringEnd;
                            }
                            
                            PushMacro(&Macros, &Macro);
                        }
                        else
                        {
                            ErrorInToken(Token, "invalid define directive");
                        }
                    }
                    else if(TokenIs(Token, "include"))
                    {
                        Token = Token->Next;
                        
                        char *Filename = 0;
                        if(TokenIs(Token, "\""))
                        {
                            // Pattern 1: #include "foo.h"
                            
                            Filename = StringFromToken(Token->Next);
                            AssertNext(Token, "\"");
                            Token = Token->Next;
                        }
                        else if(TokenIsCharacter(Token, '<'))
                        {
                            ErrorInToken(Token, "<filename> is unsupported");
                        }
                        else
                        {
                            ErrorInToken(Token, "unexpected \"filename\" or <filename>");
                        }
                        
                        Token = IncludeFile(Token, Filename, Start->Next->Next);
                    }
                    else if(TokenIs(Token, "error"))
                    {
                        ErrorInToken(Token->Next, "error preprocessor directive");
                    }
                    else if(TokenIs(Token, "warning"))
                    {
                        WarningInToken(Token->Next, "warning preprocessor directive");
                    }
                    else
                    {
                        ErrorInToken(Token, "unsupported preprocessor directive");
                    }
                }
            }
            
            // DEBUG: Print produced tokens.
            char *TokenTypes[] =
                {
                    "Ident",
                    "Punct",
                    "Keywo",
                    "Numbe",
                    "EOF  ",
                };
            
            for(token *Token = Head.Next;
                Token;
                Token = Token->Next)
            {
                printf(" Token %c (%s): %.*s\n", Token->AtBeginningOfLine?'Y':'N', TokenTypes[Token->TokenType], Token->Length, Token->Location);
            }
            //
            
            // NOTE(felipe): Parse
            program *Program = ParseTokens(Head.Next);
            
            // NOTE(felipe): Generate Intermediate Representation.
            GenerateIR(Program);
        }
    }
    else
    {
        Error("no input files");
    }
    
    return 0;
}
