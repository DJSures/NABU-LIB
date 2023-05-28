@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************

zcc +nabu -vn --list -m -create-app -compiler=sdcc main.c -o "C:\NABU Segments\000001.nabu"

echo ****************************************************************************

pause
