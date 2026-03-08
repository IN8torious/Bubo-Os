@echo off
:: =============================================================================
:: BATTY.BAT — PC Health Checker
:: Designed after Batty Koda from FernGully: The Last Rainforest
:: Voiced by Robin Williams. Scrambled by science. Loyal to the end.
::
:: "They've done experiments on me. I KNOW things."
::
:: Part of BUBO OS Agent Suite
:: Built for Landon Pankuch. NO MAS DISADVANTAGED.
:: Copyright (c) 2025 Nathan Pankuch — MIT License
:: =============================================================================

setlocal enabledelayedexpansion
title BATTY — PC Health Monitor [BUBO OS]
color 0A

:: ── INTRO ────────────────────────────────────────────────────────────────────
cls
echo.
echo  ██████╗  █████╗ ████████╗████████╗██╗   ██╗
echo  ██╔══██╗██╔══██╗╚══██╔══╝╚══██╔══╝╚██╗ ██╔╝
echo  ██████╔╝███████║   ██║      ██║    ╚████╔╝
echo  ██╔══██╗██╔══██║   ██║      ██║     ╚██╔╝
echo  ██████╔╝██║  ██║   ██║      ██║      ██║
echo  ╚═════╝ ╚═╝  ╚═╝   ╚═╝      ╚═╝      ╚═╝
echo.
echo  [ BUBO OS PC Health Agent — Codename: BATTY ]
echo  [ "They scrambled my radar but they couldn't scramble MY LOYALTY." ]
echo.
timeout /t 2 /nobreak >nul

echo  Initiating health scan... THEY SAID I COULDN'T DO IT. THEY WERE WRONG.
echo  =========================================================================
echo.
timeout /t 1 /nobreak >nul

:: ── CPU INFO ─────────────────────────────────────────────────────────────────
echo  [BATTY] Checking CPU... hold on, my antenna is picking something up...
echo.
for /f "tokens=2 delims==" %%A in ('wmic cpu get Name /value 2^>nul') do (
    set "CPU_NAME=%%A"
)
for /f "tokens=2 delims==" %%A in ('wmic cpu get NumberOfCores /value 2^>nul') do (
    set "CPU_CORES=%%A"
)
for /f "tokens=2 delims==" %%A in ('wmic cpu get NumberOfLogicalProcessors /value 2^>nul') do (
    set "CPU_THREADS=%%A"
)
for /f "tokens=2 delims==" %%A in ('wmic cpu get MaxClockSpeed /value 2^>nul') do (
    set "CPU_SPEED=%%A"
)
for /f "tokens=2 delims==" %%A in ('wmic cpu get LoadPercentage /value 2^>nul') do (
    set "CPU_LOAD=%%A"
)

echo  CPU: !CPU_NAME!
echo  Cores: !CPU_CORES! physical / !CPU_THREADS! logical
echo  Max Speed: !CPU_SPEED! MHz
echo  Current Load: !CPU_LOAD!%%

if !CPU_LOAD! GTR 90 (
    echo.
    echo  [BATTY] THEY'RE COOKING YOU FROM THE INSIDE!! CPU at !CPU_LOAD!%%!
    echo          THAT IS NOT NORMAL. THAT IS WHAT THEY WANT YOU TO THINK IS NORMAL.
    color 0C
) else if !CPU_LOAD! GTR 70 (
    echo.
    echo  [BATTY] CPU is working hard... !CPU_LOAD!%%. I've seen worse. In the lab.
    echo          Keep an eye on it. THEY always strike when you're not watching.
    color 0E
) else (
    echo.
    echo  [BATTY] CPU looks okay. !CPU_LOAD!%% load. For now. FOR NOW.
    color 0A
)
echo.

:: ── RAM INFO ─────────────────────────────────────────────────────────────────
echo  [BATTY] Scanning memory banks... this is where they hide things...
echo.
for /f "tokens=2 delims==" %%A in ('wmic OS get TotalVisibleMemorySize /value 2^>nul') do (
    set /a "RAM_TOTAL_MB=%%A / 1024"
)
for /f "tokens=2 delims==" %%A in ('wmic OS get FreePhysicalMemory /value 2^>nul') do (
    set /a "RAM_FREE_MB=%%A / 1024"
)
set /a "RAM_USED_MB=!RAM_TOTAL_MB! - !RAM_FREE_MB!"
set /a "RAM_PCT=(!RAM_USED_MB! * 100) / !RAM_TOTAL_MB!"

