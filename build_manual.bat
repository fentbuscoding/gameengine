@echo off
echo Building Nexus Game Engine (Simple Build without CMake)...

:: Set paths
set "DXSDK_DIR=C:\Users\Doggle\DirectX"
set "PROJECT_DIR=%~dp0"
set "BUILD_DIR=%PROJECT_DIR%build"

:: Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: Find Visual Studio
set "VS_PATH="
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do (
    set "VS_PATH=%%i"
)

if "%VS_PATH%"=="" (
    echo Visual Studio not found! Trying fallback...
    if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
    ) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community"
    ) else (
        echo Visual Studio not found! Please install Visual Studio with C++ tools.
        pause
        exit /b 1
    )
)

echo Using Visual Studio at: %VS_PATH%

:: Setup Visual Studio environment
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

:: Set include and library paths
set "INCLUDE_PATHS=/I"%PROJECT_DIR%include" /I"%DXSDK_DIR%\Include""
set "LIB_PATHS=/LIBPATH:"%DXSDK_DIR%\Lib\x64""

:: Compile core engine as static library
echo Compiling core engine...
cd "%BUILD_DIR%"

:: Compile all .cpp files
cl /nologo /c /EHsc /std:c++17 /MD /O2 %INCLUDE_PATHS% ^
   "%PROJECT_DIR%src\core\Engine.cpp" ^
   "%PROJECT_DIR%src\utils\Logger.cpp" ^
   "%PROJECT_DIR%src\utils\Timer.cpp" ^
   "%PROJECT_DIR%src\platform\Platform.cpp" ^
   "%PROJECT_DIR%src\audio\AudioDevice.cpp" ^
   "%PROJECT_DIR%src\input\InputManager.cpp" ^
   "%PROJECT_DIR%src\scripting\ScriptingEngine.cpp" ^
   "%PROJECT_DIR%src\utils\ResourceManager.cpp"

if %ERRORLEVEL% NEQ 0 (
    echo Core compilation failed!
    pause
    exit /b 1
)

:: Try to compile graphics with DirectX
echo Compiling graphics components...
cl /nologo /c /EHsc /std:c++17 /MD /O2 %INCLUDE_PATHS% /DNEXUS_DIRECTX_SUPPORT ^
   "%PROJECT_DIR%src\graphics\GraphicsDevice.cpp" ^
   "%PROJECT_DIR%src\graphics\Texture.cpp" ^
   "%PROJECT_DIR%src\graphics\Shader.cpp" ^
   "%PROJECT_DIR%src\graphics\Mesh.cpp" ^
   "%PROJECT_DIR%src\graphics\Camera.cpp" ^
   "%PROJECT_DIR%src\graphics\Light.cpp"

if %ERRORLEVEL% NEQ 0 (
    echo Graphics compilation failed, trying without DirectX...
    cl /nologo /c /EHsc /std:c++17 /MD /O2 %INCLUDE_PATHS% ^
       "%PROJECT_DIR%src\graphics\GraphicsDevice.cpp" ^
       "%PROJECT_DIR%src\graphics\Texture.cpp" ^
       "%PROJECT_DIR%src\graphics\Shader.cpp" ^
       "%PROJECT_DIR%src\graphics\Mesh.cpp" ^
       "%PROJECT_DIR%src\graphics\Camera.cpp" ^
       "%PROJECT_DIR%src\graphics\Light.cpp"
)

:: Compile subsystems
echo Compiling subsystems...
cl /nologo /c /EHsc /std:c++17 /MD /O2 %INCLUDE_PATHS% /DNEXUS_DIRECTX_SUPPORT ^
   "%PROJECT_DIR%src\audio\AudioSystem.cpp" ^
   "%PROJECT_DIR%src\physics\PhysicsEngine.cpp" ^
   "%PROJECT_DIR%src\ai\AISystem.cpp" ^
   "%PROJECT_DIR%src\graphics\LightingEngine.cpp" ^
   "%PROJECT_DIR%src\graphics\AnimationSystem.cpp" ^
   "%PROJECT_DIR%src\graphics\ParticleSystem.cpp" ^
   "%PROJECT_DIR%src\motion\MotionControlSystem.cpp"

if %ERRORLEVEL% NEQ 0 (
    echo Subsystems compilation failed!
    pause
    exit /b 1
)

:: Create static library
echo Creating static library...
lib /nologo /out:NexusCore.lib *.obj

if %ERRORLEVEL% NEQ 0 (
    echo Library creation failed!
    pause
    exit /b 1
)

:: Compile main executable
echo Compiling main executable...
cl /nologo /EHsc /std:c++17 /MD /O2 %INCLUDE_PATHS% ^
   "%PROJECT_DIR%src\main.cpp" ^
   NexusCore.lib ^
   user32.lib gdi32.lib winmm.lib %LIB_PATHS% ^
   /Fe:NexusEngine.exe

if %ERRORLEVEL% NEQ 0 (
    echo Main executable compilation failed!
    pause
    exit /b 1
)

:: Compile basic demo
echo Compiling basic demo...
cl /nologo /EHsc /std:c++17 /MD /O2 %INCLUDE_PATHS% ^
   "%PROJECT_DIR%examples\basic_demo\main.cpp" ^
   NexusCore.lib ^
   user32.lib gdi32.lib winmm.lib %LIB_PATHS% ^
   /Fe:BasicDemo.exe

if %ERRORLEVEL% NEQ 0 (
    echo Demo compilation failed (continuing anyway)
)

echo.
echo Build completed successfully!
echo Executables:
if exist NexusEngine.exe echo   - NexusEngine.exe
if exist BasicDemo.exe echo   - BasicDemo.exe
echo.
echo To run: cd build && NexusEngine.exe
echo.
pause
