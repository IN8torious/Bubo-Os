@echo off
:: =============================================================================
:: BATTY.BAT — PC Health Agent
:: Designed after Batty Koda from FernGully: The Last Rainforest
:: Voiced by Robin Williams.
::
:: Batty was hurt in the lab. His radar was scrambled.
:: He stutters. He glitches. He loses the thread and finds it again.
:: But he shows up. Every single time. And he tells you the truth.
::
:: ACCESSIBILITY: Voice is ON by default.
:: First run asks your preference. Choice is saved to batty_config.txt.
:: People who need the voice should never have to go looking for a setting.
::
:: Part of BUBO OS Agent Suite — Alchemical Framework
:: Built for Landon Pankuch. NO MAS DISADVANTAGED.
:: Copyright (c) 2025 Nathan Brown — MIT License
:: =============================================================================

setlocal enabledelayedexpansion
title BATTY — PC Health Monitor [BUBO OS]
color 0A

:: ── Resolve script directory ─────────────────────────────────────────────────
set "BATTY_DIR=%~dp0"
set "VOICE_DIR=%BATTY_DIR%batty_voice"
set "CONFIG_FILE=%BATTY_DIR%batty_config.txt"

:: ── Voice preference ─────────────────────────────────────────────────────────
:: Default: voice ON (accessibility first — people who need it should have it)
set "VOICE_MODE=on"

if exist "%CONFIG_FILE%" (
    set /p VOICE_MODE=<"%CONFIG_FILE%"
) else (
    :: First run — ask the user
    cls
    echo.
    echo  ╔══════════════════════════════════════════════════════════════════╗
    echo  ║  BATTY — First Run                                               ║
    echo  ║                                                                  ║
    echo  ║  Hey. Before I start... do you want me to speak out loud?        ║
    echo  ║                                                                  ║
    echo  ║  Voice is on by default because some people need it.             ║
    echo  ║  You can always change this by editing batty_config.txt          ║
    echo  ║                                                                  ║
    echo  ║  Press V  — Voice on, always  (saves your choice)               ║
    echo  ║  Press S  — Silent mode        (saves your choice)               ║
    echo  ║  Press Enter — Voice just this once                              ║
    echo  ╚══════════════════════════════════════════════════════════════════╝
    echo.
    choice /c VSE /n /m "  Your choice: "
    if errorlevel 3 (
        set "VOICE_MODE=once"
    ) else if errorlevel 2 (
        set "VOICE_MODE=off"
        echo off>"%CONFIG_FILE%"
    ) else (
        set "VOICE_MODE=on"
        echo on>"%CONFIG_FILE%"
    )
)

:: ── Voice playback helper ─────────────────────────────────────────────────────
:: Uses PowerShell to play audio non-blocking so scan continues while speaking
:: Only plays if voice mode is on or once
:speak
    set "SPEAK_FILE=%VOICE_DIR%\%~1"
    if /i "!VOICE_MODE!"=="off" goto :eof
    if exist "!SPEAK_FILE!" (
        powershell -c "(New-Object Media.SoundPlayer '!SPEAK_FILE!').PlaySync()" >nul 2>&1
    )
goto :eof

:: ── Banner ────────────────────────────────────────────────────────────────────
cls
echo.
echo  ██████╗  █████╗ ████████╗████████╗██╗   ██╗
echo  ██╔══██╗██╔══██╗╚══██╔══╝╚══██╔══╝╚██╗ ██╔╝
echo  ██████╔╝███████║   ██║      ██║    ╚████╔╝
echo  ██╔══██╗██╔══██║   ██║      ██║     ╚██╔╝
echo  ██████╔╝██║  ██║   ██║      ██║      ██║
echo  ╚═════╝ ╚═╝  ╚═╝   ╚═╝      ╚═╝      ╚═╝
echo.
echo  [ BUBO OS PC Health Agent ]  [ Alchemical Framework ]
echo  [ "They scrambled my radar. But not my loyalty." ]
echo  =========================================================================
echo.

call :speak 01_intro.wav
timeout /t 1 /nobreak >nul

:: ── CPU ───────────────────────────────────────────────────────────────────────
echo  [BATTY] C — CPU. The brain. The — the thinking part.
call :speak 02_cpu.wav

