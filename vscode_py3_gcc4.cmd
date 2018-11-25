@echo off
setlocal

call %~dp0env_config.cmd

set CONTITLE="VSCode"

REM set BUILD_FLAVOR=debug

start %CONTITLE% %VSCODE_PATH% -r %~dp0
cd /d %~dp0
