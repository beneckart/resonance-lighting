@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -STA -File "%~dp0perimeter_gobo_usb.ps1" %*
if errorlevel 1 (
  echo.
  echo The perimeter gobo utility stopped with an error.
  pause
)
