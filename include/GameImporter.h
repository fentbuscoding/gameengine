#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <DirectXMath.h>
#include "Logger.h"

using namespace DirectX;

namespace Nexus {

class Scene;
class Mesh;
class Material;
class Texture;
class Engine;

/**
 * Universal Game Import System
 * Supports importing projects from Unity, Unreal Engine, and Godot
 */
class GameImporter {
public:
    enum class EngineType {
        Unity,
        UnrealEngine,
        Godot,
        Unknown
    };

    enum class AssetType {
        Scene,
        Mesh,
        Material,
        Texture,
        Audio,
        Script,
        Animation,
        Prefab,
        Level,
        Blueprint,
        Shader
    };

    struct ImportSettings {
        bool convertMaterials = true;
        bool convertScripts = true;
        bool convertAnimations = true;
        bool preserveHierarchy = true;
        bool optimizeMeshes = true;
        bool generateLODs = false;
        float scaleMultiplier = 1.0f;
        std::string outputDirectory = "imported_assets/";
        std::string scriptLanguage = "cpp"; // cpp, lua, python
    };

    struct AssetInfo {
        std::string originalPath;
        std::string nexusPath;
        AssetType type;
        std::string name;
        size_t fileSize;
        std::vector<std::string> dependencies;
        std::map<std::string, std::string> metadata;
    };

    struct ImportResult {
        bool success = false;
        std::string message;
        std::vector<AssetInfo> importedAssets;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        float conversionTime = 0.0f;
    };

public:
    GameImporter();
    ~GameImporter();

    // Project Detection
    EngineType DetectEngineType(const std::string& projectPath);
    bool ValidateProjectStructure(const std::string& projectPath, EngineType engineType);

    // Main Import Functions
    ImportResult ImportProject(const std::string& projectPath, const ImportSettings& settings = ImportSettings{});
    ImportResult ImportAsset(const std::string& assetPath, const ImportSettings& settings = ImportSettings{});
    ImportResult ImportScene(const std::string& scenePath, EngineType engineType, const ImportSettings& settings = ImportSettings{});

    // Engine-Specific Importers
    ImportResult ImportUnityProject(const std::string& projectPath, const ImportSettings& settings);
    ImportResult ImportUnrealProject(const std::string& projectPath, const ImportSettings& settings);
    ImportResult ImportGodotProject(const std::string& projectPath, const ImportSettings& settings);

    // Asset Conversion
    bool ConvertUnityScene(const std::string& sceneFile, const std::string& outputPath, const ImportSettings& settings);
    bool ConvertUnrealLevel(const std::string& levelFile, const std::string& outputPath, const ImportSettings& settings);
    bool ConvertGodotScene(const std::string& sceneFile, const std::string& outputPath, const ImportSettings& settings);

    // Material Conversion
    bool ConvertUnityMaterial(const std::string& materialFile, const std::string& outputPath);
    bool ConvertUnrealMaterial(const std::string& materialFile, const std::string& outputPath);
    bool ConvertGodotMaterial(const std::string& materialFile, const std::string& outputPath);

    // Script Conversion
    bool ConvertUnityScript(const std::string& scriptFile, const std::string& outputPath, const ImportSettings& settings);
    bool ConvertUnrealBlueprint(const std::string& blueprintFile, const std::string& outputPath, const ImportSettings& settings);
    bool ConvertGodotScript(const std::string& scriptFile, const std::string& outputPath, const ImportSettings& settings);

    // Utility Functions
    std::vector<std::string> ScanForAssets(const std::string& directory, EngineType engineType);
    std::string GetNexusAssetPath(const std::string& originalPath, AssetType type);
    void SetEngine(Engine* engine) { engine_ = engine; }

private:
    // Unity Import Helpers
    bool ParseUnityScene(const std::string& sceneFile);
    bool ParseUnityPrefab(const std::string& prefabFile);
    bool ParseUnityMaterial(const std::string& materialFile);
    bool ConvertUnityTransform(const std::map<std::string, std::string>& unityTransform, XMFLOAT3& position, XMFLOAT3& rotation, XMFLOAT3& scale);

    // Unreal Import Helpers
    bool ParseUnrealLevel(const std::string& levelFile);
    bool ParseUnrealBlueprint(const std::string& blueprintFile);
    bool ParseUnrealMaterial(const std::string& materialFile);
    bool ConvertUnrealTransform(const std::map<std::string, std::string>& unrealTransform, XMFLOAT3& position, XMFLOAT3& rotation, XMFLOAT3& scale);

