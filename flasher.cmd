@echo off
setlocal

set DOWNLOADER_HOME=%~dp0\tools;%~dp0\tools\py_scripts

set SYS_PATH=%SystemRoot%\system32;%SystemRoot%

set PATH=%SYS_PATH%;%DOWNLOADER_HOME%

REM flasher.py Debug/bin/example.blinky.blinky_gz.img COM11
REM flasher.py Debug/bin/example.blinky.blinky_gz.img
set scriptname=flasher.py

"%scriptname%" %*
