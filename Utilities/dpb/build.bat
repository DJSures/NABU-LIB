@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************

zcc +cpm -subtype=nabu -create-app -compiler=sdcc -O3 --opt-code-speed dpb.c -o "DPBVIEW"

echo ****************************************************************************

dd if=/dev/zero of="C:\My Documents\NABU Internet Adapter\Store\b.dsk" bs=512 count=16384 

mkfs.cpm -f naburn8mb "C:\My Documents\NABU Internet Adapter\Store\b.dsk"

cpmcp -f naburn8mb    "C:\My Documents\NABU Internet Adapter\Store\b.dsk" DPBVIEW.com 0:

dir

pause