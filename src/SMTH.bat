# NEXUS ENGINE - THIRD-PARTY DEPENDENCIES SETUP GUIDE

## 📁 Third-Party Folder Structure

Create the following folder structure in your `g:\code\gameengine\third_party` directory:

```
third_party/
├── bullet3/                    # Bullet Physics Engine
├── lua/                        # Lua Scripting Language
├── physx/                      # NVIDIA PhysX Physics
├── imgui/                      # Dear ImGui UI Library
├── fmod/                       # FMOD Audio Engine
├── assimp/                     # Asset Import Library
├── pybind11/                   # Python C++ Bindings
├── stb/                        # STB Image/Audio Libraries
├── rapidjson/                  # JSON Parser
├── glfw/                       # OpenGL Framework (optional)
├── rtx/                        # NVIDIA RTX SDK
├── wwise/                      # Audiokinetic Wwise
├── steamaudio/                 # Steam Audio SDK
└── directxtk/                  # DirectX Toolkit
```

## 🔗 DOWNLOAD LINKS AND SETUP INSTRUCTIONS

### 1. BULLET PHYSICS (Required)
**Download:** https://github.com/bulletphysics/bullet3/releases/latest
- File: `bullet3-X.X.zip` (latest version)
- Extract to: `third_party/bullet3/`
- License: zlib License

### 2. LUA SCRIPTING (Required)
**Download:** https://www.lua.org/download.html
- File: `lua-5.4.7.tar.gz` (or latest 5.4.x)
- Extract to: `third_party/lua/`
- Windows Binaries: https://luabinaries.sourceforge.net/download.html
- License: MIT License

### 3. NVIDIA PHYSX (Optional - for advanced physics)
**Download:** https://github.com/NVIDIA-Omniverse/PhysX/releases
- File: Latest PhysX SDK
- Extract to: `third_party/physx/`
- License: BSD 3-Clause

### 4. DEAR IMGUI (Required for UI)
**Download:** https://github.com/ocornut/imgui/releases/latest
- File: `imgui-X.X.zip`
- Extract to: `third_party/imgui/`
- License: MIT License

### 5. FMOD AUDIO ENGINE (Optional - Professional Audio)
**Download:** https://www.fmod.com/download
- File: FMOD Engine (Windows)
- Extract to: `third_party/fmod/`
- License: FMOD License (Free for non-commercial)

### 6. ASSIMP MODEL LOADER
**Download:** https://github.com/assimp/assimp/releases/latest
- File: `assimp-X.X.X-windows-msvc-X.zip`
- Extract to: `third_party/assimp/`
- License: BSD 3-Clause

### 7. PYBIND11 (Required for Python support)
**Download:** https://github.com/pybind/pybind11/releases/latest
- File: `pybind11-X.X.X.zip`
- Extract to: `third_party/pybind11/`
- License: BSD-style

### 8. STB LIBRARIES (Required for texture loading)
**Download:** https://github.com/nothings/stb
- Clone entire repo to: `third_party/stb/`
- License: MIT License / Public Domain

### 9. RAPIDJSON (Required for configuration)
**Download:** https://github.com/Tencent/rapidjson/releases/latest
- File: `rapidjson-X.X.X.zip`
- Extract to: `third_party/rapidjson/`
- License: MIT License

### 10. DIRECTX TOOLKIT (Recommended)
**Download:** https://github.com/Microsoft/DirectXTK/releases/latest
- File: `DirectXTK_Desktop_202X_XX_XX.zip`
- Extract to: `third_party/directxtk/`
- License: MIT License

### 11. NVIDIA RTX SDK (Optional - for ray tracing)
**Download:** https://developer.nvidia.com/rtx/ray-tracing/rtxgi
- Requires NVIDIA Developer account
- Extract to: `third_party/rtx/`
- License: NVIDIA Software License

### 12. AUDIOKINETIC WWISE (Optional - Professional Audio)
**Download:** https://www.audiokinetic.com/download/
- File: Wwise SDK
- Extract to: `third_party/wwise/`
- License: Audiokinetic License

### 13. STEAM AUDIO (Optional - Spatial Audio)
**Download:** https://github.com/ValveSoftware/steam-audio/releases
- File: Latest Steam Audio SDK
- Extract to: `third_party/steamaudio/`
- License: Apache 2.0

