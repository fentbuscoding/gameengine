#include "GameImporter.h"
#include "GodotImporter.h"
#include "Logger.h"
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::cout << "=== NEXUS ENGINE - GODOT PROJECT IMPORTER ===" << std::endl;
    
    if (argc < 3) {
        std::cout << "Usage: NexusGodotImporter <godot_project_path> <output_path>" << std::endl;
        std::cout << "Example: NexusGodotImporter \"C:/GodotProjects/MyGame\" \"C:/NexusProjects/MyGame\"" << std::endl;
        return 1;
    }
    
    std::string godotProjectPath = argv[1];
    std::string outputPath = argv[2];
    
    Nexus::Logger::Info("Starting Godot project import...");
    Nexus::Logger::Info("Source: " + godotProjectPath);
    Nexus::Logger::Info("Target: " + outputPath);
    
    try {
        Nexus::GodotImporter importer;
        
        if (!std::filesystem::exists(godotProjectPath)) {
            Nexus::Logger::Error("Godot project path does not exist: " + godotProjectPath);
            return 1;
        }
        
        // Validate Godot project
        if (!importer.ValidateProject(godotProjectPath)) {
            Nexus::Logger::Error("Invalid Godot project structure");
            return 1;
        }
        
        // Import the project
        Nexus::ProjectImportSettings settings;
        settings.sourceEngine = Nexus::SourceEngine::Godot;
        settings.sourcePath = godotProjectPath;
        settings.outputPath = outputPath;
        settings.preserveStructure = true;
        settings.convertScripts = true;
        settings.convertAssets = true;
        settings.convertScenes = true;
        
        bool success = importer.ImportProject(settings);
        
        if (success) {
            std::cout << std::endl;
            std::cout << "✅ Godot project imported successfully!" << std::endl;
            std::cout << "📁 Output location: " << outputPath << std::endl;
            std::cout << "🚀 You can now open the project in Nexus Engine" << std::endl;
            return 0;
        } else {
            std::cout << std::endl;
            std::cout << "❌ Failed to import Godot project" << std::endl;
            std::cout << "📄 Check the log for details" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        Nexus::Logger::Error("Exception during import: " + std::string(e.what()));
        std::cout << "❌ Exception occurred: " << e.what() << std::endl;
        return 1;
    }
}