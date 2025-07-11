#include "Engine.h"
#include "Logger.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    try {
        std::cout << "Starting Nexus Engine..." << std::endl;
        
        Nexus::Engine engine;
        
        std::cout << "==================================" << std::endl;
        std::cout << "    Nexus Game Engine v1.0.0     " << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Features:" << std::endl;
        std::cout << "- DirectX 11 Rendering" << std::endl;
        std::cout << "- Normal Mapping" << std::endl;
        std::cout << "- Light Bloom Effects" << std::endl;
        std::cout << "- Heat Haze Distortion" << std::endl;
        std::cout << "- Enhanced Textures" << std::endl;
        std::cout << "- Unified Shadow System" << std::endl;
        std::cout << "- Python Scripting" << std::endl;
        std::cout << "- Console Support" << std::endl;
        std::cout << "==================================" << std::endl;
        
        std::cout << "About to initialize engine..." << std::endl;
        std::cout.flush();
        
        // Initialize engine
        if (!engine.Initialize()) {
            std::cerr << "Failed to initialize Nexus Engine!" << std::endl;
            return -1;
        }
        
        std::cout << "Engine initialized successfully!" << std::endl;
        std::cout << "Press ESC to exit the engine window." << std::endl;
        
        // Run main loop
        engine.Run();
        
        std::cout << "Engine shutdown complete." << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Unknown exception occurred!" << std::endl;
        return -1;
    }
}
