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
if errorlevel 1 goto fail
echo  Building NIALLCHK (release)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallchk.c -o NIALLCHK
if errorlevel 1 goto fail
echo  Building NIALLCONV (release)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallconv.c -o NIALLCONV
if errorlevel 1 goto fail
echo  Building NIALLASC (release)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallasc.c -o NIALLASC
if errorlevel 1 goto fail
goto sizes

:build_debug
echo  Building NIALL (debug)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size -DDEBUG niall.c -o NIALL
if errorlevel 1 goto fail
echo  Building NIALLCHK (debug)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallchk.c -o NIALLCHK
if errorlevel 1 goto fail
echo  Building NIALLCONV (debug)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallconv.c -o NIALLCONV
if errorlevel 1 goto fail
echo  Building NIALLASC (debug)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallasc.c -o NIALLASC
if errorlevel 1 goto fail

:sizes
echo.
echo  Output sizes:
for %%F in (NIALL.COM NIALLCHK.COM NIALLCONV.COM NIALLASC.COM) do (
    echo    %%F  %%~zF bytes
)
echo.
echo ****************************************************************************
echo  Build OK. Usage: build.bat [debug]
echo ****************************************************************************
exit /b 0

:fail
echo.
echo  *** BUILD FAILED ***
echo ****************************************************************************
exit /b 1
