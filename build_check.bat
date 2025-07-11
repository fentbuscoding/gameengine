@echo off
echo Building Nexus Engine...
echo.

echo Checking for required tools...
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo CMake not found in PATH
    echo Please install CMake or add it to your PATH
    echo.
)

where msbuild >nul 2>&1
if %errorlevel% neq 0 (
    echo MSBuild not found in PATH
    echo Please install Visual Studio or Visual Studio Build Tools
    echo.
)

echo.
echo Available Visual Studio solutions:
dir /b *.sln 2>nul
if %errorlevel% neq 0 (
    echo No Visual Studio solution files found
    echo Please run cmake first to generate build files
    echo.
)

echo.
echo To build manually:
echo 1. Open NexusEngine.sln in Visual Studio
echo 2. Build the solution (Ctrl+Shift+B)
echo.
echo Or install CMake and run:
echo cmake --build build --config Release
echo.

pause
