#include "GameImporter.h"
#include "Engine.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <chrono>

namespace fs = std::filesystem;

namespace Nexus {

GameImporter::GameImporter() : engine_(nullptr) {
}

GameImporter::~GameImporter() {
}

GameImporter::EngineType GameImporter::DetectEngineType(const std::string& projectPath) {
    if (!fs::exists(projectPath)) {
        Logger::Error("Project path does not exist: " + projectPath);
        return EngineType::Unknown;
    }

    // Check for Unity project
    if (fs::exists(projectPath + "/ProjectSettings/ProjectVersion.txt") ||
        fs::exists(projectPath + "/Assets") ||
        fs::exists(projectPath + "/Library")) {
        Logger::Info("Detected Unity project");
        return EngineType::Unity;
    }

    // Check for Unreal Engine project
    if (fs::exists(projectPath + "/Config/DefaultEngine.ini") ||
        fs::exists(projectPath + "/Source") ||
        fs::exists(projectPath + "/Content")) {
        for (const auto& entry : fs::directory_iterator(projectPath)) {
            if (entry.path().extension() == ".uproject") {
                Logger::Info("Detected Unreal Engine project");
                return EngineType::UnrealEngine;
            }
        }
    }

    // Check for Godot project
    if (fs::exists(projectPath + "/project.godot") ||
        fs::exists(projectPath + "/default_env.tres") ||
        fs::exists(projectPath + "/export_presets.cfg")) {
        Logger::Info("Detected Godot project");
        return EngineType::Godot;
    }

    Logger::Warning("Could not detect engine type for project: " + projectPath);
    return EngineType::Unknown;
}

bool GameImporter::ValidateProjectStructure(const std::string& projectPath, EngineType engineType) {
    switch (engineType) {
        case EngineType::Unity:
            return fs::exists(projectPath + "/Assets") && 
                   fs::exists(projectPath + "/ProjectSettings");
        
        case EngineType::UnrealEngine:
            return fs::exists(projectPath + "/Content") && 
                   fs::exists(projectPath + "/Config");
        
        case EngineType::Godot:
            return fs::exists(projectPath + "/project.godot");
        
        default:
            return false;
    }
}

GameImporter::ImportResult GameImporter::ImportProject(const std::string& projectPath, const ImportSettings& settings) {
    auto startTime = std::chrono::high_resolution_clock::now();
    ImportResult result;
    
    Logger::Info("Starting project import from: " + projectPath);
    
    // Clear previous import data
    importedAssets_.clear();
    importErrors_.clear();
    importWarnings_.clear();
    currentSettings_ = settings;

    // Detect engine type
    EngineType engineType = DetectEngineType(projectPath);
    if (engineType == EngineType::Unknown) {
        result.success = false;
        result.message = "Could not detect project engine type";
        return result;
    }

    // Validate project structure
    if (!ValidateProjectStructure(projectPath, engineType)) {
        result.success = false;
        result.message = "Invalid project structure detected";
        return result;
    }

    // Create output directory
    if (!CreateDirectoryStructure(settings.outputDirectory)) {
        result.success = false;
        result.message = "Failed to create output directory: " + settings.outputDirectory;
        return result;
    }

    // Import based on engine type
    switch (engineType) {
        case EngineType::Unity:
            result = ImportUnityProject(projectPath, settings);
            break;
        case EngineType::UnrealEngine:
            result = ImportUnrealProject(projectPath, settings);
            break;
        case EngineType::Godot:
            result = ImportGodotProject(projectPath, settings);
            break;
        default:
            result.success = false;
            result.message = "Unsupported engine type";
            break;
    }

    // Calculate conversion time
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    result.conversionTime = duration.count() / 1000.0f;

    // Copy results
    result.importedAssets = importedAssets_;
    result.errors = importErrors_;
    result.warnings = importWarnings_;

    Logger::Info("Project import completed in " + std::to_string(result.conversionTime) + " seconds");
    Logger::Info("Imported " + std::to_string(result.importedAssets.size()) + " assets");
    
    if (!result.errors.empty()) {
        Logger::Warning("Import completed with " + std::to_string(result.errors.size()) + " errors");
    }

    return result;
}

GameImporter::ImportResult GameImporter::ImportUnityProject(const std::string& projectPath, const ImportSettings& settings) {
    ImportResult result;
    result.success = true;
    result.message = "Unity project import started";

    Logger::Info("Importing Unity project from: " + projectPath);

    // Scan for Unity assets
    std::vector<std::string> assetPaths = ScanForAssets(projectPath + "/Assets", EngineType::Unity);
    
    Logger::Info("Found " + std::to_string(assetPaths.size()) + " Unity assets to import");

    for (const std::string& assetPath : assetPaths) {
        std::string extension = GetFileExtension(assetPath);
        
        try {
            if (extension == ".unity") {
                // Unity scene file
                std::string outputPath = GetNexusAssetPath(assetPath, AssetType::Scene);
                if (ConvertUnityScene(assetPath, outputPath, settings)) {
                    AssetInfo info;
                    info.originalPath = assetPath;
                    info.nexusPath = outputPath;
                    info.type = AssetType::Scene;
                    info.name = GetBaseName(assetPath);
                    importedAssets_.push_back(info);
                    Logger::Info("Imported Unity scene: " + info.name);
                }
            }
            else if (extension == ".prefab") {
                // Unity prefab
                std::string outputPath = GetNexusAssetPath(assetPath, AssetType::Prefab);
                if (ParseUnityPrefab(assetPath)) {
                    AssetInfo info;
                    info.originalPath = assetPath;
                    info.nexusPath = outputPath;
                    info.type = AssetType::Prefab;
                    info.name = GetBaseName(assetPath);
                    importedAssets_.push_back(info);
                    Logger::Info("Imported Unity prefab: " + info.name);
                }
            }
            else if (extension == ".mat") {
                // Unity material
                std::string outputPath = GetNexusAssetPath(assetPath, AssetType::Material);
                if (ConvertUnityMaterial(assetPath, outputPath)) {
                    AssetInfo info;
                    info.originalPath = assetPath;
                    info.nexusPath = outputPath;
                    info.type = AssetType::Material;
                    info.name = GetBaseName(assetPath);
                    importedAssets_.push_back(info);
                    Logger::Info("Imported Unity material: " + info.name);
                }
            }
            else if (extension == ".cs") {
                // Unity C# script
                if (settings.convertScripts) {
                    std::string outputPath = GetNexusAssetPath(assetPath, AssetType::Script);
                    if (ConvertUnityScript(assetPath, outputPath, settings)) {
                        AssetInfo info;
                        info.originalPath = assetPath;
                        info.nexusPath = outputPath;
                        info.type = AssetType::Script;
                        info.name = GetBaseName(assetPath);
                        importedAssets_.push_back(info);
                        Logger::Info("Converted Unity script: " + info.name);
                    }
                }
            }
            else if (extension == ".fbx" || extension == ".obj" || extension == ".dae") {
                // 3D models
                std::string outputPath = GetNexusAssetPath(assetPath, AssetType::Mesh);
                if (ProcessMeshAsset(assetPath, AssetType::Mesh)) {
                    CopyAssetFile(assetPath, outputPath);
                    AssetInfo info;
                    info.originalPath = assetPath;
                    info.nexusPath = outputPath;
                    info.type = AssetType::Mesh;
                    info.name = GetBaseName(assetPath);
                    importedAssets_.push_back(info);
                    Logger::Info("Imported mesh: " + info.name);
                }
            }
            else if (extension == ".png" || extension == ".jpg" || extension == ".tga" || extension == ".exr") {
                // Textures
                std::string outputPath = GetNexusAssetPath(assetPath, AssetType::Texture);
                if (ProcessTextureAsset(assetPath, AssetType::Texture)) {
                    CopyAssetFile(assetPath, outputPath);
                    AssetInfo info;
                    info.originalPath = assetPath;
                    info.nexusPath = outputPath;
                    info.type = AssetType::Texture;
                    info.name = GetBaseName(assetPath);
                    importedAssets_.push_back(info);
                    Logger::Info("Imported texture: " + info.name);
                }
            }
            else if (extension == ".wav" || extension == ".mp3" || extension == ".ogg") {
                // Audio files
                std::string outputPath = GetNexusAssetPath(assetPath, AssetType::Audio);
                if (ProcessAudioAsset(assetPath, AssetType::Audio)) {
                    CopyAssetFile(assetPath, outputPath);
                    AssetInfo info;
                    info.originalPath = assetPath;
                    info.nexusPath = outputPath;
                    info.type = AssetType::Audio;
                    info.name = GetBaseName(assetPath);
                    importedAssets_.push_back(info);
                    Logger::Info("Imported audio: " + info.name);
                }
            }
        }
        catch (const std::exception& e) {
            std::string error = "Failed to import asset " + assetPath + ": " + e.what();
            importErrors_.push_back(error);
            Logger::Error(error);
        }
    }

    if (importErrors_.empty()) {
        result.message = "Unity project imported successfully";
        Logger::Info("Unity project import completed successfully");
    } else {
        result.message = "Unity project imported with " + std::to_string(importErrors_.size()) + " errors";
        Logger::Warning(result.message);
    }

    return result;
}

GameImporter::ImportResult GameImporter::ImportUnrealProject(const std::string& projectPath, const ImportSettings& settings) {
    ImportResult result;
    result.success = true;
    result.message = "Unreal Engine project import started";

    Logger::Info("Importing Unreal Engine project from: " + projectPath);

    // Scan for Unreal assets
    std::vector<std::string> assetPaths = ScanForAssets(projectPath + "/Content", EngineType::UnrealEngine);
    
    Logger::Info("Found " + std::to_string(assetPaths.size()) + " Unreal assets to import");

    for (const std::string& assetPath : assetPaths) {
        std::string extension = GetFileExtension(assetPath);
        
        try {
            if (extension == ".umap") {
                // Unreal level file
                std::string outputPath = GetNexusAssetPath(assetPath, AssetType::Level);
                if (ConvertUnrealLevel(assetPath, outputPath, settings)) {
                    AssetInfo info;
                    info.originalPath = assetPath;
                    info.nexusPath = outputPath;
                    info.type = AssetType::Level;
                    info.name = GetBaseName(assetPath);
                    importedAssets_.push_back(info);
                    Logger::Info("Imported Unreal level: " + info.name);
                }
            }
            else if (extension == ".uasset") {
                // Generic Unreal asset - determine type by content
                // This could be materials, blueprints, meshes, etc.
                std::string outputPath = GetNexusAssetPath(assetPath, AssetType::Scene);
                // Add specific parsing logic based on asset content
                AssetInfo info;
                info.originalPath = assetPath;
                info.nexusPath = outputPath;
                info.type = AssetType::Scene;
                info.name = GetBaseName(assetPath);
                importedAssets_.push_back(info);
                Logger::Info("Imported Unreal asset: " + info.name);
            }
            // Handle other Unreal-specific file types...
        }
        catch (const std::exception& e) {
            std::string error = "Failed to import Unreal asset " + assetPath + ": " + e.what();
            importErrors_.push_back(error);
            Logger::Error(error);
        }
    }

    result.message = "Unreal Engine project imported";
    return result;
}

GameImporter::ImportResult GameImporter::ImportGodotProject(const std::string& projectPath, const ImportSettings& settings) {
    ImportResult result;
    result.success = true;
    result.message = "Godot project import started";

    Logger::Info("Importing Godot project from: " + projectPath);

    // Scan for Godot assets
    std::vector<std::string> assetPaths = ScanForAssets(projectPath, EngineType::Godot);
    
    Logger::Info("Found " + std::to_string(assetPaths.size()) + " Godot assets to import");

    for (const std::string& assetPath : assetPaths) {
        std::string extension = GetFileExtension(assetPath);
        
        try {
            if (extension == ".tscn") {
                // Godot scene file
                std::string outputPath = GetNexusAssetPath(assetPath, AssetType::Scene);
                if (ConvertGodotScene(assetPath, outputPath, settings)) {
                    AssetInfo info;
                    info.originalPath = assetPath;
                    info.nexusPath = outputPath;
                    info.type = AssetType::Scene;
                    info.name = GetBaseName(assetPath);
                    importedAssets_.push_back(info);
                    Logger::Info("Imported Godot scene: " + info.name);
                }
            }
            else if (extension == ".gd") {
                // Godot GDScript
                if (settings.convertScripts) {
                    std::string outputPath = GetNexusAssetPath(assetPath, AssetType::Script);
                    if (ConvertGodotScript(assetPath, outputPath, settings)) {
                        AssetInfo info;
                        info.originalPath = assetPath;
                        info.nexusPath = outputPath;
                        info.type = AssetType::Script;
                        info.name = GetBaseName(assetPath);
                        importedAssets_.push_back(info);
                        Logger::Info("Converted Godot script: " + info.name);
                    }
                }
            }
            else if (extension == ".tres" || extension == ".res") {
                // Godot resource files
                std::string outputPath = GetNexusAssetPath(assetPath, AssetType::Material);
                AssetInfo info;
                info.originalPath = assetPath;
                info.nexusPath = outputPath;
                info.type = AssetType::Material;
                info.name = GetBaseName(assetPath);
                importedAssets_.push_back(info);
                Logger::Info("Imported Godot resource: " + info.name);
            }
            // Handle other Godot-specific file types...
        }
        catch (const std::exception& e) {
            std::string error = "Failed to import Godot asset " + assetPath + ": " + e.what();
            importErrors_.push_back(error);
            Logger::Error(error);
        }
    }

    result.message = "Godot project imported";
    return result;
}

std::vector<std::string> GameImporter::ScanForAssets(const std::string& directory, EngineType engineType) {
    std::vector<std::string> assetPaths;
    
    if (!fs::exists(directory)) {
        Logger::Warning("Asset directory does not exist: " + directory);
        return assetPaths;
    }

    std::vector<std::string> extensions;
    
    // Define extensions based on engine type
    switch (engineType) {
        case EngineType::Unity:
            extensions = {".unity", ".prefab", ".mat", ".cs", ".fbx", ".obj", ".png", ".jpg", ".wav", ".mp3"};
            break;
        case EngineType::UnrealEngine:
            extensions = {".umap", ".uasset", ".cpp", ".h", ".fbx", ".obj", ".png", ".jpg", ".wav", ".mp3"};
            break;
        case EngineType::Godot:
            extensions = {".tscn", ".gd", ".tres", ".res", ".fbx", ".obj", ".png", ".jpg", ".wav", ".ogg"};
            break;
        default:
            extensions = {".fbx", ".obj", ".png", ".jpg", ".wav", ".mp3"};
            break;
    }

    // Recursively scan directory
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string extension = entry.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end()) {
                assetPaths.push_back(entry.path().string());
            }
        }
    }

    return assetPaths;
}

