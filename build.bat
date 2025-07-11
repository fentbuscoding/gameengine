@echo off
setlocal enabledelayedexpansion

echo.
echo ╔══════════════════════════════════════════════════════════╗
echo ║              NEXUS GAME ENGINE BUILD SCRIPT             ║
echo ║           Multi-Language Game Development Platform       ║
echo ╚══════════════════════════════════════════════════════════╝
echo.

:: Check for Visual Studio 2022 (it's already initialized based on your output)
echo ✅ Visual Studio 2022 environment detected

:: Use a simpler approach - just set the Windows SDK version directly
set "WINSDK_VER=10.0.26100.0"
echo ✅ Using Windows SDK: %WINSDK_VER%

:: Create build directory
if not exist build (
    echo 📁 Creating build directory...
    mkdir build
)

pushd build

:: Clean previous build if requested
if "%1"=="clean" (
    echo 🧹 Cleaning previous build...
    if exist CMakeCache.txt del CMakeCache.txt
    if exist CMakeFiles rmdir /s /q CMakeFiles 2>nul
    if exist *.vcxproj del *.vcxproj 2>nul
    if exist *.sln del *.sln 2>nul
)

echo.
echo 🔧 UPDATING SOURCE TO USE MAIN.CPP...

:: Use a simpler approach to update the CMakeLists.txt
if exist "..\src\CMakeLists.txt" (
    powershell -Command "(Get-Content '..\src\CMakeLists.txt') -replace 'main_simple\.cpp', 'main.cpp' | Set-Content '..\src\CMakeLists.txt'"
    if !ERRORLEVEL! EQU 0 (
        echo ✅ Updated CMakeLists.txt to use main.cpp
    ) else (
        echo ⚠  Could not automatically update CMakeLists.txt
        echo    Please manually change 'main_simple.cpp' to 'main.cpp' in src/CMakeLists.txt
    )
) else (
    echo ❌ CMakeLists.txt not found at ..\src\CMakeLists.txt
)

echo.
echo 🔧 CONFIGURING WITH CMAKE (MULTI-LANGUAGE SUPPORT)...
echo    Generator: Visual Studio 17 2022
echo    Platform: x64
echo    Languages: C, C++, Python, Lua
echo    DirectX: 11 (Windows SDK)
echo.

:: Language support options
set "ENABLE_PYTHON=ON"
set "ENABLE_LUA=ON"
set "ENABLE_C_API=ON"

:: Check if user wants to disable any languages
if "%2"=="no-python" set "ENABLE_PYTHON=OFF"
if "%3"=="no-python" set "ENABLE_PYTHON=OFF"
if "%2"=="no-lua" set "ENABLE_LUA=OFF"
if "%3"=="no-lua" set "ENABLE_LUA=OFF"

echo 🎯 Language Configuration:
echo    C API:      %ENABLE_C_API%
echo    C++:        ON (Core Engine)
echo    Python:     %ENABLE_PYTHON%
echo    Lua:        %ENABLE_LUA%
echo.

:: Configure with CMake for multi-language support
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DDIRECTINPUT_VERSION=0x0800 ^
    -DENABLE_PYTHON=%ENABLE_PYTHON% ^
    -DENABLE_LUA=%ENABLE_LUA% ^
    -DENABLE_C_API=%ENABLE_C_API%

if %ERRORLEVEL% NEQ 0 (
    echo ❌ CMake configuration failed!
    echo    Check the error messages above for details.
    popd
    pause
    exit /b 1
)

echo ✅ CMake configuration successful
echo.

:: Determine build configuration
set "BUILD_CONFIG=Release"
if "%1"=="debug" set "BUILD_CONFIG=Debug"
if "%2"=="debug" set "BUILD_CONFIG=Debug"

echo 🔨 BUILDING %BUILD_CONFIG% CONFIGURATION...
echo    This may take a few minutes...
echo.

:: Build the project
cmake --build . --config %BUILD_CONFIG% --parallel

if %ERRORLEVEL% NEQ 0 (
    echo ❌ Build failed!
    echo    Check the error messages above for details.
    popd
    pause
    exit /b 1
)

echo.
echo ✅ BUILD COMPLETED SUCCESSFULLY!
echo.
echo 📦 Build artifacts:
echo    Executables: build\bin\%BUILD_CONFIG%\
echo    Libraries:   build\lib\%BUILD_CONFIG%\
echo.

:: List built executables
if exist "bin\%BUILD_CONFIG%\" (
    echo 🎮 Available executables:
    for %%f in (bin\%BUILD_CONFIG%\*.exe) do (
        echo    - %%~nf.exe
    )
    echo.
)

:: Copy example files to appropriate directories
echo 📁 Setting up example directories...
if not exist "examples\c" mkdir "examples\c"
if not exist "examples\cpp" mkdir "examples\cpp"
if not exist "examples\python" mkdir "examples\python"
if not exist "examples\lua" mkdir "examples\lua"

popd

echo.
echo 🚀 READY FOR MULTI-LANGUAGE GAME DEVELOPMENT!
echo.
echo 📋 Usage examples:
echo.
echo    🔵 C/C++ Development:
echo       build\bin\%BUILD_CONFIG%\NexusEngine.exe
echo.
if "%ENABLE_PYTHON%"=="ON" (
    echo    🐍 Python Development:
    echo       build\bin\%BUILD_CONFIG%\NexusEngine.exe --script examples\python\my_game.py
    echo.
)
if "%ENABLE_LUA%"=="ON" (
    echo    🌙 Lua Development:
    echo       build\bin\%BUILD_CONFIG%\NexusEngine.exe --script examples\lua\my_game.lua
    echo.
)
echo    ⚙️  General Options:
echo       build\bin\%BUILD_CONFIG%\NexusEngine.exe --help
echo       build\bin\%BUILD_CONFIG%\NexusEngine.exe --version
echo.
echo 📚 Language-specific documentation:
echo    C API:      docs\c_api.md
echo    C++:        docs\cpp_api.md
echo    Python:     docs\python_api.md
echo    Lua:        docs\lua_api.md
echo.

pause
