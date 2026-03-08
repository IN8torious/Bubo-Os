@echo off
title BUBO OS — Blue OS
color 04

echo.
echo  ============================================================
echo   BUBO OS — Blue OS
echo   Built by Nathan Brown for Landon Pankuch
echo   NO MAS DISADVANTAGED
echo  ============================================================
echo.

REM Check Python
python --version >nul 2>&1
if errorlevel 1 (
    echo  [ERROR] Python not found. Install from https://python.org
    echo  Make sure to check "Add Python to PATH" during install.
    pause
    exit /b 1
)

REM Install dependencies if needed
echo  Checking dependencies...
pip install -r requirements.txt -q
if errorlevel 1 (
    echo  [WARN] Some dependencies may not have installed. Continuing...
)

echo.
echo  Starting BUBO OS...
echo  Ctrl+Shift+K  =  Summon Kami
echo  Click the red bar at top  =  Toggle crew tray
echo  Escape  =  Close tray
echo.

REM Launch the full desktop
python agents\world\bubo_desktop.py

pause
