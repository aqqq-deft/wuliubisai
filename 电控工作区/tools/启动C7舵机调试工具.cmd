@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0codbot_servo_tool.ps1" -Mode C7
if errorlevel 1 pause
