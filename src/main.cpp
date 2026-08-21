/**
 * @file main.cpp
 * @brief Nexus Engine main entry point with modern RHI multi-API support
 * @author Nexus Engine Team
 * @date 2026-07-22
 * @version 2.0.0
 */

#include "Engine.h"
#include "Logger.h"
#include "RHI/RHIDevice.h"
#include "ScriptingEngine.h"   // Engine.h only forward-declares it.
#include "EngineConfig.h"
// SDL2 is an optional dependency (ENABLE_SDL2 / NEXUS_SDL2_ENABLED). It was
// included unconditionally, so the engine executable could not be built at all
// without it - even for backends that never use SDL.
#ifdef NEXUS_SDL2_ENABLED
    #include <SDL2/SDL.h>
#endif
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <fstream>

namespace {
    /// Tears down SDL when it is compiled in; a no-op otherwise.
    void ShutdownSDL() {
#ifdef NEXUS_SDL2_ENABLED
        SDL_Quit();
#endif
    }

    // Version information
    constexpr const char* ENGINE_VERSION = "2.0.0";
    constexpr const char* BUILD_DATE = __DATE__;
    constexpr const char* BUILD_TIME = __TIME__;

    void PrintBanner() {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "║                  NEXUS GAME ENGINE                      ║\n";
        std::cout << "║                    Version " << ENGINE_VERSION << "                       ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
    }

    void PrintFeatures() {
        std::cout << "🎮 ENGINE FEATURES:\n";

        // Graphics APIs
        std::cout << "  🎨 Graphics APIs:\n";
#ifdef NEXUS_VULKAN_ENABLED
        std::cout << "    ✓ Vulkan 1.3+ (High Performance)\n";
#endif
#ifdef NEXUS_DIRECTX11_ENABLED
        std::cout << "    ✓ DirectX 11 (Windows)\n";
#endif
#ifdef NEXUS_OPENGL_ENABLED
        std::cout << "    ✓ OpenGL 4.5+ (Cross-Platform)\n";
#endif

        // Rendering Features
        std::cout << "  🌟 Rendering:\n";
        std::cout << "    ✓ PBR Materials & Normal Mapping\n";
        std::cout << "    ✓ HDR & Light Bloom Effects\n";
        std::cout << "    ✓ Unified Shadow Mapping\n";
#ifdef NEXUS_RAY_TRACING_ENABLED
        std::cout << "    ✓ Ray Tracing Support\n";
#endif

        // Core Systems
        std::cout << "  ⚙️  Core Systems:\n";
#ifdef NEXUS_BULLET_PHYSICS_ENABLED
        std::cout << "    ✓ Bullet Physics Engine\n";
#endif
#ifdef NEXUS_PHYSX_ENABLED
        std::cout << "    ✓ NVIDIA PhysX\n";
#endif
        std::cout << "    ✓ AI & Animation Systems\n";

        // Scripting
        std::cout << "  📜 Scripting:\n";
#ifdef NEXUS_PYTHON_ENABLED
        std::cout << "    ✓ Python 3.9+ Support\n";
#else
        std::cout << "    ⚠ Python (Disabled)\n";
#endif
#ifdef NEXUS_LUA_ENABLED
        std::cout << "    ✓ Lua 5.4 Support\n";
#endif

        // Import Support
        std::cout << "  📦 Asset Import:\n";
        std::cout << "    ✓ Unity Projects\n";
        std::cout << "    ✓ Unreal Engine Projects\n";
        std::cout << "    ✓ Godot Projects\n";
        std::cout << "    ✓ Source Engine Content (NEW!)\n";

        std::cout << "\n";
    }

    void PrintControls() {
        std::cout << "🎯 CONTROLS:\n";
        std::cout << "  WASD     - Move camera\n";
        std::cout << "  Q/E      - Move camera up/down\n";
        std::cout << "  Mouse    - Look around\n";
        std::cout << "  SPACE    - Trigger action\n";
        std::cout << "  F1       - Toggle debug info\n";
        std::cout << "  F2       - Switch graphics API (if multiple enabled)\n";
        std::cout << "  F11      - Toggle fullscreen\n";
        std::cout << "  ESC      - Exit application\n\n";
    }

