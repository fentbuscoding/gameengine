@echo off
echo ========================================
echo Nexus Engine - Clean Rebuild Script
echo ========================================
echo.

echo Cleaning build directory...
if exist build (
    echo Removing old build directory...
    rmdir /s /q build
    echo Build directory cleaned.
) else (
    echo No build directory found, skipping cleanup.
)

echo.
echo Creating new build directory...
mkdir build

echo.
echo Running CMake configuration...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo.
    echo ========================================
    echo CMake configuration FAILED!
    echo ========================================
    pause
    exit /b 1
)

echo.
echo ========================================
echo Building Nexus Engine (Release)...
echo ========================================
cmake --build build --config Release --parallel

if errorlevel 1 (
    echo.
    echo ========================================
    echo Build FAILED!
    echo ========================================
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Executable location: build\Release\NexusEngine.exe
echo.
pause
