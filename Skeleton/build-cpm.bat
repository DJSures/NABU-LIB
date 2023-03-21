@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************

zcc +cpm -vn --list -m -create-app -compiler=sdcc -O3 --opt-code-speed main.c -o "myapp"

echo ****************************************************************************

mkfs.cpm -f naburn8mb "C:\My Documents\NABU Internet Adapter\Store\c.dsk"

cpmcp -f naburn8mb    "C:\My Documents\NABU Internet Adapter\Store\c.dsk" myapp.com 0:

dir *.com

pause