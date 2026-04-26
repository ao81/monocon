@echo off
setlocal
REM ============================================================
REM ArduinoUploader build script
REM ============================================================

echo Building ArduinoUploader...

REM CMake チェック
where cmake >nul 2>&1
if errorlevel 1 (
    echo Error: CMake not found. Please install CMake and add it to PATH.
    echo Download from: https://cmake.org/download/
    exit /b 1
)

REM ビルドディレクトリ
if not exist build mkdir build
pushd build

REM プロジェクト生成
echo Generating project files...
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo Error: CMake generation failed
    popd
    exit /b 1
)

REM ビルド (並列ジョブ)
echo Building project...
cmake --build . --config Release -j
if errorlevel 1 (
    echo Error: Build failed
    popd
    exit /b 1
)

popd
echo.
echo Build completed successfully!
echo Executable: build\bin\ArduinoUploader.exe
endlocal