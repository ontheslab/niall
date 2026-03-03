@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************

rem zcc +cpm -subtype=nabu -create-app --math32 -compiler=sdcc -O3 --opt-code-speed  niall.c -o NIALL
zcc +cpm -vn -create-app -compiler=sdcc -O3 nival.c -o NIVAL
zcc +cpm -vn -create-app -compiler=sdcc -O3 nifix.c -o NIFIX

echo ****************************************************************************

pause