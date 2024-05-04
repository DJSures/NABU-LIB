@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************

zcc +cpm -subtype=nabu -create-app --list -m -compiler=sdcc -O3 --opt-code-speed  main.c -o "GPTAKTOE"

echo ****************************************************************************

mkfs.cpm -f naburn8mb "C:\My Documents\NABU Internet Adapter\Store\C.DSK"

cpmcp -f naburn8mb    "C:\My Documents\NABU Internet Adapter\Store\C.DSK" GPTAKTOE.COM 0:

pause