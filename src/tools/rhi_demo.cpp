/**
 * @file rhi_demo.cpp
 * @brief Minimal cross-platform window + RHI host loop.
 *
 * The engine executable (src/main.cpp) drives the legacy Direct3D 11 runtime and
 * only links on Windows, so before this there was nothing runnable on Linux or
 * macOS - the core library built, and that was all anyone could check.
 *
 * This is the smallest program that exercises the cross-platform path end to
 * end: open a window, bring up an RHI device on it, and present frames until
 * asked to stop. It is deliberately small enough to be read in one sitting and
 * to serve as the reference for how the pieces fit together.
 *
 *   NexusRHIDemo                  open a window and run until closed
 *   NexusRHIDemo --frames 60      present 60 frames, then exit (CI smoke test)
 *   NexusRHIDemo --probe          report the compiled-in backends and exit
 *   NexusRHIDemo --api opengl     pick a backend explicitly
 */

#include "Logger.h"
#include "Platform.h"
#include "Window.h"
#include "RHI/RHIDevice.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Options {
    int width = 1280;
    int height = 720;
    int frameLimit = 0;         ///< 0 means run until the window is closed.
    bool vsync = true;
    bool probeOnly = false;
    std::string api = "auto";
};

void PrintUsage() {
    std::cout <<
        "Usage: NexusRHIDemo [options]\n"
        "  --width N          window width  (default 1280)\n"
        "  --height N         window height (default 720)\n"
        "  --frames N         present N frames then exit; 0 = until closed\n"
        "  --api NAME         auto | vulkan | opengl | d3d11 (default auto)\n"
        "  --no-vsync         request a non-blocking present mode\n"
        "  --probe            print the compiled-in backends and exit\n"
        "  --help             show this message\n";
}

/// Returns false when an argument is malformed, having explained why.
bool ParseArgs(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        // Options that take a value need the value to actually be there;
        // reading argv[i + 1] unguarded is a read past the end of argv.
        const auto value = [&](int& out) {
            if (i + 1 >= argc) {
                std::cerr << arg << " requires a value\n";
                return false;
            }
            out = std::atoi(argv[++i]);
            return true;
        };

        if (arg == "--help" || arg == "-h") {
            PrintUsage();
            std::exit(0);
        } else if (arg == "--probe") {
            options.probeOnly = true;
        } else if (arg == "--no-vsync") {
            options.vsync = false;
        } else if (arg == "--width") {
            if (!value(options.width)) return false;
        } else if (arg == "--height") {
            if (!value(options.height)) return false;
        } else if (arg == "--frames") {
            if (!value(options.frameLimit)) return false;
        } else if (arg == "--api") {
            if (i + 1 >= argc) {
                std::cerr << "--api requires a value\n";
                return false;
            }
            options.api = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            PrintUsage();
            return false;
        }
    }

    if (options.width <= 0 || options.height <= 0) {
        std::cerr << "Window dimensions must be positive\n";
        return false;
    }
    if (options.frameLimit < 0) {
        std::cerr << "--frames cannot be negative\n";
        return false;
    }
    return true;
}

Nexus::RHI::GraphicsAPI ParseAPI(const std::string& name, bool& ok) {
    ok = true;
    if (name == "auto")   return Nexus::RHI::GraphicsAPI::Auto;
    if (name == "vulkan") return Nexus::RHI::GraphicsAPI::Vulkan;
    if (name == "opengl") return Nexus::RHI::GraphicsAPI::OpenGL;
    if (name == "d3d11" || name == "directx11") return Nexus::RHI::GraphicsAPI::DirectX11;

    std::cerr << "Unknown graphics API: " << name << "\n";
    ok = false;
    return Nexus::RHI::GraphicsAPI::Auto;
}

void PrintBackends() {
    std::cout << "Platform: " << Nexus::Platform::GetPlatformName() << "\n";
    std::cout << "Windowing: " << (Nexus::Window::IsSupported() ? "SDL2" : "none") << "\n";
    std::cout << "Graphics backends compiled in:\n";
#ifdef NEXUS_VULKAN_ENABLED
    std::cout << "  vulkan\n";
#endif
#ifdef NEXUS_OPENGL_ENABLED
    std::cout << "  opengl\n";
#endif
#ifdef NEXUS_DIRECTX11_ENABLED
    std::cout << "  d3d11\n";
#endif
}