echo  Total RAM: !RAM_TOTAL_MB! MB
echo  Used:      !RAM_USED_MB! MB  (!RAM_PCT!%%)
echo  Free:      !RAM_FREE_MB! MB

if !RAM_PCT! GTR 90 (
    echo.
    echo  [BATTY] MEMORY CRITICAL! !RAM_PCT!%% used! They're filling your head
    echo          with JUNK just like they filled mine! CLOSE SOMETHING NOW!
    color 0C
) else if !RAM_PCT! GTR 75 (
    echo.
    echo  [BATTY] RAM at !RAM_PCT!%%. Getting crowded in there. Like a lab cage.
    echo          I know what crowded feels like. Trust me.
    color 0E
) else (
    echo.
    echo  [BATTY] Memory looks healthy. !RAM_PCT!%% used. Plenty of room to think.
    echo          Unlike what THEY left me with after the experiments.
    color 0A
)
echo.

:: ── DISK HEALTH ──────────────────────────────────────────────────────────────
echo  [BATTY] Checking disk... the storage unit... where secrets are buried...
echo.
for /f "usebackq tokens=1,2,3" %%A in (`wmic logicaldisk where "DriveType=3" get DeviceID^,Size^,FreeSpace /format:list 2^>nul ^| findstr "="`) do (
    echo  %%A %%B %%C
)

:: Get C: drive specifically
for /f "tokens=2 delims==" %%A in ('wmic logicaldisk where "DeviceID='C:'" get Size /value 2^>nul') do (
    set /a "DISK_TOTAL_GB=%%A / 1073741824" 2>nul
)
for /f "tokens=2 delims==" %%A in ('wmic logicaldisk where "DeviceID='C:'" get FreeSpace /value 2^>nul') do (
    set /a "DISK_FREE_GB=%%A / 1073741824" 2>nul
)
set /a "DISK_USED_GB=!DISK_TOTAL_GB! - !DISK_FREE_GB!" 2>nul
if !DISK_TOTAL_GB! GTR 0 (
    set /a "DISK_PCT=(!DISK_USED_GB! * 100) / !DISK_TOTAL_GB!"
) else (
    set "DISK_PCT=0"
)

echo.
echo  C: Drive — Total: !DISK_TOTAL_GB! GB  Used: !DISK_USED_GB! GB  Free: !DISK_FREE_GB! GB  (!DISK_PCT!%% full)

if !DISK_PCT! GTR 95 (
    echo.
    echo  [BATTY] DISK ALMOST FULL! !DISK_PCT!%%! This is how it starts!
    echo          First the disk fills up, then EVERYTHING STOPS. I'VE SEEN IT.
    color 0C
) else if !DISK_PCT! GTR 80 (
    echo.
    echo  [BATTY] Disk at !DISK_PCT!%%. Getting tight. Like a cage. A DIGITAL CAGE.
    echo          Consider cleaning up. I cleaned up my act. Mostly.
    color 0E
) else (
    echo.
    echo  [BATTY] Disk looks fine. !DISK_PCT!%% full. You have room. Use it wisely.
    echo          Unlike THEM who wasted perfectly good bat brain space on experiments.
    color 0A
)
echo.

:: ── NETWORK ──────────────────────────────────────────────────────────────────
echo  [BATTY] Pinging the outside world... if it's still there...
echo.
ping -n 1 8.8.8.8 >nul 2>&1
if %errorlevel% EQU 0 (
    echo  [BATTY] Network is UP. Google responded. THEY'RE ALWAYS LISTENING.
    echo          But at least your internet works. Silver lining.
    color 0A
) else (
    echo  [BATTY] NO NETWORK RESPONSE! Either you're offline or THEY cut the line!
    echo          Check your cables. Check your router. Check EVERYTHING.
    color 0C
)
echo.

