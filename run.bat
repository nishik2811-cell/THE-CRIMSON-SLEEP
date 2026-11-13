@echo off
setlocal
cd /d "%~dp0"

echo ========================================================
echo               THE CRIMSON SLEEP
echo ========================================================
echo.

:: Ensure MinGW / MSYS2 UCRT64 toolchain and SFML libraries are on PATH
set "PATH=C:\msys64\ucrt64\bin;%PATH%"

:: Check if the executable exists, if not build it
if not exist "crimson_sleep.exe" (
    echo [INFO] Executable not found. Compiling project...
    mingw32-make
    if errorlevel 1 (
        echo.
        echo [ERROR] Build failed!
        pause
        exit /b 1
    )
    echo [INFO] Build successful!
    echo.
)

:: Launch the game
echo [INFO] Launching The Crimson Sleep...
crimson_sleep.exe %*