    // Godot Import Helpers
    bool ParseGodotScene(const std::string& sceneFile);
    bool ParseGodotResource(const std::string& resourceFile);
    bool ParseGodotScript(const std::string& scriptFile);
    bool ConvertGodotTransform(const std::map<std::string, std::string>& godotTransform, XMFLOAT3& position, XMFLOAT3& rotation, XMFLOAT3& scale);

    // Asset Processing
    bool ProcessMeshAsset(const std::string& meshFile, AssetType sourceType);
    bool ProcessTextureAsset(const std::string& textureFile, AssetType sourceType);
    bool ProcessAudioAsset(const std::string& audioFile, AssetType sourceType);
    bool ProcessAnimationAsset(const std::string& animationFile, AssetType sourceType);

    // File System Helpers
    bool CopyAssetFile(const std::string& sourcePath, const std::string& destinationPath);
    bool CreateDirectoryStructure(const std::string& path);
    std::string GetFileExtension(const std::string& filename);
    std::string GetBaseName(const std::string& path);
    
    // JSON/YAML Parsing
    std::map<std::string, std::string> ParseKeyValueFile(const std::string& content);
    std::string ConvertYAMLToJSON(const std::string& yamlContent);

private:
    Engine* engine_;
    ImportSettings currentSettings_;
    std::vector<AssetInfo> importedAssets_;
    std::vector<std::string> importErrors_;
    std::vector<std::string> importWarnings_;
    std::map<std::string, std::string> assetMapping_; // original -> nexus path mapping
};

/**
 * Unity-specific asset parsers and converters
 */
class UnityImporter {
public:
    struct UnityAsset {
        std::string guid;
        std::string fileID;
        std::string type;
        std::map<std::string, std::string> properties;
    };

    struct UnityGameObject {
        std::string name;
        XMFLOAT3 position;
        XMFLOAT3 rotation;
        XMFLOAT3 scale;
        std::vector<std::string> components;
        std::vector<std::shared_ptr<UnityGameObject>> children;
    };

    static bool ParseMetaFile(const std::string& metaFile, UnityAsset& asset);
    static bool ParseSceneFile(const std::string& sceneFile, std::vector<UnityGameObject>& gameObjects);
    static bool ParsePrefabFile(const std::string& prefabFile, UnityGameObject& prefab);
    static std::string ConvertCSharpToLua(const std::string& csharpCode);
    static std::string ConvertCSharpToCpp(const std::string& csharpCode);
};

/**
 * Unreal Engine-specific asset parsers and converters
 */
class UnrealImporter {
public:
    struct UnrealActor {
        std::string name;
        std::string className;
        XMFLOAT3 location;
        XMFLOAT3 rotation;
        XMFLOAT3 scale;
        std::map<std::string, std::string> properties;
        std::vector<std::shared_ptr<UnrealActor>> children;
    };

    struct UnrealBlueprint {
        std::string name;
        std::string parentClass;
        std::vector<std::string> components;
        std::vector<std::string> functions;
        std::map<std::string, std::string> variables;
    };

    static bool ParseLevelFile(const std::string& levelFile, std::vector<UnrealActor>& actors);
    static bool ParseBlueprintFile(const std::string& blueprintFile, UnrealBlueprint& blueprint);
    static bool ParseMaterialFile(const std::string& materialFile, std::map<std::string, std::string>& materialData);
    static std::string ConvertBlueprintToLua(const UnrealBlueprint& blueprint);
    static std::string ConvertBlueprintToCpp(const UnrealBlueprint& blueprint);
};

/**
 * Godot-specific asset parsers and converters
 */
class GodotImporter {
public:
    struct GodotNode {
        std::string name;
        std::string type;
        XMFLOAT3 position;
        XMFLOAT3 rotation;
        XMFLOAT3 scale;
        std::map<std::string, std::string> properties;
        std::vector<std::shared_ptr<GodotNode>> children;
    };

    struct GodotResource {
        std::string type;
        std::map<std::string, std::string> properties;
    };

    static bool ParseSceneFile(const std::string& sceneFile, std::vector<GodotNode>& nodes);
    static bool ParseResourceFile(const std::string& resourceFile, GodotResource& resource);
    static bool ParseGDScriptFile(const std::string& scriptFile, std::string& code);
    static std::string ConvertGDScriptToLua(const std::string& gdscriptCode);
    static std::string ConvertGDScriptToCpp(const std::string& gdscriptCode);
};

} // namespace Nexus