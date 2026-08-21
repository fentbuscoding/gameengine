/**
 * @file SourceEngineImporter.cpp
 * @brief Valve Source Engine (VMF / VMT) parsing and conversion.
 */

#include "GameImporter.h"
#include "JsonWriter.h"
#include "Logger.h"
#include "ValveKeyValues.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace Nexus {

namespace {

/// Reads a whole file. Returns false rather than an empty string on failure, so
/// "could not open" and "file is empty" stay distinguishable.
bool ReadTextFile(const std::string& path, std::string& outText) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    outText = buffer.str();
    return true;
}

bool WriteTextFile(const std::string& path, const std::string& text) {
    std::error_code ec;
    // The output directory may not exist yet - the importer creates a fixed set
    // of subdirectories up front, but converted assets can be nested deeper.
    // Writing without this is how ConvertUnityScript silently failed.
    fs::create_directories(fs::path(path).parent_path(), ec);

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        Logger::Error("Could not write " + path);
        return false;
    }

    file << text;
    return static_cast<bool>(file);
}

/// Source material references are stored without an extension and with
/// inconsistent case and slashes ("TOOLS/TOOLSNODRAW", "concrete\floor01").
/// Normalising them is what lets the material list deduplicate correctly.
std::string NormaliseMaterialName(const std::string& raw) {
    std::string name = raw;
    std::replace(name.begin(), name.end(), '\\', '/');
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    while (!name.empty() && (name.front() == '/' || name.front() == ' ')) {
        name.erase(name.begin());
    }
    while (!name.empty() && (name.back() == '/' || name.back() == ' ')) {
        name.pop_back();
    }

    // Collapse repeated separators. Shipped content mixes "models\props\" and
    // "models/props/", and some tools emit both together; without this the same
    // material appears twice in the deduplicated list.
    std::string collapsed;
    collapsed.reserve(name.size());
    for (const char c : name) {
        if (c == '/' && !collapsed.empty() && collapsed.back() == '/') {
            continue;
        }
        collapsed += c;
    }
    return collapsed;
}

void CollectSolids(const std::string& body,
                   std::vector<SourceEngineImporter::MapSolid>& outSolids,
                   std::vector<std::string>& outMaterials) {
    for (const ValveBlock& solidBlock : ExtractNamedBlocks(body)) {
        if (solidBlock.name != "solid") {
            continue;
        }

        SourceEngineImporter::MapSolid solid;
        for (const ValveBlock& sideBlock : ExtractNamedBlocks(solidBlock.body)) {
            if (sideBlock.name != "side") {
                continue;
            }

            std::string material;
            if (FindValveValue(ParseKeyValueBody(sideBlock.body), "material", material)) {
                material = NormaliseMaterialName(material);
                if (!material.empty()) {
                    solid.faceMaterials.push_back(material);
                    outMaterials.push_back(material);
                }
            }
        }

        outSolids.push_back(std::move(solid));
    }
}

} // namespace

