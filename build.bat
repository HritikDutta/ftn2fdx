@echo off

cl /c converter/*.c /I ./
cl main.c /Fe:fftest *.obj

del *.obj