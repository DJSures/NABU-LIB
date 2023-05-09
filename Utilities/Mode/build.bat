@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************

zcc +cpm main.c -compiler=sdcc -create-app -O3 --opt-code-speed -o mode

echo ****************************************************************************

mkfs.cpm -f naburn8mb "C:\My Documents\NABU Internet Adapter\Store\a.dsk"

cpmcp -f naburn8mb    "C:\My Documents\NABU Internet Adapter\Store\a.dsk" mode.com 0:

pause