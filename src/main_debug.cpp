#include "Engine.h"
#include "Logger.h"
#include <iostream>
#include <string>
#include <chrono>
#include <fstream>

// Enhanced logging for debugging
void DebugLog(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ofstream debugFile("engine_debug.log", std::ios::app);
    if (debugFile.is_open()) {
        debugFile << "[" << time_t << "] " << message << std::endl;
        debugFile.close();
    }
    
    std::cout << "[DEBUG] " << message << std::endl;
    std::cout.flush();
}

int main(int argc, char* argv[]) {
    DebugLog("=== NEXUS ENGINE DEBUG MODE STARTED ===");
    
    try {
        DebugLog("Creating Engine instance...");
        Nexus::Engine engine;
        
        DebugLog("Engine instance created successfully");
        DebugLog("Starting engine initialization...");
        
        // Add a timeout mechanism
        auto startTime = std::chrono::high_resolution_clock::now();
        bool initSuccess = false;
        
        // Try initialization in a separate thread (simulation)
        DebugLog("Calling engine.Initialize()...");
        initSuccess = engine.Initialize();
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        if (initSuccess) {
            DebugLog("Engine initialized successfully in " + std::to_string(duration.count()) + "ms");
            DebugLog("Starting main loop...");
            
            // Run for a limited time to test
            auto loopStart = std::chrono::high_resolution_clock::now();
            const int maxRunTimeSeconds = 5; // Run for 5 seconds max
            
            while (engine.IsRunning()) {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - loopStart);
                
                if (elapsed.count() >= maxRunTimeSeconds) {
                    DebugLog("Debug run time limit reached, exiting...");
                    engine.RequestExit();
                    break;
                }
                
                // Check for ESC key
                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                    DebugLog("ESC key pressed, exiting...");
                    engine.RequestExit();
                    break;
                }
                
                Sleep(16); // ~60 FPS
            }
            
            DebugLog("Main loop completed");
        } else {
            DebugLog("ERROR: Engine initialization failed after " + std::to_string(duration.count()) + "ms");
            return -1;
        }
        
        DebugLog("Engine shutting down...");
        
    } catch (const std::exception& e) {
        DebugLog("EXCEPTION: " + std::string(e.what()));
        return -1;
    } catch (...) {
        DebugLog("UNKNOWN EXCEPTION occurred!");
        return -1;
    }
    
    DebugLog("=== NEXUS ENGINE DEBUG MODE COMPLETED ===");
    return 0;
}