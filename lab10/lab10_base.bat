@echo off
rem Lab 10, basic variant (Windows): ask for a directory name, create it in the
rem current directory and copy into it all files with the given extension.
rem (Text is in English on purpose: cmd reads .bat files in the OEM code page,
rem and non-ASCII characters break parsing.)
rem
rem Run from the directory that holds the files:  ..\lab10_base.bat
setlocal EnableDelayedExpansion

rem EXT is the extension of the student's files. The task says: for example .c or .txt
set EXT=c

rem set /p prints a prompt and reads one line into the variable DIR.
set /p DIR=Enter directory name:

rem An empty name is not allowed.
if "%DIR%"=="" (
    echo Directory name is empty.
    exit /b 1
)

rem Create the directory unless it already exists.
if not exist "%DIR%" mkdir "%DIR%"

rem Loop over every file of the current directory matching *.c.
rem %%f is the loop variable. Several commands in the body are written in
rem parentheses, one per line, instead of joining them with the & character.
rem The counter changes inside the loop, so it is read as !COUNT! (delayed
rem expansion): %COUNT% would be substituted once, before the loop starts.
set COUNT=0
for %%f in (*.%EXT%) do (
    copy "%%f" "%DIR%" >nul
    echo Copied file: %%f
    set /a COUNT+=1
)

echo Done. Copied files .%EXT%: !COUNT!, directory: %DIR%
endlocal
