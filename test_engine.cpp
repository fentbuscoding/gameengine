#include <iostream>
#include <windows.h>
#include <d3d11.h>
#include <directxmath.h>

int main() {
    std::cout << "Starting Nexus Engine Test..." << std::endl;
    
    try {
        // Test basic window creation
        WNDCLASSEXA wc = {};
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
            switch (msg) {
                case WM_DESTROY:
                    PostQuitMessage(0);
                    return 0;
                case WM_KEYDOWN:
                    if (wParam == VK_ESCAPE) {
                        PostQuitMessage(0);
                    }
                    return 0;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        };
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = "NexusEngineTest";

        if (!RegisterClassExA(&wc)) {
            std::cerr << "Failed to register window class" << std::endl;
            return -1;
        }

        HWND hwnd = CreateWindowExA(
            0,
            "NexusEngineTest",
            "Nexus Game Engine Test",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            1280, 720,
            nullptr, nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );

        if (!hwnd) {
            std::cerr << "Failed to create window" << std::endl;
            return -1;
        }

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        std::cout << "Window created successfully!" << std::endl;
        
        // Test basic D3D11 initialization
        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount = 1;
        scd.BufferDesc.Width = 1280;
        scd.BufferDesc.Height = 720;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = hwnd;
        scd.SampleDesc.Count = 1;
        scd.Windowed = TRUE;
        scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* context = nullptr;
        IDXGISwapChain* swapChain = nullptr;
        D3D_FEATURE_LEVEL featureLevel;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &scd,
            &swapChain,
            &device,
            &featureLevel,
            &context
        );

        if (FAILED(hr)) {
            std::cerr << "Failed to create D3D11 device and swap chain" << std::endl;
            return -1;
        }

        std::cout << "D3D11 initialized successfully!" << std::endl;
        
        // Clean up
        if (context) context->Release();
        if (swapChain) swapChain->Release();
        if (device) device->Release();
        
        DestroyWindow(hwnd);
        UnregisterClassA("NexusEngineTest", GetModuleHandle(nullptr));
        
        std::cout << "Test completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Unknown exception!" << std::endl;
        return -1;
    }
}
