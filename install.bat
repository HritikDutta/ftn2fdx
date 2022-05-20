@echo off

set outdir=C:\hd-tools\

if not exist %outdir% md %outdir%

cl /O2 /c converter/*.c /I ./
cl /O2 main.c /Fe:%outdir%ff *.obj /GL

del *.obj