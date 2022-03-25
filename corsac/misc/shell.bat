@echo off

REM
REM    To run this at startup, use this as you shortcut target:
REM    "%windir%\system32\cmd.exe /k w:\Corsac_git\corsac\misc\shell.bat"
REM

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
set path=w:\Corsac_git\corsac\misc;%path%
w:
cd Corsac_git