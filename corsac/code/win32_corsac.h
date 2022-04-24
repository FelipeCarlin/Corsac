#if !defined(WIN32_CORSAC_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Felipe Carlin $
   $Notice: Copyright © 2022 Felipe Carlin $
   ======================================================================== */

internal void Error(char *Format, ...);
internal void ErrorInToken(token *Token, char *Format, ...);
internal void Warning(char *Format, ...);
internal void WarningInToken(token *Token, char *Format, ...);

internal loaded_file Win32ReadEntireFile(char *Filename);

#define WIN32_CORSAC_H
#endif
