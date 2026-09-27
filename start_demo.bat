@echo off
REM ============================================================
REM  HAB01 Dashboard — Quick Demo Mode
REM  Starts backend + frontend + flight simulator
REM ============================================================

echo.
echo  ===== HAB01 Dashboard - DEMO MODE =====
echo.

REM --- Create data directory ---
if not exist data mkdir data

REM --- Start Backend ---
echo [1/3] Starting backend...
start "HAB-Backend" cmd /k "cd backend && python run.py"
timeout /t 4 /nobreak >nul

REM --- Start Simulator ---
echo [2/3] Starting flight simulator...
start "HAB-Simulator" cmd /k "cd bridge && python simulate_flight.py"

REM --- Start Frontend ---
echo [3/3] Starting frontend...
start "HAB-Frontend" cmd /k "cd frontend && npm run dev"

echo.
echo  Demo running! Open http://localhost:5173
echo  The simulator will fly a complete HAB mission over ~5 minutes.
echo.
pause
