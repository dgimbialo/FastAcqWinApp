@echo off
REM Compile and run FastAcq. Forwards any args to the PowerShell script.
REM Usage:  build-and-run.bat                (Release, build + run)
REM         build-and-run.bat -Configuration Debug
REM         build-and-run.bat -Clean -NoRun
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-and-run.ps1" %*
