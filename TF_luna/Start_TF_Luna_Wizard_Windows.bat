@echo off
setlocal
chcp 65001 >nul
title TF-Luna Configuration Wizard
cd /d "%~dp0"

where py >nul 2>nul
if not errorlevel 1 (
    py -3 -c "import sys; raise SystemExit(0 if sys.version_info >= (3,8) else 1)"
    if not errorlevel 1 (
        py -3 "%~dp0configure_tf_luna.py" --wizard
        goto finished
    )
)

where python >nul 2>nul
if not errorlevel 1 (
    python -c "import sys; raise SystemExit(0 if sys.version_info >= (3,8) else 1)"
    if not errorlevel 1 (
        python "%~dp0configure_tf_luna.py" --wizard
        goto finished
    )
)

echo.
echo Python 3.8 or newer was not found.
echo Install Python 3 from https://www.python.org/downloads/
echo During Windows installation, enable "Add python.exe to PATH".

:finished
echo.
pause
endlocal