    void PrintUsage(const char* programName) {
        std::cout << "📖 USAGE:\n";
        std::cout << "  " << programName << " [options] [script]\n\n";
        std::cout << "  Options:\n";
        std::cout << "    --help, -h              Show this help message\n";
        std::cout << "    --version, -v           Show version information\n";
        std::cout << "    --fullscreen, -f        Start in fullscreen mode\n";
        std::cout << "    --resolution WxH        Set window resolution (e.g., 1920x1080)\n";
        std::cout << "    --api <vulkan|dx11|opengl|auto>  Choose graphics API\n";
        std::cout << "    --config FILE           Use custom config file\n";
        std::cout << "    --debug, -d             Enable debug mode\n";
        std::cout << "    --vsync <on|off>        Enable/disable VSync\n";
        std::cout << "    --import-unity PATH     Import Unity project\n";
        std::cout << "    --import-unreal PATH    Import Unreal project\n";
        std::cout << "    --import-source PATH    Import Source Engine content\n\n";
        std::cout << "  Examples:\n";
        std::cout << "    " << programName << " demo.py\n";
        std::cout << "    " << programName << " --api vulkan --fullscreen\n";
        std::cout << "    " << programName << " --resolution 1920x1080 --vsync off\n";
        std::cout << "    " << programName << " --import-unity \"C:/UnityProjects/MyGame\"\n\n";
    }

    void PrintVersionInfo() {
        std::cout << "Nexus Game Engine " << ENGINE_VERSION << "\n";
        std::cout << "Built on " << BUILD_DATE << " at " << BUILD_TIME << "\n";
        std::cout << "Copyright (c) 2026 Nexus Engine Team\n\n";

        std::cout << "Build Configuration:\n";
#ifdef _DEBUG
        std::cout << "  Build Type: Debug\n";
#else
        std::cout << "  Build Type: Release\n";
#endif

        // Graphics APIs
        std::cout << "  Graphics APIs:\n";
#ifdef NEXUS_VULKAN_ENABLED
        std::cout << "    ✓ Vulkan\n";
#endif
#ifdef NEXUS_DIRECTX11_ENABLED
        std::cout << "    ✓ DirectX 11\n";
#endif
#ifdef NEXUS_OPENGL_ENABLED
        std::cout << "    ✓ OpenGL\n";
#endif

        // Scripting
#ifdef NEXUS_PYTHON_ENABLED
        std::cout << "  Python Support: Enabled\n";
#else
        std::cout << "  Python Support: Disabled\n";
#endif
#ifdef NEXUS_LUA_ENABLED
        std::cout << "  Lua Support: Enabled\n";
#endif

        // Platform
#ifdef _WIN32
        std::cout << "  Platform: Windows\n";
#elif defined(__linux__)
        std::cout << "  Platform: Linux\n";
#elif defined(__APPLE__)
        std::cout << "  Platform: macOS\n";
#else
        std::cout << "  Platform: Unknown\n";
#endif
        std::cout << "\n";
    }

    struct CommandLineArgs {
        std::string scriptFile;
        std::string configFile;
        std::string importUnityPath;
        std::string importUnrealPath;
        std::string importSourcePath;
        Nexus::RHI::GraphicsAPI graphicsAPI = Nexus::RHI::GraphicsAPI::Auto;
        bool showHelp = false;
        bool showVersion = false;
        bool fullscreen = false;
        bool debugMode = false;
        bool vsync = true;
        int windowWidth = 1280;
        int windowHeight = 720;
    };