for /f "tokens=2 delims==" %%A in ('wmic cpu get Name /value 2^>nul') do set "CPU_NAME=%%A"
for /f "tokens=2 delims==" %%A in ('wmic cpu get NumberOfCores /value 2^>nul') do set "CPU_CORES=%%A"
for /f "tokens=2 delims==" %%A in ('wmic cpu get NumberOfLogicalProcessors /value 2^>nul') do set "CPU_THREADS=%%A"
for /f "tokens=2 delims==" %%A in ('wmic cpu get MaxClockSpeed /value 2^>nul') do set "CPU_SPEED=%%A"
for /f "tokens=2 delims==" %%A in ('wmic cpu get LoadPercentage /value 2^>nul') do set "CPU_LOAD=%%A"
if not defined CPU_LOAD set "CPU_LOAD=0"

echo  CPU ........ !CPU_NAME!
echo  Cores ...... !CPU_CORES! physical / !CPU_THREADS! logical
echo  Speed ...... !CPU_SPEED! MHz
echo  Load ....... !CPU_LOAD!%%
echo.

if !CPU_LOAD! GTR 90 (
    color 0C
    echo  [BATTY] !CPU_LOAD!%% load. That's — that's not okay. Close something. Please.
    call :speak 03_cpu_critical.wav
) else if !CPU_LOAD! GTR 70 (
    color 0E
    echo  [BATTY] !CPU_LOAD!%%. Elevated. Keep an eye on it.
) else (
    color 0A
    echo  [BATTY] !CPU_LOAD!%%. Good. Really — really good.
    call :speak 04_cpu_ok.wav
)
echo.

:: ── RAM ───────────────────────────────────────────────────────────────────────
color 0A
echo  [BATTY] Memory. The — the storage of thoughts.
call :speak 05_ram.wav

for /f "tokens=2 delims==" %%A in ('wmic OS get TotalVisibleMemorySize /value 2^>nul') do set /a "RAM_TOTAL_MB=%%A / 1024"
for /f "tokens=2 delims==" %%A in ('wmic OS get FreePhysicalMemory /value 2^>nul') do set /a "RAM_FREE_MB=%%A / 1024"
set /a "RAM_USED_MB=!RAM_TOTAL_MB! - !RAM_FREE_MB!"
if !RAM_TOTAL_MB! GTR 0 (set /a "RAM_PCT=(!RAM_USED_MB! * 100) / !RAM_TOTAL_MB!") else (set "RAM_PCT=0")

echo  Total ...... !RAM_TOTAL_MB! MB
echo  Used ....... !RAM_USED_MB! MB  (!RAM_PCT!%%)
echo  Free ....... !RAM_FREE_MB! MB
echo.

if !RAM_PCT! GTR 90 (
    color 0C
    echo  [BATTY] !RAM_PCT!%% used. Memory is almost gone. Close something.
    call :speak 06_ram_critical.wav
) else if !RAM_PCT! GTR 75 (
    color 0E
    echo  [BATTY] !RAM_PCT!%%. Getting crowded. Consider closing some things.
) else (
    color 0A
    echo  [BATTY] !RAM_PCT!%% used. Plenty of room. Good.
    call :speak 07_ram_ok.wav
)
echo.

:: ── DISK ──────────────────────────────────────────────────────────────────────
color 0A
echo  [BATTY] Disk. Long-term storage. Where things are kept.
call :speak 08_disk.wav

for /f "tokens=2 delims==" %%A in ('wmic logicaldisk where "DeviceID='C:'" get Size /value 2^>nul') do set /a "DISK_TOTAL_GB=%%A / 1073741824" 2>nul
for /f "tokens=2 delims==" %%A in ('wmic logicaldisk where "DeviceID='C:'" get FreeSpace /value 2^>nul') do set /a "DISK_FREE_GB=%%A / 1073741824" 2>nul
if not defined DISK_TOTAL_GB set "DISK_TOTAL_GB=1"
if not defined DISK_FREE_GB set "DISK_FREE_GB=0"
set /a "DISK_USED_GB=!DISK_TOTAL_GB! - !DISK_FREE_GB!"
set /a "DISK_PCT=(!DISK_USED_GB! * 100) / !DISK_TOTAL_GB!"

echo  C: Total ... !DISK_TOTAL_GB! GB
echo  Used ....... !DISK_USED_GB! GB  (!DISK_PCT!%%)
echo  Free ....... !DISK_FREE_GB! GB
echo.