bool SourceEngineImporter::ParseMap(const std::string& vmfText, Map& outMap) {
    outMap = Map{};

    const std::vector<ValveBlock> topLevel = ExtractNamedBlocks(vmfText);
    if (topLevel.empty()) {
        return false;
    }

    std::vector<std::string> materials;
    bool sawKnownBlock = false;

    for (const ValveBlock& block : topLevel) {
        if (block.name == "world") {
            sawKnownBlock = true;
            CollectSolids(block.body, outMap.worldSolids, materials);
        } else if (block.name == "entity") {
            sawKnownBlock = true;

            const std::vector<ValveKeyValue> pairs = ParseKeyValueBody(block.body);

            MapEntity entity;
            FindValveValue(pairs, "classname", entity.className);
            FindValveValue(pairs, "targetname", entity.targetName);
            FindValveValue(pairs, "model", entity.model);

            std::string origin;
            if (FindValveValue(pairs, "origin", origin)) {
                ParseValveVector3(origin, entity.origin.x, entity.origin.y, entity.origin.z);
            }

            std::string angles;
            if (FindValveValue(pairs, "angles", angles)) {
                ParseValveVector3(angles, entity.angles.x, entity.angles.y, entity.angles.z);
            }

            for (const ValveKeyValue& pair : pairs) {
                entity.properties[pair.key] = pair.value;
            }

            // Brush entities (func_detail, trigger volumes, doors) carry their
            // own solids, and their materials belong in the map's material list
            // just as much as the world's.
            std::vector<MapSolid> entitySolids;
            CollectSolids(block.body, entitySolids, materials);

            outMap.entities.push_back(std::move(entity));
        }
    }

    if (!sawKnownBlock) {
        // versioninfo/viewsettings alone is not a map; treating it as one would
        // report a successful import of nothing.
        return false;
    }

    std::sort(materials.begin(), materials.end());
    materials.erase(std::unique(materials.begin(), materials.end()), materials.end());
    outMap.materials = std::move(materials);

    return true;
}

bool SourceEngineImporter::ParseMapFile(const std::string& vmfPath, Map& outMap) {
    std::string text;
    if (!ReadTextFile(vmfPath, text)) {
        Logger::Error("Could not open VMF: " + vmfPath);
        return false;
    }
    if (!ParseMap(text, outMap)) {
        Logger::Error("Not a readable VMF (no world or entity blocks): " + vmfPath);
        return false;
    }
    return true;
}

bool SourceEngineImporter::ParseMaterial(const std::string& vmtText, Material& outMaterial) {
    outMaterial = Material{};

    const std::vector<ValveBlock> blocks = ExtractNamedBlocks(vmtText);
    if (blocks.empty()) {
        return false;
    }

    // A VMT is a single block whose *name* is the shader ("LightmappedGeneric",
    // "VertexLitGeneric", "UnlitGeneric", ...). Patch materials use "patch" with
    // an `include` pointing at the real one.
    const ValveBlock& root = blocks.front();
    outMaterial.shader = root.name;

    const std::vector<ValveKeyValue> pairs = ParseKeyValueBody(root.body);
    for (const ValveKeyValue& pair : pairs) {
        outMaterial.parameters[pair.key] = pair.value;
    }

    std::string value;
    if (FindValveValue(pairs, "$basetexture", value)) {
        outMaterial.baseTexture = NormaliseMaterialName(value);
    }
    if (FindValveValue(pairs, "$bumpmap", value)) {
        outMaterial.bumpMap = NormaliseMaterialName(value);
    }
    if (FindValveValue(pairs, "$surfaceprop", value)) {
        outMaterial.surfaceProperty = value;
    }
    if (FindValveValue(pairs, "$translucent", value)) {
        outMaterial.translucent = (value != "0");
    } else if (FindValveValue(pairs, "$alphatest", value)) {
        outMaterial.translucent = (value != "0");
    }

    // A patch material's parameters live in a nested "replace" or "insert"
    // block, so a flat read finds nothing. Merge them over the base.
    if (outMaterial.shader == "patch" || outMaterial.shader == "Patch") {
        for (const ValveBlock& nested : ExtractNamedBlocks(root.body)) {
            for (const ValveKeyValue& pair : ParseKeyValueBody(nested.body)) {
                outMaterial.parameters[pair.key] = pair.value;
            }
        }

        const auto patched = outMaterial.parameters.find("$basetexture");
        if (patched != outMaterial.parameters.end() && outMaterial.baseTexture.empty()) {
            outMaterial.baseTexture = NormaliseMaterialName(patched->second);
        }
    }

    return true;
}

bool SourceEngineImporter::ParseMaterialFile(const std::string& vmtPath, Material& outMaterial) {
    std::string text;
    if (!ReadTextFile(vmtPath, text)) {
        Logger::Error("Could not open VMT: " + vmtPath);
        return false;
    }
    if (!ParseMaterial(text, outMaterial)) {
        Logger::Error("Not a readable VMT: " + vmtPath);
        return false;
    }
    return true;
}

