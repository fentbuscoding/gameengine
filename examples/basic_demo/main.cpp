#include "Engine.h"
#include <iostream>

int main() {
    std::cout << "==================================" << std::endl;
    std::cout << "   Nexus Engine - Basic Demo     " << std::endl;
    std::cout << "==================================" << std::endl;
    
    Nexus::Engine engine;
    
    if (!engine.Initialize()) {
        std::cerr << "Failed to initialize engine!" << std::endl;
        return -1;
    }
    
    std::cout << "Engine initialized successfully!" << std::endl;
    std::cout << "Press ESC to exit" << std::endl;
    
    // Run the engine
    engine.Run();
    
    return 0;
}
