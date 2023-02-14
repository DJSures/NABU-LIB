@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************

zcc +cpm main.c -compiler=sdcc -create-app -O3 --opt-code-speed -o vtest52

echo ****************************************************************************

mkfs.cpm -f naburn "C:\My Documents\NABU Internet Adapter\Store\a.dsk"

cpmcp -f naburn    "C:\My Documents\NABU Internet Adapter\Store\a.dsk" vtest52.com 0:

pause