bool SourceEngineImporter::LooksLikeSourceGame(const std::string& projectPath) {
    std::error_code ec;

    if (fs::is_regular_file(fs::path(projectPath) / "gameinfo.txt", ec)) {
        return true;
    }

    // Extracted content without a gameinfo.txt still has the standard layout.
    // Requiring both directories keeps a stray "materials" folder in an
    // unrelated project from matching.
    const bool hasMaps = fs::is_directory(fs::path(projectPath) / "maps", ec);
    const bool hasMaterials = fs::is_directory(fs::path(projectPath) / "materials", ec);
    return hasMaps && hasMaterials;
}

// --- GameImporter entry points ----------------------------------------------

bool GameImporter::ConvertSourceEngineMap(const std::string& mapFile,
                                          const std::string& outputPath,
                                          const ImportSettings& settings) {
    SourceEngineImporter::Map map;
    if (!SourceEngineImporter::ParseMapFile(mapFile, map)) {
        return false;
    }

    size_t faceCount = 0;
    for (const SourceEngineImporter::MapSolid& solid : map.worldSolids) {
        faceCount += solid.faceMaterials.size();
    }

    JsonWriter json;
    json.BeginObject()
        .Value("format", "nexus.scene")
        .Value("version", 1)
        .Value("source", mapFile)
        .Value("sourceEngine", "valve-source")
        .Value("brushCount", map.worldSolids.size())
        .Value("faceCount", faceCount);

    json.BeginArray("materials");
    for (const std::string& material : map.materials) {
        json.Element(material);
    }
    json.EndArray();

    json.BeginArray("entities");
    for (const SourceEngineImporter::MapEntity& entity : map.entities) {
        json.BeginObject("")
            .Value("class", entity.className);

        if (!entity.targetName.empty()) {
            json.Value("name", entity.targetName);
        }
        if (!entity.model.empty()) {
            json.Value("model", entity.model);
        }

        // Source is Z-up with 1 unit = 1 inch; the scale factor the caller asked
        // for is applied here so downstream consumers see engine units.
        json.Vector3("position",
                     entity.origin.x * settings.scaleMultiplier,
                     entity.origin.y * settings.scaleMultiplier,
                     entity.origin.z * settings.scaleMultiplier);
        json.Vector3("rotation", entity.angles.x, entity.angles.y, entity.angles.z);
        json.EndObject();
    }
    json.EndArray();

    json.EndObject();

    if (!WriteTextFile(outputPath, json.ToString())) {
        return false;
    }

    Logger::Info("Converted Source map " + mapFile + ": " +
                 std::to_string(map.worldSolids.size()) + " brushes, " +
                 std::to_string(map.entities.size()) + " entities, " +
                 std::to_string(map.materials.size()) + " materials");
    return true;
}

bool GameImporter::ConvertSourceEngineMaterial(const std::string& vmtFile,
                                               const std::string& outputPath) {
    SourceEngineImporter::Material material;
    if (!SourceEngineImporter::ParseMaterialFile(vmtFile, material)) {
        return false;
    }

    JsonWriter json;
    json.BeginObject()
        .Value("format", "nexus.material")
        .Value("version", 1)
        .Value("source", vmtFile)
        .Value("shader", material.shader)
        .Value("baseTexture", material.baseTexture)
        .Value("normalMap", material.bumpMap)
        .Value("surfaceProperty", material.surfaceProperty)
        .Value("translucent", material.translucent);

    json.BeginObject("parameters");
    for (const auto& parameter : material.parameters) {
        json.Value(parameter.first, parameter.second);
    }
    json.EndObject();

    json.EndObject();

    return WriteTextFile(outputPath, json.ToString());
}

} // namespace Nexus