:: ── UPTIME ───────────────────────────────────────────────────────────────────
echo  [BATTY] Checking how long this machine has been running...
echo.
for /f "tokens=2 delims==" %%A in ('wmic OS get LastBootUpTime /value 2^>nul') do (
    set "BOOT_TIME=%%A"
)
echo  Last Boot: !BOOT_TIME:~0,4!-!BOOT_TIME:~4,2!-!BOOT_TIME:~6,2! at !BOOT_TIME:~8,2!:!BOOT_TIME:~10,2!:!BOOT_TIME:~12,2!
echo.
echo  [BATTY] Reboots are healthy. Like sleep. I never sleep. THEY took that from me.
echo.

:: ── TEMPERATURE (if available via OpenHardwareMonitor WMI) ───────────────────
echo  [BATTY] Attempting temperature scan... my internal thermometer was removed
echo          in experiment 7 but I'll try anyway...
echo.
wmic /namespace:\\root\OpenHardwareMonitor path Sensor where "SensorType='Temperature'" get Name,Value /format:list 2>nul | findstr /i "cpu\|core\|temp" >nul 2>&1
if %errorlevel% EQU 0 (
    for /f "tokens=1,2 delims==" %%A in ('wmic /namespace:\\root\OpenHardwareMonitor path Sensor where "SensorType=''Temperature''" get Name^,Value /format:list 2^>nul') do (
        echo  %%A = %%B C
    )
) else (
    echo  [BATTY] Temperature sensors not accessible without OpenHardwareMonitor.
    echo          Download it from openhardwaremonitor.org and run as admin.
    echo          I'd tell you the temp myself but THEY removed my thermoreceptors.
)
echo.

:: ── RUNNING PROCESSES (top memory consumers) ─────────────────────────────────
echo  [BATTY] Scanning for suspicious processes... you never know what's lurking...
echo.
echo  Top 5 memory consumers:
echo  -------------------------
wmic process get Name,WorkingSetSize /format:list 2>nul | findstr "WorkingSetSize\|Name" | sort /r | head >nul 2>&1
for /f "skip=1 tokens=1,2,3,4,5,6,7,8,9,10 delims= " %%A in ('tasklist /fo table /nh /fi "STATUS eq running" 2^>nul ^| sort /r /+65 ^| findstr /v "^$" ^| head') do (
    echo  %%A
)
tasklist /fo table /nh 2>nul | sort /r /+64 | findstr /v "^$" | (for /l %%i in (1,1,5) do set /p line= && echo  !line!)
echo.
echo  [BATTY] If you see anything called "EXPERIMENT_7.exe" — RUN.
echo.

:: ── SUMMARY ──────────────────────────────────────────────────────────────────
color 0A
echo  =========================================================================
echo  [BATTY] HEALTH REPORT COMPLETE
echo  =========================================================================
echo.
echo  CPU Load:    !CPU_LOAD!%%
echo  RAM Used:    !RAM_PCT!%%  (!RAM_USED_MB! / !RAM_TOTAL_MB! MB)
echo  Disk C:      !DISK_PCT!%%  (!DISK_USED_GB! / !DISK_TOTAL_GB! GB)
echo.

:: Overall verdict
set "ISSUES=0"
if !CPU_LOAD! GTR 90 set /a "ISSUES+=1"
if !RAM_PCT! GTR 90 set /a "ISSUES+=1"
if !DISK_PCT! GTR 95 set /a "ISSUES+=1"

if !ISSUES! EQU 0 (
    color 0A
    echo  [BATTY] VERDICT: YOUR PC IS HEALTHY. I'm as surprised as you are.
    echo          Everything checks out. For now. FOR NOW.
    echo          "I may be a little scrambled but I know a healthy system when I see one."
) else if !ISSUES! EQU 1 (
    color 0E
    echo  [BATTY] VERDICT: ONE ISSUE DETECTED. Not a crisis. Yet.
    echo          Address it before THEY notice the weakness.
    echo          "I've survived worse. So can your PC."
) else (
    color 0C
    echo  [BATTY] VERDICT: MULTIPLE ISSUES! THIS IS WHAT THEY WANT!
    echo          Your system is under stress. Take action NOW.
    echo          "In the lab they said I was overreacting. I WAS NOT OVERREACTING."
)

echo.
echo  =========================================================================
echo  BATTY — BUBO OS Agent Suite ^| Built for Landon ^| NO MAS DISADVANTAGED
echo  "They may have scrambled my radar. But they never scrambled my heart."
echo  =========================================================================
echo.
echo  Press any key to exit... or don't. I'll be here either way.
pause >nul
endlocal
