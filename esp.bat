@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "IDF_EXPORT=C:\Users\16013\esp\v5.5.5\esp-idf\export.bat"
set "ESP_PORT=COM7"

if not exist "%IDF_EXPORT%" (
    echo [ERROR] ESP-IDF environment script not found:
    echo         %IDF_EXPORT%
    exit /b 1
)

call "%IDF_EXPORT%"
if errorlevel 1 exit /b %errorlevel%

cd /d "%PROJECT_DIR%"
set "ACTION=%~1"
if not defined ACTION set "ACTION=build"

if /i "%ACTION%"=="build"   goto build
if /i "%ACTION%"=="flash"   goto flash
if /i "%ACTION%"=="monitor" goto monitor
if /i "%ACTION%"=="all"     goto all
if /i "%ACTION%"=="clean"   goto clean
if /i "%ACTION%"=="menuconfig" goto menuconfig

echo Usage: esp.bat [build^|flash^|monitor^|all^|clean^|menuconfig]
exit /b 2

:build
idf.py build
exit /b %errorlevel%

:flash
idf.py -p %ESP_PORT% flash
exit /b %errorlevel%

:monitor
idf.py -p %ESP_PORT% monitor
exit /b %errorlevel%

:all
idf.py -p %ESP_PORT% build flash monitor
exit /b %errorlevel%

:clean
idf.py fullclean
exit /b %errorlevel%

:menuconfig
idf.py menuconfig
exit /b %errorlevel%
