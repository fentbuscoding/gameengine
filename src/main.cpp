#include "Engine.h"
#include "Logger.h"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>

namespace {
    // Version information
    constexpr const char* ENGINE_VERSION = "1.0.0";
    constexpr const char* BUILD_DATE = __DATE__;
    constexpr const char* BUILD_TIME = __TIME__;
    
    void PrintBanner() {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "║                  NEXUS GAME ENGINE                      ║\n";
        std::cout << "║                    Version " << ENGINE_VERSION << "                         ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
    }
    
    void PrintFeatures() {
        std::cout << "🎮 ENGINE FEATURES:\n";
        std::cout << "  ✓ DirectX 11 Rendering Pipeline\n";
        std::cout << "  ✓ Normal Mapping & PBR Materials\n";
        std::cout << "  ✓ HDR & Light Bloom Effects\n";
        std::cout << "  ✓ Heat Haze Distortion\n";
        std::cout << "  ✓ Enhanced Texture Loading\n";
        std::cout << "  ✓ Unified Shadow Mapping\n";
        std::cout << "  ✓ Physics Simulation (Bullet)\n";
        std::cout << "  ✓ AI & Animation Systems\n";
#ifdef NEXUS_PYTHON_ENABLED
        std::cout << "  ✓ Python Scripting Support\n";
#else
        std::cout << "  ⚠ Python Scripting (Disabled)\n";
#endif
        std::cout << "  ✓ Multi-Platform Support\n\n";
    }
    
    void PrintControls() {
        std::cout << "🎯 CONTROLS:\n";
        std::cout << "  WASD     - Move camera\n";
        std::cout << "  Q/E      - Move camera up/down\n";
        std::cout << "  Mouse    - Look around\n";
        std::cout << "  SPACE    - Trigger explosion\n";
        std::cout << "  F1       - Toggle debug info\n";
        std::cout << "  F11      - Toggle fullscreen\n";
        std::cout << "  ESC      - Exit application\n\n";
    }
    
    void PrintUsage(const char* programName) {
        std::cout << "📖 USAGE:\n";
        std::cout << "  " << programName << " [options] [script.py]\n\n";
        std::cout << "  Options:\n";
        std::cout << "    --help, -h        Show this help message\n";
        std::cout << "    --version, -v     Show version information\n";
        std::cout << "    --fullscreen, -f  Start in fullscreen mode\n";
        std::cout << "    --resolution WxH  Set window resolution (e.g., 1920x1080)\n";
        std::cout << "    --config FILE     Use custom config file\n";
        std::cout << "    --debug, -d       Enable debug mode\n\n";
        std::cout << "  Examples:\n";
        std::cout << "    " << programName << " demo.py\n";
        std::cout << "    " << programName << " --fullscreen --resolution 1920x1080\n";
        std::cout << "    " << programName << " --config custom.ini mygame.py\n\n";
    }
    
    void PrintVersionInfo() {
        std::cout << "Nexus Game Engine " << ENGINE_VERSION << "\n";
        std::cout << "Built on " << BUILD_DATE << " at " << BUILD_TIME << "\n";
        std::cout << "Copyright (c) 2025 Nexus Engine Team\n\n";
        
        std::cout << "Build Configuration:\n";
#ifdef _DEBUG
        std::cout << "  Build Type: Debug\n";
#else
        std::cout << "  Build Type: Release\n";
#endif
        
#ifdef NEXUS_PYTHON_ENABLED
        std::cout << "  Python Support: Enabled\n";
#else
        std::cout << "  Python Support: Disabled\n";
#endif

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
        bool showHelp = false;
        bool showVersion = false;
        bool fullscreen = false;
        bool debugMode = false;
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
            else if (!arg.empty() && arg[0] != '-') {
                // Assume it's a script file
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
            // Check if script file exists
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
    
    // Initialize console output
    std::cout << std::fixed << std::setprecision(2);
    
    try {
        PrintBanner();
        PrintFeatures();
        
        // Create engine instance
        Nexus::Engine engine;
        
        // Configure engine based on command line arguments
        if (args.debugMode) {
            std::cout << "🔧 Debug mode enabled\n";
            // engine.SetDebugMode(true); // Implement this in Engine class
        }
        
        std::cout << "🚀 INITIALIZING ENGINE...\n";
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Initialize with custom config if provided
        bool initSuccess = args.configFile.empty() ? 
            engine.Initialize() : 
            engine.Initialize(args.configFile);
            
        if (!initSuccess) {
            std::cerr << "❌ Failed to initialize Nexus Engine!\n";
            std::cerr << "   Check the log file for detailed error information.\n";
            return -1;
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        std::cout << "✅ Engine initialized successfully in " << duration.count() << "ms\n\n";
        
        // Execute Python script if provided
        if (!args.scriptFile.empty()) {
#ifdef NEXUS_PYTHON_ENABLED
            std::cout << "🐍 Loading Python script: " << args.scriptFile << "\n";
            auto* scripting = engine.GetScripting();
            if (scripting) {
                scripting->EnableHotReload(true);
                if (!scripting->ExecuteFile(args.scriptFile)) {
                    std::cerr << "❌ Failed to execute script: " << args.scriptFile << "\n";
                    std::cerr << "   Check script syntax and engine log for details.\n";
                    return -1;
                }
                std::cout << "✅ Script loaded successfully\n\n";
            } else {
                std::cerr << "❌ Scripting engine not available\n";
                return -1;
            }
#else
            std::cerr << "❌ Python support not enabled in this build\n";
            std::cerr << "   Cannot execute script: " << args.scriptFile << "\n";
            return -1;
#endif
        } else {
            std::cout << "🎮 Starting with built-in physics demo\n\n";
            PrintControls();
        }
        
        std::cout << "▶️  STARTING MAIN LOOP...\n";
        std::cout << "   (Press ESC to exit)\n\n";
        std::cout.flush();
        
        // Start the main game loop
        engine.Run();
        
        std::cout << "\n✅ Engine main loop completed successfully\n";
        std::cout << "👋 Nexus Engine shutdown complete\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ FATAL ERROR: " << e.what() << "\n";
        std::cerr << "   The application will now exit.\n";
        std::cerr << "   Please check the log file for more details.\n";
        return -1;
    } catch (...) {
        std::cerr << "\n❌ UNKNOWN FATAL ERROR occurred!\n";
        std::cerr << "   The application will now exit.\n";
        std::cerr << "   Please check the log file for more details.\n";
        return -1;
    }
}