    CommandLineArgs ParseCommandLine(int argc, char* argv[]) {
        CommandLineArgs args;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--help" || arg == "-h") {
                args.showHelp = true;
            }
            else if (arg == "--version" || arg == "-v") {
                args.showVersion = true;
            }
            else if (arg == "--fullscreen" || arg == "-f") {
                args.fullscreen = true;
            }
            else if (arg == "--debug" || arg == "-d") {
                args.debugMode = true;
            }
            else if (arg == "--api" && i + 1 < argc) {
                std::string api = argv[++i];
                if (api == "vulkan") args.graphicsAPI = Nexus::RHI::GraphicsAPI::Vulkan;
                else if (api == "dx11" || api == "directx11") args.graphicsAPI = Nexus::RHI::GraphicsAPI::DirectX11;
                else if (api == "opengl" || api == "gl") args.graphicsAPI = Nexus::RHI::GraphicsAPI::OpenGL;
                else if (api == "auto") args.graphicsAPI = Nexus::RHI::GraphicsAPI::Auto;
                else std::cerr << "⚠ Unknown graphics API: " << api << " (using auto)\n";
            }
            else if (arg == "--vsync" && i + 1 < argc) {
                std::string vsyncArg = argv[++i];
                args.vsync = (vsyncArg == "on" || vsyncArg == "1" || vsyncArg == "true");
            }
            else if (arg == "--resolution" && i + 1 < argc) {
                std::string resolution = argv[++i];
                size_t xPos = resolution.find('x');
                if (xPos != std::string::npos) {
                    try {
                        args.windowWidth = std::stoi(resolution.substr(0, xPos));
                        args.windowHeight = std::stoi(resolution.substr(xPos + 1));
                    } catch (const std::exception&) {
                        std::cerr << "⚠ Invalid resolution format: " << resolution << "\n";
                    }
                }
            }
            else if (arg == "--config" && i + 1 < argc) {
                args.configFile = argv[++i];
            }
            else if (arg == "--import-unity" && i + 1 < argc) {
                args.importUnityPath = argv[++i];
            }
            else if (arg == "--import-unreal" && i + 1 < argc) {
                args.importUnrealPath = argv[++i];
            }
            else if (arg == "--import-source" && i + 1 < argc) {
                args.importSourcePath = argv[++i];
            }
            else if (!arg.empty() && arg[0] != '-') {
                args.scriptFile = arg;
            }
            else {
                std::cerr << "⚠ Unknown argument: " << arg << "\n";
            }
        }

        return args;
    }

    bool ValidateArgs(const CommandLineArgs& args) {
        if (!args.scriptFile.empty()) {
            std::ifstream file(args.scriptFile);
            if (!file.good()) {
                std::cerr << "❌ Script file not found: " << args.scriptFile << "\n";
                return false;
            }
        }

        if (!args.configFile.empty()) {
            std::ifstream file(args.configFile);
            if (!file.good()) {
                std::cerr << "❌ Config file not found: " << args.configFile << "\n";
                return false;
            }
        }

        if (args.windowWidth < 640 || args.windowHeight < 480) {
            std::cerr << "❌ Resolution too small. Minimum: 640x480\n";
            return false;
        }

        return true;
    }

    const char* GetAPIName(Nexus::RHI::GraphicsAPI api) {
        switch (api) {
            case Nexus::RHI::GraphicsAPI::Vulkan: return "Vulkan";
            case Nexus::RHI::GraphicsAPI::DirectX11: return "DirectX 11";
            case Nexus::RHI::GraphicsAPI::OpenGL: return "OpenGL";
            default: return "Auto";
        }
    }

    bool HandleImports(const CommandLineArgs& args) {
        if (!args.importUnityPath.empty()) {
            std::cout << "📦 Importing Unity project: " << args.importUnityPath << "\n";
            // TODO: Call Unity importer
            std::cout << "⚠ Unity import functionality coming soon!\n";
            std::cout << "   See docs/GAME_IMPORTER_GUIDE.md for manual import instructions.\n\n";
            return true; // Don't start engine after import
        }

        if (!args.importUnrealPath.empty()) {
            std::cout << "📦 Importing Unreal project: " << args.importUnrealPath << "\n";
            // TODO: Call Unreal importer
            std::cout << "⚠ Unreal import functionality coming soon!\n";
            std::cout << "   See docs/GAME_IMPORTER_GUIDE.md for manual import instructions.\n\n";
            return true;
        }

        if (!args.importSourcePath.empty()) {
            std::cout << "📦 Importing Source Engine content: " << args.importSourcePath << "\n";
            // TODO: Call Source Engine importer
            std::cout << "⚠ Source Engine import functionality coming soon!\n";
            std::cout << "   See docs/GAME_IMPORTER_GUIDE.md for manual import instructions.\n\n";
            return true;
        }

        return false; // No import, continue with engine
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    auto args = ParseCommandLine(argc, argv);

    // Handle help and version requests
    if (args.showHelp) {
        PrintBanner();
        PrintUsage(argv[0]);
        return 0;
    }

    if (args.showVersion) {
        PrintVersionInfo();
        return 0;
    }

    // Validate arguments
    if (!ValidateArgs(args)) {
        return -1;
    }

    // Handle import operations
    if (HandleImports(args)) {
        return 0; // Exit after import
    }

    // Initialize console output
    std::cout << std::fixed << std::setprecision(2);

    try {
        PrintBanner();

#ifdef NEXUS_SDL2_ENABLED
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
            std::cerr << "❌ Failed to initialize SDL: " << SDL_GetError() << "\n";
            return -1;
        }
#endif

        std::cout << "🎨 Graphics API: " << GetAPIName(args.graphicsAPI) << "\n";
        PrintFeatures();

        // Create engine instance
        Nexus::Engine engine;

        // Configure engine based on command line arguments
        if (args.debugMode) {
            std::cout << "🔧 Debug mode enabled\n";
            Nexus::Logger::SetLogLevel(Nexus::LogLevel::Debug);
        }

        std::cout << "🚀 INITIALIZING ENGINE...\n";
        auto startTime = std::chrono::high_resolution_clock::now();

        // Initialize with custom config if provided
        bool initSuccess = args.configFile.empty() ?
            engine.Initialize() :
            engine.Initialize(args.configFile);

        if (!initSuccess) {
            std::cerr << "❌ Failed to initialize Nexus Engine!\n";
            std::cerr << "   Check logs/engine.log for detailed error information.\n";
            ShutdownSDL();
            return -1;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::cout << "✅ Engine initialized successfully in " << duration.count() << "ms\n";

        // Get actual graphics API in use
        // auto* graphics = engine.GetGraphics();
        // if (graphics) {
        //     std::cout << "   Using: " << graphics->GetAPIName() << "\n";
        // }
        std::cout << "\n";

        // Execute script if provided
        if (!args.scriptFile.empty()) {
#ifdef NEXUS_PYTHON_ENABLED
            std::cout << "🐍 Loading Python script: " << args.scriptFile << "\n";
            auto* scripting = engine.GetScripting();
            if (scripting) {
                scripting->EnableHotReload(true);
                if (!scripting->ExecuteFile(args.scriptFile)) {
                    std::cerr << "❌ Failed to execute script: " << args.scriptFile << "\n";
                    std::cerr << "   Check script syntax and logs/engine.log for details.\n";
                    engine.Shutdown();
                    ShutdownSDL();
                    return -1;
                }
                std::cout << "✅ Script loaded successfully\n\n";
            } else {
                std::cerr << "❌ Scripting engine not available\n";
                engine.Shutdown();
                ShutdownSDL();
                return -1;
            }
#else
            std::cerr << "❌ Python support not enabled in this build\n";
            std::cerr << "   Cannot execute script: " << args.scriptFile << "\n";
            std::cerr << "   Rebuild with -DENABLE_PYTHON=ON\n";
            engine.Shutdown();
            ShutdownSDL();
            return -1;
#endif
        } else {
            std::cout << "🎮 Starting with built-in demo\n\n";
            PrintControls();
        }

        std::cout << "▶️  STARTING MAIN LOOP...\n";
        std::cout << "   (Press ESC to exit)\n\n";
        std::cout.flush();

        // Start the main game loop
        engine.Run();

        std::cout << "\n✅ Engine main loop completed successfully\n";

        // Cleanup
        engine.Shutdown();
        ShutdownSDL();

        std::cout << "👋 Nexus Engine shutdown complete\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n❌ FATAL ERROR: " << e.what() << "\n";
        std::cerr << "   The application will now exit.\n";
        std::cerr << "   Check logs/engine.log for more details.\n";
        ShutdownSDL();
        return -1;
    } catch (...) {
        std::cerr << "\n❌ UNKNOWN FATAL ERROR occurred!\n";
        std::cerr << "   The application will now exit.\n";
        std::cerr << "   Check logs/engine.log for more details.\n";
        ShutdownSDL();
        return -1;
    }
}
