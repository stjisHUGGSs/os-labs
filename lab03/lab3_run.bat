@echo off
rem Lab 3 (extended variant): start three copies of lab3.exe automatically,
rem about 3 seconds apart. (Comments are in English on purpose: cmd reads
rem .bat files in the OEM code page, and non-ASCII text breaks parsing.)
setlocal EnableDelayedExpansion

set COUNT=3
rem There is no sleep command in cmd, so ping of the loopback address is used:
rem one packet per second, hence "-n 4" gives a pause of about 3 seconds.
set PING_COUNT=4

for /l %%i in (1,1,%COUNT%) do (
    rem !time! (delayed expansion) is evaluated on every pass; %time% inside
    rem a loop would be substituted only once, before the loop starts.
    echo Starting instance %%i at !time!
    rem Each copy runs in its own window. "cmd /k" keeps the window open
    rem after the program ends so the messages can be read.
    start "lab3 instance %%i" cmd /k .\lab3.exe
    if %%i lss %COUNT% ping 127.0.0.1 -n %PING_COUNT% >nul
)

echo All instances started.
endlocal
