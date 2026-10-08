@echo off
rem Lab 10, extended variant (Windows): ask for a directory name, create it in
rem the current directory and MOVE all files with the given extension into newly
rem created subdirectories. The subdirectory name equals the file name without the
rem extension:  proga.c  ->  <directory>\proga\proga.c
rem (Text is in English on purpose: cmd reads .bat files in the OEM code page.)
rem
rem Run from the directory that holds the files:  ..\lab10_ext.bat  (files are moved!)
setlocal EnableDelayedExpansion

set EXT=c

set /p DIR=Enter directory name:

if "%DIR%"=="" (
    echo Directory name is empty.
    exit /b 1
)

if not exist "%DIR%" mkdir "%DIR%"

rem %%~nf is the file name without the extension. The same can be done with the
rem substitution form !NAME:.%EXT%=! described in the manual. NAME and COUNT
rem change inside the loop, so they are read with exclamation marks (delayed
rem expansion).
set COUNT=0
for %%f in (*.%EXT%) do (
    set NAME=%%~nf
    if not exist "%DIR%\!NAME!" mkdir "%DIR%\!NAME!"
    move "%%f" "%DIR%\!NAME!\" >nul
    echo Moved file: %%f to %DIR%\!NAME!\
    set /a COUNT+=1
)

echo Done. Moved files .%EXT%: !COUNT!, directory: %DIR%
endlocal
