@echo off
chcp 65001 >nul
cd /d "%~dp0"

python -c "import tkinter, serial" >nul 2>nul
if errorlevel 1 (
    echo 无法启动：没有找到可用的 Python、Tkinter 或 pyserial。
    echo 请把本窗口截图发给 Codex。
    pause
    exit /b 1
)

python "%~dp0magnet_control_gui.pyw"
if errorlevel 1 (
    echo.
    echo 上位机启动失败，请把上面的错误内容截图发给 Codex。
    pause
)