std::string GameImporter::GetNexusAssetPath(const std::string& originalPath, AssetType type) {
    std::string baseName = GetBaseName(originalPath);
    std::string typeDir;
    
    switch (type) {
        case AssetType::Scene: typeDir = "scenes"; break;
        case AssetType::Mesh: typeDir = "meshes"; break;
        case AssetType::Material: typeDir = "materials"; break;
        case AssetType::Texture: typeDir = "textures"; break;
        case AssetType::Audio: typeDir = "audio"; break;
        case AssetType::Script: typeDir = "scripts"; break;
        case AssetType::Animation: typeDir = "animations"; break;
        case AssetType::Prefab: typeDir = "prefabs"; break;
        case AssetType::Level: typeDir = "levels"; break;
        default: typeDir = "misc"; break;
    }
    
    return currentSettings_.outputDirectory + typeDir + "/" + baseName;
}

bool GameImporter::CreateDirectoryStructure(const std::string& path) {
    try {
        fs::create_directories(path);
        
        // Create subdirectories for different asset types
        fs::create_directories(path + "/scenes");
        fs::create_directories(path + "/meshes");
        fs::create_directories(path + "/materials");
        fs::create_directories(path + "/textures");
        fs::create_directories(path + "/audio");
        fs::create_directories(path + "/scripts");
        fs::create_directories(path + "/animations");
        fs::create_directories(path + "/prefabs");
        fs::create_directories(path + "/levels");
        fs::create_directories(path + "/misc");
        
        return true;
    }
    catch (const fs::filesystem_error& e) {
        Logger::Error("Failed to create directory structure: " + std::string(e.what()));
        return false;
    }
}

