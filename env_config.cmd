@echo off

set PYTHON_VERSION=3
set GCC_VERSION=-arm-none-eabi-4_8-2014q1-20140314-win32
set JLINK_VERSION=620h
set OPENOCD_PATH=%~dp0tools\openocd\bin
set OPENOCD_SCRIPTS=%~dp0openocd\scripts

REM set runocd=openocd -s tools\openocd\scripts -f w600_swd.cfg

set GCC_HOME=E:\dev\toolchain\gcc%GCC_VERSION%
set PYTHON_HOME=E:\dev\py%PYTHON_VERSION%

set GCC_PATH=%GCC_HOME%\bin;%GCC_HOME%\arm-none-eabi\bin
set GCC_HOME=

set PYTHON_PATH=%PYTHON_HOME%\Scripts;%PYTHON_HOME%
set PYTHON_HOME=

set SYS_PATH=%SystemRoot%\system32;%SystemRoot%

set JLINK_HOME=C:\Program Files (x86)\SEGGER\JLink_V%JLINK_VERSION%

set PATH=%SYS_PATH%;%GCC_PATH%;%PYTHON_PATH%;%JLINK_HOME%;%OPENOCD_PATH%
set SYS_PATH=
set GCC_PATH=
set JLINK_HOME=
set OPENOCD_PATH=

rem set ECLIPSE_PATH=E:\embedded\ide\eclipse\eclipse-cpp-oxygen-3a\eclipse.exe
set ECLIPSE_PATH=E:\embedded\ide\eclipse\eclipse-cpp-2018-09\eclipse.exe
set VSCODE_PATH="C:\Program Files\Microsoft VS Code\Code.exe"