/// The window flag SDL needs depends on which API will draw into it, and it
/// cannot be changed after creation - so this has to be decided up front, from
/// the same choice the RHI factory will make.
Nexus::Window::Backend WindowBackendFor(Nexus::RHI::GraphicsAPI api) {
    switch (api) {
        case Nexus::RHI::GraphicsAPI::OpenGL:
            return Nexus::Window::Backend::OpenGL;
        case Nexus::RHI::GraphicsAPI::Vulkan:
            return Nexus::Window::Backend::Vulkan;
        case Nexus::RHI::GraphicsAPI::Auto:
        default:
#if defined(NEXUS_VULKAN_ENABLED)
            return Nexus::Window::Backend::Vulkan;
#elif defined(NEXUS_OPENGL_ENABLED)
            return Nexus::Window::Backend::OpenGL;
#else
            return Nexus::Window::Backend::None;
#endif
    }
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    if (!ParseArgs(argc, argv, options)) {
        return 2;
    }

    if (options.probeOnly) {
        PrintBackends();
        return 0;
    }

    bool apiOk = false;
    const Nexus::RHI::GraphicsAPI api = ParseAPI(options.api, apiOk);
    if (!apiOk) {
        return 2;
    }

    if (!Nexus::Window::IsSupported()) {
        std::cerr << "This build has no windowing backend. Install SDL2 development "
                     "headers and reconfigure.\n";
        return 1;
    }

    Nexus::Platform::Initialize();

    Nexus::Window::Desc windowDesc;
    windowDesc.title = "Nexus RHI Demo";
    windowDesc.width = options.width;
    windowDesc.height = options.height;
    windowDesc.closeOnEscape = true;
    windowDesc.backend = WindowBackendFor(api);

    std::unique_ptr<Nexus::Window> window = Nexus::Window::Create(windowDesc);
    if (!window) {
        Nexus::Platform::Shutdown();
        return 1;
    }

    std::unique_ptr<Nexus::RHI::RHIDevice> device = Nexus::RHI::RHIDevice::Create(api);
    if (!device) {
        std::cerr << "No graphics backend available. Run with --probe to see what "
                     "this build supports.\n";
        Nexus::Platform::Shutdown();
        return 1;
    }

    // The swap chain is sized from the drawable, not the window: on a HiDPI
    // display these differ, and using the window size gives a swap chain at the
    // wrong resolution.
    int drawableWidth = 0;
    int drawableHeight = 0;
    window->GetDrawableSize(drawableWidth, drawableHeight);

    Nexus::RHI::SwapChainDesc swapChain{};
    swapChain.windowHandle = window->GetNativeHandle();
    swapChain.width = static_cast<uint32_t>(drawableWidth);
    swapChain.height = static_cast<uint32_t>(drawableHeight);
    swapChain.bufferCount = 2;
    swapChain.format = Nexus::RHI::TextureFormat::RGBA8_SRGB;
    swapChain.vsync = options.vsync;
    swapChain.fullscreen = false;

    if (!device->Initialize(swapChain)) {
        std::cerr << "Failed to initialise the " << device->GetAPIName() << " device.\n";
        Nexus::Platform::Shutdown();
        return 1;
    }

    Nexus::Logger::Info(std::string("Running on ") + device->GetAPIName() +
                        " / " + Nexus::Platform::GetPlatformName());

    int presented = 0;
    const double startTime = Nexus::Platform::GetTime();

    while (window->PumpEvents()) {
        int newWidth = 0;
        int newHeight = 0;
        if (window->ConsumeResize(newWidth, newHeight) && newWidth > 0 && newHeight > 0) {
            device->WaitIdle();
            device->ResizeSwapChain(static_cast<uint32_t>(newWidth),
                                    static_cast<uint32_t>(newHeight));
        }

        if (window->IsMinimized()) {
            // Nothing can be presented to a zero-sized drawable; idle instead of
            // spinning on failed acquires.
            Nexus::Platform::Sleep(16);
            continue;
        }

        device->BeginFrame();

        if (device->IsDeviceLost()) {
            // Usually an out-of-date swap chain after a resize the window system
            // reported late. Rebuild it and try again next iteration.
            if (!device->ResetDevice()) {
                Nexus::Platform::Sleep(16);
            }
            continue;
        }

        Nexus::RHI::RHICommandBufferPtr commands = device->CreateCommandBuffer();
        if (!commands) {
            Nexus::Logger::Error("Could not allocate a command buffer");
            break;
        }

        // A slow hue sweep, so a still screenshot is enough to tell that frames
        // are really being produced rather than one clear being left on screen.
        const float t = static_cast<float>(Nexus::Platform::GetTime() - startTime);
        Nexus::RHI::ClearColor color{};
        color.r = 0.5f + 0.5f * std::sin(t * 0.7f);
        color.g = 0.5f + 0.5f * std::sin(t * 0.7f + 2.094f);
        color.b = 0.5f + 0.5f * std::sin(t * 0.7f + 4.189f);
        color.a = 1.0f;

        commands->Begin();
        commands->ClearRenderTarget(device->GetBackBuffer(), color);
        commands->End();

        device->SubmitCommandBuffer(commands.get());
        device->EndFrame();
        device->Present();

        ++presented;
        if (options.frameLimit > 0 && presented >= options.frameLimit) {
            break;
        }
    }

    device->WaitIdle();
    device->Shutdown();
    window.reset();
    Nexus::Platform::Shutdown();

    std::cout << "Presented " << presented << " frames\n";

    // --frames is the CI smoke test: exiting 0 without having presented anything
    // would let a broken swap chain pass silently.
    if (options.frameLimit > 0 && presented < options.frameLimit) {
        std::cerr << "Exited before presenting the requested " << options.frameLimit
                  << " frames\n";
        return 1;
    }
    return 0;
}