std::string GameImporter::GetFileExtension(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos) {
        std::string ext = filename.substr(pos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }
    return "";
}

std::string GameImporter::GetBaseName(const std::string& path) {
    fs::path p(path);
    return p.stem().string();
}

bool GameImporter::CopyAssetFile(const std::string& sourcePath, const std::string& destinationPath) {
    try {
        fs::create_directories(fs::path(destinationPath).parent_path());
        fs::copy_file(sourcePath, destinationPath, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const fs::filesystem_error& e) {
        Logger::Error("Failed to copy file from " + sourcePath + " to " + destinationPath + ": " + e.what());
        return false;
    }
}

// Placeholder implementations for conversion functions
bool GameImporter::ConvertUnityScene(const std::string& sceneFile, const std::string& outputPath, const ImportSettings& settings) {
    Logger::Info("Converting Unity scene: " + sceneFile);
    // TODO: Implement Unity scene parsing and conversion
    return true;
}

bool GameImporter::ConvertUnrealLevel(const std::string& levelFile, const std::string& outputPath, const ImportSettings& settings) {
    Logger::Info("Converting Unreal level: " + levelFile);
    // TODO: Implement Unreal level parsing and conversion
    return true;
}

bool GameImporter::ConvertGodotScene(const std::string& sceneFile, const std::string& outputPath, const ImportSettings& settings) {
    Logger::Info("Converting Godot scene: " + sceneFile);
    // TODO: Implement Godot scene parsing and conversion
    return true;
}

bool GameImporter::ConvertUnityMaterial(const std::string& materialFile, const std::string& outputPath) {
    Logger::Info("Converting Unity material: " + materialFile);
    // TODO: Implement Unity material conversion
    return true;
}

bool GameImporter::ConvertUnrealMaterial(const std::string& materialFile, const std::string& outputPath) {
    Logger::Info("Converting Unreal material: " + materialFile);
    // TODO: Implement Unreal material conversion
    return true;
}

bool GameImporter::ConvertGodotMaterial(const std::string& materialFile, const std::string& outputPath) {
    Logger::Info("Converting Godot material: " + materialFile);
    // TODO: Implement Godot material conversion
    return true;
}

bool GameImporter::ConvertUnityScript(const std::string& scriptFile, const std::string& outputPath, const ImportSettings& settings) {
    Logger::Info("Converting Unity script: " + scriptFile);
    
    std::ifstream file(scriptFile);
    if (!file.is_open()) {
        Logger::Error("Failed to open Unity script file: " + scriptFile);
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    std::string convertedCode;
    if (settings.scriptLanguage == "lua") {
        convertedCode = UnityImporter::ConvertCSharpToLua(content);
    } else if (settings.scriptLanguage == "cpp") {
        convertedCode = UnityImporter::ConvertCSharpToCpp(content);
    } else {
        // Default to copying as-is with comments
        convertedCode = "// Converted from Unity C# script\n// Original: " + scriptFile + "\n\n" + content;
    }
    
    std::ofstream outFile(outputPath);
    if (outFile.is_open()) {
        outFile << convertedCode;
        outFile.close();
        return true;
    }
    
    return false;
}

bool GameImporter::ConvertUnrealBlueprint(const std::string& blueprintFile, const std::string& outputPath, const ImportSettings& settings) {
    Logger::Info("Converting Unreal Blueprint: " + blueprintFile);
    // TODO: Implement Blueprint to script conversion
    return true;
}

bool GameImporter::ConvertGodotScript(const std::string& scriptFile, const std::string& outputPath, const ImportSettings& settings) {
    Logger::Info("Converting Godot script: " + scriptFile);
    
    std::ifstream file(scriptFile);
    if (!file.is_open()) {
        Logger::Error("Failed to open Godot script file: " + scriptFile);
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    std::string convertedCode;
    if (settings.scriptLanguage == "lua") {
        convertedCode = GodotImporter::ConvertGDScriptToLua(content);
    } else if (settings.scriptLanguage == "cpp") {
        convertedCode = GodotImporter::ConvertGDScriptToCpp(content);
    } else {
        // Default to copying as-is with comments
        convertedCode = "# Converted from Godot GDScript\n# Original: " + scriptFile + "\n\n" + content;
    }
    
    std::ofstream outFile(outputPath);
    if (outFile.is_open()) {
        outFile << convertedCode;
        outFile.close();
        return true;
    }
    
    return false;
}

bool GameImporter::ProcessMeshAsset(const std::string& meshFile, AssetType sourceType) {
    Logger::Info("Processing mesh asset: " + meshFile);
    // TODO: Add mesh optimization, LOD generation, etc.
    return true;
}

bool GameImporter::ProcessTextureAsset(const std::string& textureFile, AssetType sourceType) {
    Logger::Info("Processing texture asset: " + textureFile);
    // TODO: Add texture compression, format conversion, etc.
    return true;
}

bool GameImporter::ProcessAudioAsset(const std::string& audioFile, AssetType sourceType) {
    Logger::Info("Processing audio asset: " + audioFile);
    // TODO: Add audio format conversion, compression, etc.
    return true;
}

bool GameImporter::ProcessAnimationAsset(const std::string& animationFile, AssetType sourceType) {
    Logger::Info("Processing animation asset: " + animationFile);
    // TODO: Add animation format conversion, optimization, etc.
    return true;
}

bool GameImporter::ParseUnityPrefab(const std::string& prefabFile) {
    Logger::Info("Parsing Unity prefab: " + prefabFile);
    // TODO: Implement Unity prefab parsing
    return true;
}

// Unity Importer Implementation
std::string UnityImporter::ConvertCSharpToLua(const std::string& csharpCode) {
    std::string luaCode = "-- Converted from C# Unity script\n\n";
    
    // Basic conversion patterns
    std::string code = csharpCode;
    
    // Replace common Unity patterns
    code = std::regex_replace(code, std::regex("public class (\\w+) : MonoBehaviour"), "local $1 = {}\n\nfunction $1:new()");
    code = std::regex_replace(code, std::regex("void Start\\(\\)"), "function $1:initialize()");
    code = std::regex_replace(code, std::regex("void Update\\(\\)"), "function $1:update(deltaTime)");
    code = std::regex_replace(code, std::regex("transform\\.position"), "self.position");
    code = std::regex_replace(code, std::regex("GameObject"), "game_object");
    code = std::regex_replace(code, std::regex("Vector3"), "vector3");
    
    luaCode += code;
    luaCode += "\n\nreturn " + "ScriptClass"; // Placeholder for actual class name
    
    return luaCode;
}

std::string UnityImporter::ConvertCSharpToCpp(const std::string& csharpCode) {
    std::string cppCode = "// Converted from C# Unity script\n";
    cppCode += "#include \"Engine.h\"\n#include \"GameObject.h\"\n\n";
    
    std::string code = csharpCode;
    
    // Basic conversion patterns
    code = std::regex_replace(code, std::regex("public class (\\w+) : MonoBehaviour"), "class $1 : public Component");
    code = std::regex_replace(code, std::regex("void Start\\(\\)"), "void Initialize() override");
    code = std::regex_replace(code, std::regex("void Update\\(\\)"), "void Update(float deltaTime) override");
    code = std::regex_replace(code, std::regex("Vector3"), "XMFLOAT3");
    
    cppCode += code;
    
    return cppCode;
}

// Godot Importer Implementation
std::string GodotImporter::ConvertGDScriptToLua(const std::string& gdscriptCode) {
    std::string luaCode = "-- Converted from Godot GDScript\n\n";
    
    std::string code = gdscriptCode;
    
    // Basic conversion patterns
    code = std::regex_replace(code, std::regex("extends (\\w+)"), "-- Extends $1\nlocal ScriptClass = {}");
    code = std::regex_replace(code, std::regex("func _ready\\(\\):"), "function ScriptClass:initialize()");
    code = std::regex_replace(code, std::regex("func _process\\(delta\\):"), "function ScriptClass:update(deltaTime)");
    code = std::regex_replace(code, std::regex("\\$"), "self."); // Node references
    code = std::regex_replace(code, std::regex("Vector2"), "vector2");
    code = std::regex_replace(code, std::regex("Vector3"), "vector3");
    
    luaCode += code;
    luaCode += "\n\nreturn ScriptClass";
    
    return luaCode;
}

std::string GodotImporter::ConvertGDScriptToCpp(const std::string& gdscriptCode) {
    std::string cppCode = "// Converted from Godot GDScript\n";
    cppCode += "#include \"Engine.h\"\n#include \"Node.h\"\n\n";
    
    std::string code = gdscriptCode;
    
    // Basic conversion patterns
    code = std::regex_replace(code, std::regex("extends (\\w+)"), "class ScriptClass : public $1");
    code = std::regex_replace(code, std::regex("func _ready\\(\\):"), "void Initialize() override {");
    code = std::regex_replace(code, std::regex("func _process\\(delta\\):"), "void Update(float deltaTime) override {");
    code = std::regex_replace(code, std::regex("Vector2"), "XMFLOAT2");
    code = std::regex_replace(code, std::regex("Vector3"), "XMFLOAT3");
    
    cppCode += code;
    
    return cppCode;
}

} // namespace Nexus