@echo off
REM Test script for ArduinoUploader

if "%1" == "" (
    echo Usage: test.bat [COM_PORT]
    echo Example: test.bat COM3
    exit /b 1
)

set COM_PORT=%1
set SKETCH_NAME=test_sketch
set SKETCH_DIR=%~dp0%SKETCH_NAME%

echo Creating test sketch directory...
if not exist "%SKETCH_DIR%" mkdir "%SKETCH_DIR%"

echo Creating test sketch...
echo void setup() { pinMode(13, OUTPUT); } > "%SKETCH_DIR%\%SKETCH_NAME%.ino"
echo void loop() { digitalWrite(13, HIGH); delay(1000); digitalWrite(13, LOW); delay(1000); } >> "%SKETCH_DIR%\%SKETCH_NAME%.ino"

echo Running ArduinoUploader...
REM 引数の順番を [build] [フォルダ名] [COMポート] に合わせています
ArduinoUploader.exe build "%SKETCH_DIR%" %COM_PORT%

if %errorlevel% equ 0 (
    echo.
    echo Test completed successfully!
) else (
    echo.
    echo Test failed with exit code: %errorlevel%
)

pause