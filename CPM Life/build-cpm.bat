@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************

zcc +cpm -Wall -o life.com life.c -lndos

:: zcc +cpm -vn --list -m -create-app -compiler=sdcc -O3 --opt-code-speed main.c -o "game5"

echo ****************************************************************************

mkfs.cpm -f naburn "C:\My Documents\NABU Internet Adapter\Store\b.dsk"

cpmcp -f naburn    "C:\My Documents\NABU Internet Adapter\Store\b.dsk" life.com 0:

dir "C:\My Documents\Personal\NABU\C Programs\Game Dev 5\*.com"

pause