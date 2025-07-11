@echo off
echo Building Nexus Game Engine (Simplified Build)...

:: Check if we have Python available
python --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Python not found in PATH! Please install Python or add it to PATH.
    echo Your Python installation was found at: C:\Program Files\WindowsApps\PythonSoftwareFoundation.Python.3.12_3.12.2800.0_x64__qbz5n2kfra8p0
    echo You may need to enable Python in Windows App Execution Aliases
    pause
    exit /b 1
)

:: Install pybind11 if not already installed
echo Installing pybind11...
pip install pybind11[global] --quiet

:: Create build directory
if not exist build mkdir build
cd build

:: Try to find Visual Studio
set "VS_PATH="
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if "%VS_PATH%"=="" (
    echo Visual Studio not found! Please install Visual Studio with C++ tools.
    pause
    exit /b 1
)

:: Setup Visual Studio environment
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

:: Try CMake first
cmake --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using CMake build...
    cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
    if %ERRORLEVEL% NEQ 0 (
        echo CMake configuration failed!
        goto :fallback_build
    )
    
    cmake --build . --config Release
    if %ERRORLEVEL% NEQ 0 (
        echo CMake build failed!
        goto :fallback_build
    )
    
    echo CMake build completed successfully!
    goto :success
) else (
    echo CMake not found, using fallback build...
    goto :fallback_build
)

:fallback_build
echo Building with Visual Studio compiler directly...

:: Create simplified build without DirectX dependencies
cl /nologo /EHsc /std:c++17 /I..\include ^
   ..\src\core\Engine.cpp ^
   ..\src\utils\Logger.cpp ^
   ..\src\utils\Timer.cpp ^
   ..\src\platform\Platform.cpp ^
   ..\examples\basic_demo\main.cpp ^
   /Fe:NexusEngine.exe ^
   user32.lib gdi32.lib winmm.lib

if %ERRORLEVEL% NEQ 0 (
    echo Fallback build failed!
    pause
    exit /b 1
)

echo Fallback build completed successfully!

:success
echo.
echo Build completed! 
if exist NexusEngine.exe (
    echo Executable: build\NexusEngine.exe
) else if exist bin\Release\NexusEngine.exe (
    echo Executable: build\bin\Release\NexusEngine.exe
) else (
    echo Executable location may vary
)
echo.
pause
