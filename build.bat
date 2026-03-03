@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************
echo  NIALL Build Script
echo ****************************************************************************

if /I "%1"=="debug" goto build_debug

:build_release
echo  Building NIALL (release)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niall.c -o NIALL

echo  Building NIALLCHK (release)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallchk.c -o NIALLCHK

goto done

:build_debug
echo  Building NIALL (debug)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size -DDEBUG niall.c -o NIALL

echo  Building NIALLCHK (debug)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallchk.c -o NIALLCHK

:done
echo ****************************************************************************
echo  Done. Usage: build.bat [debug]
echo ****************************************************************************
pause
