@echo off
setlocal

call %~dp0env_config.cmd

set CONTITLE="Eclipse"

REM set BUILD_FLAVOR=debug

start %CONTITLE% %ECLIPSE_PATH%
cd /d %~dp0
