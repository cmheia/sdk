@echo off
setlocal

set JLINK_VERSION=620h

set JLINK_HOME=C:\Program Files (x86)\SEGGER\JLink_V%JLINK_VERSION%

set PATH=%JLINK_HOME%;%PATH%

JLinkGDBServerCL -select USB -device Cortex-M3 -if SWD -speed 6000 -noir
