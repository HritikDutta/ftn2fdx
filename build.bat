@echo off

cl /Zi /DDEBUG /c converter/*.c /I ./
cl /Zi /DDEBUG main.c /Fe:fftest *.obj

del *.obj