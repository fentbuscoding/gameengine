#include "GameImporter.h"
#include "UnityImporter.h"
#include "Logger.h"
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::cout << "=== NEXUS ENGINE - UNITY PROJECT IMPORTER ===" << std::endl;
    
    if (argc < 3) {
        std::cout << "Usage: NexusUnityImporter <unity_project_path> <output_path>" << std::endl;
        std::cout << "Example: NexusUnityImporter \"C:/UnityProjects/MyGame\" \"C:/NexusProjects/MyGame\"" << std::endl;
        return 1;
    }
    
    std::string unityProjectPath = argv[1];
    std::string outputPath = argv[2];
    
    Nexus::Logger::Info("Starting Unity project import...");
    Nexus::Logger::Info("Source: " + unityProjectPath);
    Nexus::Logger::Info("Target: " + outputPath);
    
    try {
        Nexus::UnityImporter importer;
        
        if (!std::filesystem::exists(unityProjectPath)) {
            Nexus::Logger::Error("Unity project path does not exist: " + unityProjectPath);
            return 1;
        }
        
        // Validate Unity project
        if (!importer.ValidateProject(unityProjectPath)) {
            Nexus::Logger::Error("Invalid Unity project structure");
            return 1;
        }
        
        // Import the project
        Nexus::ProjectImportSettings settings;
        settings.sourceEngine = Nexus::SourceEngine::Unity;
        settings.sourcePath = unityProjectPath;
        settings.outputPath = outputPath;
        settings.preserveStructure = true;
        settings.convertScripts = true;
        settings.convertAssets = true;
        settings.convertScenes = true;
        
        bool success = importer.ImportProject(settings);
        
        if (success) {
            std::cout << std::endl;
            std::cout << "✅ Unity project imported successfully!" << std::endl;
            std::cout << "📁 Output location: " << outputPath << std::endl;
            std::cout << "🚀 You can now open the project in Nexus Engine" << std::endl;
            return 0;
        } else {
            std::cout << std::endl;
            std::cout << "❌ Failed to import Unity project" << std::endl;
            std::cout << "📄 Check the log for details" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        Nexus::Logger::Error("Exception during import: " + std::string(e.what()));
        std::cout << "❌ Exception occurred: " << e.what() << std::endl;
        return 1;
    }
}