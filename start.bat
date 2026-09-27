@echo off
REM ============================================================
REM  HAB01 Ground Station Dashboard — One-Click Launcher
REM  Starts: Backend API, Frontend Dev Server, Serial Bridge
REM ============================================================

echo.
echo  ===== HAB01 Ground Station Dashboard =====
echo.

REM --- Check Python ---
where python >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Python not found. Install Python 3.10+ and add to PATH.
    pause
    exit /b 1
)

REM --- Check Node ---
where node >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Node.js not found. Install Node.js 18+ and add to PATH.
    pause
    exit /b 1
)

REM --- Install Python deps ---
echo [1/5] Installing Python dependencies...
pip install -r backend\requirements.txt -q 2>nul
pip install -r bridge\requirements.txt -q 2>nul

REM --- Install Node deps ---
echo [2/5] Installing frontend dependencies...
cd frontend
call npm install --silent 2>nul
cd ..

REM --- Create data directory ---
if not exist data mkdir data

REM --- Start Backend ---
echo [3/5] Starting backend API server...
start "HAB-Backend" cmd /k "cd backend && python run.py"
timeout /t 3 /nobreak >nul

REM --- Start Frontend ---
echo [4/5] Starting frontend dev server...
start "HAB-Frontend" cmd /k "cd frontend && npm run dev"
timeout /t 3 /nobreak >nul

REM --- Prompt for serial mode ---
echo.
echo [5/5] Serial Bridge Options:
echo   1. Connect to serial port (real hardware)
echo   2. Run flight simulator (demo mode)
echo   3. Skip (start manually later)
echo.
set /p MODE="Select option (1/2/3): "

if "%MODE%"=="1" (
    set /p PORT="Enter COM port (e.g. COM3): "
    start "HAB-Bridge" cmd /k "cd bridge && python serial_bridge.py --port %PORT%"
) else if "%MODE%"=="2" (
    start "HAB-Bridge" cmd /k "cd bridge && python simulate_flight.py"
) else (
    echo Skipping serial bridge. Start manually when ready.
)

echo.
echo  Dashboard is running!
echo  Open http://localhost:5173 in your browser.
echo.
pause
