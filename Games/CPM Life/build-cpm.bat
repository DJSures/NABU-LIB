@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************

zcc +cpm -Wall -o life.com life.c -lndos

echo ****************************************************************************

mkfs.cpm -f naburn8mb "C:\My Documents\NABU Internet Adapter\Store\c.dsk"

cpmcp -f naburn8mb    "C:\My Documents\NABU Internet Adapter\Store\c.dsk" life.com 0:

dir *.com

pause