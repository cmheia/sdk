@echo off
rem Written By phd

call %~dp0..\..\env_config.cmd

set project_config=Debug

for /f "delims=" %%i in ("%cd%") do set folder=%%~ni
set project_name=%folder%

set CONTITLE=%project_name%-%project_config%

set project_elf=..\..\%project_config%\example\%project_name%\%project_name%.elf

title %CONTITLE%

:again
echo.
echo.
echo.
set /p addrs=addr list:
arm-none-eabi-addr2line -e %project_elf% -a -f %addrs%

goto again
pause
