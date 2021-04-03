@echo off

set outdir=C:\tools\

cl /c converter/*.c /I ./
cl main.c /Fe:%outdir%ff *.obj

del *.obj