if !DISK_PCT! GTR 95 (
    color 0C
    echo  [BATTY] !DISK_PCT!%% full. Critical. Clean up your drive.
    call :speak 09_disk_critical.wav
) else if !DISK_PCT! GTR 80 (
    color 0E
    echo  [BATTY] !DISK_PCT!%%. Getting full. Think about what you actually need.
) else (
    color 0A
    echo  [BATTY] !DISK_PCT!%% full. Good. Plenty of space.
    call :speak 10_disk_ok.wav
)
echo.

:: ── NETWORK ───────────────────────────────────────────────────────────────────
color 0A
echo  [BATTY] Network. The outside world. Checking connection...
call :speak 11_network.wav

ping -n 1 8.8.8.8 >nul 2>&1
if %errorlevel% EQU 0 (
    color 0A
    echo  [BATTY] Connected. The outside world responded.
    call :speak 12_network_ok.wav
) else (
    color 0C
    echo  [BATTY] No response. Connection is down. Check your router.
    call :speak 13_network_down.wav
)
echo.

:: ── UPTIME ────────────────────────────────────────────────────────────────────
color 0A
echo  [BATTY] Uptime. How long has this machine been running?
for /f "tokens=2 delims==" %%A in ('wmic OS get LastBootUpTime /value 2^>nul') do set "BOOT_TIME=%%A"
echo  Last Boot: !BOOT_TIME:~0,4!-!BOOT_TIME:~4,2!-!BOOT_TIME:~6,2! at !BOOT_TIME:~8,2!:!BOOT_TIME:~10,2!
echo  [BATTY] Reboot when you can. Fresh starts matter.
echo.

:: ── PROCESSES ─────────────────────────────────────────────────────────────────
color 0A
echo  [BATTY] Top processes by memory:
echo  -------------------------------------------------------------------------
tasklist /fo table /nh 2>nul | sort /r /+64 | findstr /v "^$" > %TEMP%\batty_procs.tmp 2>nul
set "COUNT=0"
for /f "tokens=1,5 delims= " %%A in (%TEMP%\batty_procs.tmp) do (
    if !COUNT! LSS 5 (
        echo    %%A  [%%B K]
        set /a "COUNT+=1"
    )
)
del %TEMP%\batty_procs.tmp >nul 2>&1
echo  [BATTY] If you see EXPERIMENT_7.EXE in there — do not click it.
echo.

:: ── VERDICT ───────────────────────────────────────────────────────────────────
color 0A
echo  =========================================================================
echo  [BATTY] HEALTH REPORT
echo  =========================================================================
echo  CPU ........ !CPU_LOAD!%%
echo  RAM ........ !RAM_PCT!%%  (!RAM_USED_MB! / !RAM_TOTAL_MB! MB)
echo  Disk C: .... !DISK_PCT!%%  (!DISK_USED_GB! / !DISK_TOTAL_GB! GB)
echo.

set "ISSUES=0"
if !CPU_LOAD! GTR 90 set /a "ISSUES+=1"
if !RAM_PCT! GTR 90 set /a "ISSUES+=1"
if !DISK_PCT! GTR 95 set /a "ISSUES+=1"

if !ISSUES! EQU 0 (
    color 0A
    echo  [BATTY] VERDICT: Your PC is healthy. Everything checks out.
    echo          "Some people call it falling. I call it flying."
    echo          Your machine is flying.
    call :speak 14_verdict_healthy.wav
) else if !ISSUES! EQU 1 (
    color 0E
    echo  [BATTY] VERDICT: One issue needs attention. You can handle it.
    call :speak 15_verdict_warning.wav
) else (
    color 0C
    echo  [BATTY] VERDICT: Multiple issues found. Please take action.
    call :speak 16_verdict_critical.wav
)

echo.
echo  =========================================================================
echo  BATTY — BUBO OS Agent Suite  ^|  Alchemical Framework
echo  Built for Landon Pankuch  ^|  NO MAS DISADVANTAGED
echo  "They scrambled my radar. But they never scrambled my heart."
echo  =========================================================================
echo.

call :speak 17_goodbye.wav
echo  [BATTY] I will be here when you need me. Just call.
echo.
echo  Press any key to close.
pause >nul
endlocal
