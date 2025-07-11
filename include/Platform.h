#pragma once

// Platform-specific includes and definitions
#ifdef _WIN32
    // Define these before including Windows headers
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    
    #ifndef STRICT
    #define STRICT
    #endif
    
    // Include Windows headers first
    #include <windows.h>
    #include <objbase.h>
    #include <initguid.h>
    
    // DirectX 11 includes (order is important!)
    #include <dxgi.h>
    #include <d3d11.h>
    #include <d3dcompiler.h>
    #include <d3d11sdklayers.h>
    #include <d3d11_1.h>
    #include <dinput.h>
    // Include mmreg.h before dsound.h to avoid WAVEFORMATEX conflicts
    #include <mmreg.h>
    #include <dsound.h>
    #include <DirectXMath.h>
    
    // Link DirectX libraries
    #pragma comment(lib, "d3d11.lib")
    #pragma comment(lib, "dxgi.lib")
    #pragma comment(lib, "d3dcompiler.lib")
    #pragma comment(lib, "dinput8.lib")
    #pragma comment(lib, "dsound.lib")
    #pragma comment(lib, "dxguid.lib")
    
    using namespace DirectX;
    
    // Type aliases for compatibility with legacy DirectX 9 code
    typedef XMFLOAT2 D3DXVECTOR2;
    typedef XMFLOAT3 D3DXVECTOR3;
    typedef XMFLOAT4 D3DXVECTOR4;
    typedef XMMATRIX D3DXMATRIX;
    typedef XMFLOAT4 D3DXQUATERNION;
    
    // Windows-specific types
    typedef HWND WindowHandle;
    typedef HINSTANCE InstanceHandle;
#else
    // Platform abstraction for other platforms
    typedef void* WindowHandle;
    typedef void* InstanceHandle;
#endif

// Standard library includes
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <fstream>
#include <thread>
#include <mutex>

namespace Nexus {

/**
 * Platform abstraction layer with DirectX integration
 */
class Platform {
public:
    static bool Initialize();
    static void Shutdown();
    static std::string GetPlatformName();
    static bool IsConsoleSupported();
    static void SetConsoleMode(bool enabled);
    
    // Window management
    static WindowHandle CreateGameWindow(const std::string& title, int width, int height);
    static void DestroyGameWindow(WindowHandle window);
    static bool ProcessMessages();
    
    // Timing
    static double GetTime();
    static void Sleep(int milliseconds);
    
    // File system
    static bool FileExists(const std::string& path);
    static std::string GetExecutablePath();
    
private:
    static bool isInitialized_;
};

} // namespace Nexus
