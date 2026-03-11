@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************
echo  NIALL Build Script
echo ****************************************************************************

if /I "%1"=="debug" goto build_debug
if /I "%1"=="nabu"   goto build_nabu
if /I "%1"=="nabu80" goto build_nabu80

:build_release
echo  Building NIALL (release)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niall.c -o NIALL
if errorlevel 1 goto fail
echo  Building NIALLCHK (release)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallchk.c -o NIALLCHK
if errorlevel 1 goto fail
echo  Building NIALLCON (release)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallcon.c -o NIALLCON
if errorlevel 1 goto fail
echo  Building NIALLASC (release)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallasc.c -o NIALLASC
if errorlevel 1 goto fail
goto sizes

:build_nabu
echo  Building NIALLN (NABU native, 40-col TMS9918 — default)...
zcc +nabu -vn -create-app -compiler=sdcc --opt-code-size niall.c -o NIALLN
if errorlevel 1 goto fail
echo.
echo  Output:
for %%F in (NIALLN.nabu) do echo    %%F  %%~zF bytes
echo.
echo ****************************************************************************
echo  Build OK. Usage: build.bat [debug^|nabu^|nabu80]
echo ****************************************************************************
exit /b 0

:build_nabu80
echo  Building NIALLN (NABU native, 80-col F18A)...
zcc +nabu -vn -create-app -compiler=sdcc --opt-code-size -DVDP_80COL niall.c -o NIALLN
if errorlevel 1 goto fail
echo.
echo  Output:
for %%F in (NIALLN.nabu) do echo    %%F  %%~zF bytes
echo.
echo ****************************************************************************
echo  Build OK. Usage: build.bat [debug^|nabu^|nabu80]
echo ****************************************************************************
exit /b 0

:build_debug
echo  Building NIALL (debug)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size -DDEBUG niall.c -o NIALL
if errorlevel 1 goto fail
echo  Building NIALLCHK (debug)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallchk.c -o NIALLCHK
if errorlevel 1 goto fail
echo  Building NIALLCON (debug)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallcon.c -o NIALLCON
if errorlevel 1 goto fail
echo  Building NIALLASC (debug)...
zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallasc.c -o NIALLASC
if errorlevel 1 goto fail

:sizes
echo.
echo  Output sizes:
for %%F in (NIALL.COM NIALLCHK.COM NIALLCON.COM NIALLASC.COM) do (
    echo    %%F  %%~zF bytes
)
echo.
echo ****************************************************************************
echo  Build OK. Usage: build.bat [debug^|nabu]
echo ****************************************************************************
exit /b 0

:fail
echo.
echo  *** BUILD FAILED ***
echo ****************************************************************************
exit /b 1
