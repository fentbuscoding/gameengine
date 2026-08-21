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
        SourceEngine,      // Valve Source Engine (Half-Life 2, Portal, CS:GO, etc.)
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
    // ImportSettings is a nested type whose default member initializers are not
    // complete until GameImporter itself is complete, so `= ImportSettings{}`
    // as a default argument is ill-formed (MSVC accepts it; GCC and Clang do
    // not). Overloads give callers the same convenience portably.
    ImportResult ImportProject(const std::string& projectPath, const ImportSettings& settings);
    ImportResult ImportProject(const std::string& projectPath);

    // Declared but not yet implemented - calling either is a link error.
    // Left in place because they describe the intended API surface; see
    // src/utils/GameImporter.cpp.
    ImportResult ImportAsset(const std::string& assetPath, const ImportSettings& settings);
    ImportResult ImportScene(const std::string& scenePath, EngineType engineType, const ImportSettings& settings);

    // Engine-Specific Importers
    ImportResult ImportUnityProject(const std::string& projectPath, const ImportSettings& settings);
    ImportResult ImportUnrealProject(const std::string& projectPath, const ImportSettings& settings);
    ImportResult ImportGodotProject(const std::string& projectPath, const ImportSettings& settings);
    ImportResult ImportSourceEngineGame(const std::string& gamePath, const ImportSettings& settings);

    // Asset Conversion
    bool ConvertUnityScene(const std::string& sceneFile, const std::string& outputPath, const ImportSettings& settings);
    bool ConvertUnrealLevel(const std::string& levelFile, const std::string& outputPath, const ImportSettings& settings);
    bool ConvertGodotScene(const std::string& sceneFile, const std::string& outputPath, const ImportSettings& settings);
    bool ConvertSourceEngineMap(const std::string& mapFile, const std::string& outputPath, const ImportSettings& settings);

    // Material Conversion
    bool ConvertUnityMaterial(const std::string& materialFile, const std::string& outputPath);
    bool ConvertUnrealMaterial(const std::string& materialFile, const std::string& outputPath);
    bool ConvertGodotMaterial(const std::string& materialFile, const std::string& outputPath);
    bool ConvertSourceEngineMaterial(const std::string& vmtFile, const std::string& outputPath);

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

    /// Writes import_manifest.json into the output directory: what was
    /// converted, what was skipped and why. Without it an import leaves no
    /// record of which source asset produced which output, which is the first
    /// thing anyone needs when a conversion looks wrong.
    void WriteImportManifest(const std::string& projectPath,
                             EngineType engineType,
                             const ImportResult& result);

    /// Records that @p assetPath could not be converted, with @p reason. Callers
    /// use this instead of quietly returning true, which is what the placeholder
    /// converters used to do.
    void RecordUnconverted(const std::string& assetPath, const std::string& reason);

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

private:
    static std::shared_ptr<UnityGameObject> ParseUnityGameObject(const std::string& yamlDoc);
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
 * Valve Source Engine asset parsers.
 *
 * EngineType::SourceEngine has been in the enum since the importer was written,
 * but nothing detected it, ImportSourceEngineGame was declared and never
 * defined, and the VMF reader in GameImporter used a regex that truncated every
 * nested block. These parse the two text formats Source ships that can be
 * converted without reverse-engineering a binary layout: VMF (Hammer's map
 * source) and VMT (material definitions). Compiled BSP maps and VTF textures
 * are binary and are reported as unconverted rather than silently skipped.
 */
class SourceEngineImporter {
public:
    struct MapEntity {
        std::string className;
        std::string targetName;
        std::string model;
        XMFLOAT3 origin;
        XMFLOAT3 angles;
        std::map<std::string, std::string> properties;

        MapEntity() : origin{0.0f, 0.0f, 0.0f}, angles{0.0f, 0.0f, 0.0f} {}
    };

    struct MapSolid {
        /// One material per brush face, in file order.
        std::vector<std::string> faceMaterials;
    };

    struct Map {
        std::vector<MapSolid> worldSolids;
        std::vector<MapEntity> entities;

        /// Distinct materials referenced anywhere in the map, sorted. This is
        /// the list an asset pipeline needs in order to know what to convert.
        std::vector<std::string> materials;
    };

    struct Material {
        std::string shader;
        std::string baseTexture;
        std::string bumpMap;
        std::string surfaceProperty;
        bool translucent = false;
        std::map<std::string, std::string> parameters;
    };

    /// Parses VMF text. Split from the file-reading overload so it can be
    /// tested without touching the filesystem.
    static bool ParseMap(const std::string& vmfText, Map& outMap);
    static bool ParseMapFile(const std::string& vmfPath, Map& outMap);

    static bool ParseMaterial(const std::string& vmtText, Material& outMaterial);
    static bool ParseMaterialFile(const std::string& vmtPath, Material& outMaterial);

    /// True when @p projectPath looks like an extracted Source game directory:
    /// a gameinfo.txt, or the maps/ + materials/ pair every mod has.
    static bool LooksLikeSourceGame(const std::string& projectPath);
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