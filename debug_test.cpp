#include <iostream>
#include <windows.h>
#include <d3d11.h>
#include <chrono>
#include <fstream>

void LogMessage(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ofstream logFile("debug_test.log", std::ios::app);
    if (logFile.is_open()) {
        logFile << "[" << time_t << "] " << message << std::endl;
        logFile.close();
    }
    
    std::cout << message << std::endl;
    std::cout.flush();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
}

int main() {
    LogMessage("=== ENGINE DEBUG TEST STARTED ===");
    
    try {
        LogMessage("Step 1: Registering window class...");
        
        // Register window class
        WNDCLASSEXA wc = {};
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = "DebugTest";

        if (!RegisterClassExA(&wc)) {
            LogMessage("ERROR: Failed to register window class");
            return -1;
        }
        
        LogMessage("Step 2: Creating window...");
        
        // Create window
        HWND hwnd = CreateWindowExA(
            0,
            "DebugTest",
            "Engine Debug Test",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            800, 600,
            nullptr, nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );

        if (!hwnd) {
            LogMessage("ERROR: Failed to create window");
            return -1;
        }

        LogMessage("Step 3: Showing window...");
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        LogMessage("Step 4: Creating DirectX 11 device...");
        
        // Create DirectX 11 device and swap chain
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 1;
        swapChainDesc.BufferDesc.Width = 800;
        swapChainDesc.BufferDesc.Height = 600;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hwnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;

        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* context = nullptr;
        IDXGISwapChain* swapChain = nullptr;
        D3D_FEATURE_LEVEL featureLevel;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &swapChainDesc, &swapChain, &device, &featureLevel, &context
        );

        if (FAILED(hr)) {
            LogMessage("ERROR: Failed to create DirectX 11 device and swap chain. HRESULT: " + std::to_string(hr));
            return -1;
        }

        LogMessage("Step 5: DirectX 11 device created successfully!");
        
        LogMessage("Step 6: Starting message loop...");
        
        // Message loop
        MSG msg = {};
        int frameCount = 0;
        auto startTime = std::chrono::high_resolution_clock::now();
        
        while (true) {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                
                if (msg.message == WM_QUIT) {
                    LogMessage("Received WM_QUIT message");
                    goto exit_loop;
                }
            }

            // Simple render
            float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
            ID3D11RenderTargetView* renderTargetView = nullptr;
            
            // Get back buffer
            ID3D11Texture2D* backBuffer = nullptr;
            hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
            if (SUCCEEDED(hr)) {
                hr = device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
                backBuffer->Release();
                
                if (SUCCEEDED(hr)) {
                    context->ClearRenderTargetView(renderTargetView, clearColor);
                    swapChain->Present(1, 0);
                    renderTargetView->Release();
                }
            }
            
            frameCount++;
            if (frameCount % 60 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
                LogMessage("Running for " + std::to_string(duration.count()) + " seconds, frames: " + std::to_string(frameCount));
            }
            
            Sleep(16); // ~60 FPS
        }

exit_loop:
        LogMessage("Step 7: Cleaning up...");
        
        // Cleanup
        if (context) context->Release();
        if (swapChain) swapChain->Release();
        if (device) device->Release();
        
        DestroyWindow(hwnd);
        UnregisterClassA("DebugTest", GetModuleHandle(nullptr));
        
        LogMessage("=== ENGINE DEBUG TEST COMPLETED ===");
        
        return 0;
        
    } catch (const std::exception& e) {
        LogMessage("EXCEPTION: " + std::string(e.what()));
        return -1;
    } catch (...) {
        LogMessage("UNKNOWN EXCEPTION OCCURRED");
        return -1;
    }
}