## 🚀 QUICK SETUP BATCH SCRIPT

Create this batch script to automate some downloads:

```batch
@echo off
echo Setting up Nexus Engine third-party dependencies...

mkdir third_party 2>nul
cd third_party

echo Downloading core dependencies...

:: Download ImGui (small, direct download)
echo - Dear ImGui...
curl -L -o imgui.zip https://github.com/ocornut/imgui/archive/refs/heads/master.zip
if exist imgui.zip (
    powershell -command "Expand-Archive -Path imgui.zip -DestinationPath . -Force"
    ren imgui-master imgui
    del imgui.zip
)

:: Download STB (header-only libraries)
echo - STB Libraries...
git clone https://github.com/nothings/stb.git 2>nul || echo STB already exists

:: Download RapidJSON
echo - RapidJSON...
curl -L -o rapidjson.zip https://github.com/Tencent/rapidjson/archive/refs/heads/master.zip
if exist rapidjson.zip (
    powershell -command "Expand-Archive -Path rapidjson.zip -DestinationPath . -Force"
    ren rapidjson-master rapidjson
    del rapidjson.zip
)

echo.
echo ✅ Basic dependencies downloaded!
echo.
echo 📝 MANUAL DOWNLOADS REQUIRED:
echo    1. Bullet Physics: https://github.com/bulletphysics/bullet3/releases
echo    2. Lua: https://www.lua.org/download.html
echo    3. FMOD: https://www.fmod.com/download
echo    4. NVIDIA PhysX: https://github.com/NVIDIA-Omniverse/PhysX
echo.
echo 📁 Extract them to the corresponding folders in third_party/
echo.
pause
```

## 🛠️ CMAKE CONFIGURATION

Add this to your main `CMakeLists.txt`:

```cmake
# Set third-party directory
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

# Find packages
find_package(Bullet QUIET PATHS ${THIRD_PARTY_DIR}/bullet3)
find_package(Lua QUIET PATHS ${THIRD_PARTY_DIR}/lua)

# Include directories
target_include_directories(NexusCore PRIVATE
    ${THIRD_PARTY_DIR}/imgui
    ${THIRD_PARTY_DIR}/stb
    ${THIRD_PARTY_DIR}/rapidjson/include
    ${THIRD_PARTY_DIR}/assimp/include
)

# Conditional includes
if(BULLET_FOUND)
    target_include_directories(NexusCore PRIVATE ${BULLET_INCLUDE_DIRS})
    target_link_libraries(NexusCore ${BULLET_LIBRARIES})
endif()

if(LUA_FOUND)
    target_include_directories(NexusCore PRIVATE ${LUA_INCLUDE_DIR})
    target_link_libraries(NexusCore ${LUA_LIBRARIES})
endif()
```

## 📋 INSTALLATION PRIORITY

**ESSENTIAL (Download First):**
1. ImGui - UI system
2. STB - Image loading
3. RapidJSON - Configuration
4. Bullet Physics - Physics simulation
5. Lua - Scripting support

**RECOMMENDED:**
6. DirectX Toolkit - Enhanced DirectX support
7. Assimp - Model loading
8. PyBind11 - Python integration

**OPTIONAL (Advanced Features):**
9. FMOD - Professional audio
10. PhysX - Advanced physics
11. RTX SDK - Ray tracing
12. Wwise - Professional audio pipeline

## 🔧 BUILD ORDER

1. First, download and set up the ESSENTIAL libraries
2. Run your build to verify core functionality
3. Add RECOMMENDED libraries for enhanced features
4. Finally, add OPTIONAL libraries for professional features

## 💡 TIPS

- Most libraries include pre-built binaries for Windows
- Check each library's documentation for specific build instructions
- Some libraries (like FMOD, Wwise) require registration
- Keep licenses and attribution files for distribution

## 🆘 TROUBLESHOOTING

If you get build errors:
1. Check that include paths are correct in CMakeLists.txt
2. Verify library files are in the right directories
3. Make sure you're using compatible versions (Release/Debug)
4. Check that Windows SDK version matches your setup

## 📞 SUPPORT

For additional help:
- Check each library's official documentation
- Verify CMake finds the libraries: `cmake --build build --verbose`
- Post issues with specific error messages