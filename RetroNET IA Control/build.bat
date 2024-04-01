@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************

:: zcc +cpm -vn --list -m -create-app -compiler=sdcc -O3 --opt-code-speed main.c -o "game5"
zcc +nabu -vn --list -m -create-app -compiler=sdcc main.c -o "C:\My Documents\NABU Internet Adapter\Local Source\hedless.nabu"

:: zcc +cpm -subtype=nabu main.c -compiler=sdcc -create-app -lndos -pragma-need=ansiterminal -pragma-define:ansicolumns=51 -o DUMPRAM
:: Columns can be 85, 80, 64, 51, 42, 40, 46, 32, 28, 24

echo ****************************************************************************
pause