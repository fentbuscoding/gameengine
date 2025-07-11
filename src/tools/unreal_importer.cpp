#include "GameImporter.h"
#include "UnrealImporter.h"
#include "Logger.h"
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::cout << "=== NEXUS ENGINE - UNREAL PROJECT IMPORTER ===" << std::endl;
    
    if (argc < 3) {
        std::cout << "Usage: NexusUnrealImporter <unreal_project_path> <output_path>" << std::endl;
        std::cout << "Example: NexusUnrealImporter \"C:/UnrealProjects/MyGame\" \"C:/NexusProjects/MyGame\"" << std::endl;
        return 1;
    }
    
    std::string unrealProjectPath = argv[1];
    std::string outputPath = argv[2];
    
    Nexus::Logger::Info("Starting Unreal project import...");
    Nexus::Logger::Info("Source: " + unrealProjectPath);
    Nexus::Logger::Info("Target: " + outputPath);
    
    try {
        Nexus::UnrealImporter importer;
        
        if (!std::filesystem::exists(unrealProjectPath)) {
            Nexus::Logger::Error("Unreal project path does not exist: " + unrealProjectPath);
            return 1;
        }
        
        // Validate Unreal project
        if (!importer.ValidateProject(unrealProjectPath)) {
            Nexus::Logger::Error("Invalid Unreal project structure");
            return 1;
        }
        
        // Import the project
        Nexus::ProjectImportSettings settings;
        settings.sourceEngine = Nexus::SourceEngine::Unreal;
        settings.sourcePath = unrealProjectPath;
        settings.outputPath = outputPath;
        settings.preserveStructure = true;
        settings.convertScripts = true;
        settings.convertAssets = true;
        settings.convertScenes = true;
        settings.convertBlueprints = true;
        settings.convertMaterials = true;
        
        bool success = importer.ImportProject(settings);
        
        if (success) {
            std::cout << std::endl;
            std::cout << "✅ Unreal project imported successfully!" << std::endl;
            std::cout << "📁 Output location: " << outputPath << std::endl;
            std::cout << "🚀 You can now open the project in Nexus Engine" << std::endl;
            return 0;
        } else {
            std::cout << std::endl;
            std::cout << "❌ Failed to import Unreal project" << std::endl;
            std::cout << "📄 Check the log for details" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        Nexus::Logger::Error("Exception during import: " + std::string(e.what()));
        std::cout << "❌ Exception occurred: " << e.what() << std::endl;
        return 1;
    }
}