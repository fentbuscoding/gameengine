// ---------------------------------------------------------------------------
// Game importer tests.
//
// The importer's failure mode has always been the quiet kind: every converter
// logged a line and returned true, so an import that produced nothing reported
// complete success. These tests are written against that - they check what came
// out, not that the call returned, and several assert that a bad input is
// *rejected* rather than accepted with empty results.
// ---------------------------------------------------------------------------

#include "TestFramework.h"

#include "GameImporter.h"
#include "JsonWriter.h"
#include "ValveKeyValues.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace Nexus;

namespace {

/// A directory removed when the test finishes, so a failing run cannot leave
/// state that makes the next one pass.
class TempDir {
public:
    explicit TempDir(const std::string& name) {
        path_ = fs::temp_directory_path() / ("nexus-importer-tests-" + name);

        // Removed first as well as last: a previous run killed mid-test would
        // otherwise leave files behind that make this run's assertions pass for
        // the wrong reason.
        std::error_code ec;
        fs::remove_all(path_, ec);
        fs::create_directories(path_, ec);
    }

    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const fs::path& Path() const { return path_; }
    std::string Str() const { return path_.string(); }

    fs::path Write(const std::string& relative, const std::string& contents) const {
        const fs::path target = path_ / relative;
        std::error_code ec;
        fs::create_directories(target.parent_path(), ec);
        std::ofstream file(target, std::ios::binary | std::ios::trunc);
        file << contents;
        return target;
    }

    void MakeDir(const std::string& relative) const {
        std::error_code ec;
        fs::create_directories(path_ / relative, ec);
    }

private:
    fs::path path_;
};

std::string ReadFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

// --- Valve KeyValues --------------------------------------------------------

NEXUS_TEST(ExtractsTopLevelBlocksWithTheirNames) {
    const std::string text =
        "versioninfo\n{\n\t\"editorversion\" \"400\"\n}\n"
        "world\n{\n\t\"id\" \"1\"\n\tsolid\n\t{\n\t\t\"id\" \"2\"\n\t}\n}\n";

    const std::vector<ValveBlock> blocks = ExtractNamedBlocks(text);
    CHECK_EQ(blocks.size(), static_cast<size_t>(2));
    CHECK_EQ(blocks[0].name, std::string("versioninfo"));
    CHECK_EQ(blocks[1].name, std::string("world"));

    // The nested solid must stay inside the world body rather than being
    // reported as a sibling - the whole point of tracking depth.
    CHECK(blocks[1].body.find("solid") != std::string::npos);
}

NEXUS_TEST(KeyValuesOfABlockExcludeNestedBlocks) {
    const std::string body =
        "\"id\" \"5\"\n"
        "side\n{\n\t\"material\" \"BRICK/BRICKWALL\"\n}\n"
        "side\n{\n\t\"material\" \"TOOLS/TOOLSNODRAW\"\n}\n"
        "\"classname\" \"worldspawn\"\n";

    const std::vector<ValveKeyValue> pairs = ParseKeyValueBody(body);

    // Two pairs, not four: a flat scan would attribute both sides' materials to
    // the solid, which is exactly the bug that made VMF import wrong.
    CHECK_EQ(pairs.size(), static_cast<size_t>(2));

    std::string value;
    CHECK(FindValveValue(pairs, "id", value));
    CHECK_EQ(value, std::string("5"));
    CHECK(!FindValveValue(pairs, "material", value));
}

NEXUS_TEST(KeyLookupIgnoresCase) {
    const std::vector<ValveKeyValue> pairs = ParseKeyValueBody("\"$BaseTexture\" \"concrete/floor\"");

    std::string value;
    CHECK(FindValveValue(pairs, "$basetexture", value));
    CHECK_EQ(value, std::string("concrete/floor"));
}

NEXUS_TEST(BracesInsideQuotesAndCommentsDoNotUnbalanceTheScan) {
    const std::string text =
        "// a comment with { an unbalanced brace\n"
        "entity\n{\n\t\"message\" \"a value with { and } in it\"\n\t\"classname\" \"logic_auto\"\n}\n";

    const std::vector<ValveBlock> blocks = ExtractNamedBlocks(text);
    CHECK_EQ(blocks.size(), static_cast<size_t>(1));

    std::string value;
    CHECK(FindValveValue(ParseKeyValueBody(blocks[0].body), "classname", value));
    CHECK_EQ(value, std::string("logic_auto"));
}

NEXUS_TEST(UnbalancedInputYieldsNoRunawayBlock) {
    // A truncated file must not produce a block whose body runs to end of file:
    // that reads as a valid, very large block.
    const std::vector<ValveBlock> blocks = ExtractNamedBlocks("world\n{\n\t\"id\" \"1\"\n");
    CHECK_EQ(blocks.size(), static_cast<size_t>(0));
}

NEXUS_TEST(ParsesVectorsInEveryValveSpelling) {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    CHECK(ParseValveVector3("0 64 -32", x, y, z));
    CHECK_NEAR(x, 0.0, 1e-6);
    CHECK_NEAR(y, 64.0, 1e-6);
    CHECK_NEAR(z, -32.0, 1e-6);

    CHECK(ParseValveVector3("[1.5 2.5 3.5]", x, y, z));
    CHECK_NEAR(y, 2.5, 1e-6);

    CHECK(ParseValveVector3("(4 5 6)", x, y, z));
    CHECK_NEAR(z, 6.0, 1e-6);

    // Wrong arity must fail rather than half-write the caller's values.
    x = 99.0f;
    CHECK(!ParseValveVector3("1 2", x, y, z));
    CHECK_NEAR(x, 99.0, 1e-6);
    CHECK(!ParseValveVector3("1 2 3 4", x, y, z));
}

// --- Source Engine ----------------------------------------------------------

const char* const kSampleVmf = R"VMF(
versioninfo
{
	"editorversion" "400"
}
world
{
	"id" "1"
	"classname" "worldspawn"
	solid
	{
		"id" "2"
		side
		{
			"id" "3"
			"plane" "(0 0 0) (0 64 0) (64 64 0)"
			"material" "BRICK/BRICKWALL001A"
		}
		side
		{
			"id" "4"
			"material" "TOOLS/TOOLSNODRAW"
		}
	}
	solid
	{
		"id" "5"
		side
		{
			"id" "6"
			"material" "brick/brickwall001a"
		}
	}
}
entity
{
	"id" "10"
	"classname" "info_player_start"
	"targetname" "spawn"
	"origin" "0 64 -32"
	"angles" "0 90 0"
}
entity
{
	"id" "11"
	"classname" "prop_static"
	"model" "models/props/barrel.mdl"
	"origin" "128 0 0"
}
)VMF";

NEXUS_TEST(ParsesEveryBrushAndFaceOfAVmf) {
    SourceEngineImporter::Map map;
    CHECK(SourceEngineImporter::ParseMap(kSampleVmf, map));

    // Both solids, and all three faces. The regex this replaced stopped at the
    // first inner brace, so it saw one truncated solid and no faces at all.
    CHECK_EQ(map.worldSolids.size(), static_cast<size_t>(2));
    CHECK_EQ(map.worldSolids[0].faceMaterials.size(), static_cast<size_t>(2));
    CHECK_EQ(map.worldSolids[1].faceMaterials.size(), static_cast<size_t>(1));
}

NEXUS_TEST(MaterialListIsDeduplicatedCaseInsensitively) {
    SourceEngineImporter::Map map;
    CHECK(SourceEngineImporter::ParseMap(kSampleVmf, map));

    // BRICK/BRICKWALL001A and brick/brickwall001a are the same material; Source
    // content mixes the two spellings freely.
    CHECK_EQ(map.materials.size(), static_cast<size_t>(2));
    CHECK_EQ(map.materials[0], std::string("brick/brickwall001a"));
    CHECK_EQ(map.materials[1], std::string("tools/toolsnodraw"));
}

NEXUS_TEST(ReadsEntityClassOriginAndAngles) {
    SourceEngineImporter::Map map;
    CHECK(SourceEngineImporter::ParseMap(kSampleVmf, map));
    CHECK_EQ(map.entities.size(), static_cast<size_t>(2));

    const SourceEngineImporter::MapEntity& spawn = map.entities[0];
    CHECK_EQ(spawn.className, std::string("info_player_start"));
    CHECK_EQ(spawn.targetName, std::string("spawn"));
    CHECK_NEAR(spawn.origin.y, 64.0, 1e-6);
    CHECK_NEAR(spawn.origin.z, -32.0, 1e-6);
    CHECK_NEAR(spawn.angles.y, 90.0, 1e-6);

    CHECK_EQ(map.entities[1].model, std::string("models/props/barrel.mdl"));
}

NEXUS_TEST(RejectsTextThatIsNotAMap) {
    SourceEngineImporter::Map map;
    // Header blocks alone are not a map. Accepting this would report a
    // successful import of an empty world.
    CHECK(!SourceEngineImporter::ParseMap("versioninfo\n{\n\t\"editorversion\" \"400\"\n}\n", map));
    CHECK(!SourceEngineImporter::ParseMap("", map));
}

NEXUS_TEST(ParsesVmtShaderAndTextures) {
    const std::string vmt =
        "\"LightmappedGeneric\"\n"
        "{\n"
        "\t\"$basetexture\" \"concrete\\concretefloor001a\"\n"
        "\t\"$bumpmap\" \"concrete/concretefloor001a_normal\"\n"
        "\t\"$surfaceprop\" \"concrete\"\n"
        "\t\"$translucent\" \"1\"\n"
        "}\n";

    SourceEngineImporter::Material material;
    CHECK(SourceEngineImporter::ParseMaterial(vmt, material));
    CHECK_EQ(material.shader, std::string("LightmappedGeneric"));

    // Backslashes are normalised: Source content is authored on Windows and
    // ships with both separators. They are also literal, not escapes - Valve's
    // parser has escape sequences off for VMT, and paths depend on that.
    CHECK_EQ(material.baseTexture, std::string("concrete/concretefloor001a"));
    CHECK_EQ(material.surfaceProperty, std::string("concrete"));
    CHECK(material.translucent);
}

NEXUS_TEST(RepeatedPathSeparatorsCollapse) {
    SourceEngineImporter::Material material;
    CHECK(SourceEngineImporter::ParseMaterial(
        "\"UnlitGeneric\"\n{\n\t\"$basetexture\" \"models//props\\\\crate\"\n}\n", material));
    CHECK_EQ(material.baseTexture, std::string("models/props/crate"));
}

NEXUS_TEST(PatchMaterialsPickUpTheirNestedOverrides) {
    const std::string vmt =
        "\"patch\"\n"
        "{\n"
        "\tinclude \"materials/models/base.vmt\"\n"
        "\treplace\n"
        "\t{\n"
        "\t\t\"$basetexture\" \"models/custom/skin01\"\n"
        "\t}\n"
        "}\n";

    SourceEngineImporter::Material material;
    CHECK(SourceEngineImporter::ParseMaterial(vmt, material));

    // A flat read finds nothing here: the parameters live one level down.
    CHECK_EQ(material.baseTexture, std::string("models/custom/skin01"));
}

// --- Godot ------------------------------------------------------------------

NEXUS_TEST(ReadsGodotSceneHierarchyAndTransforms) {
    TempDir dir("godot-scene");
    const fs::path scene = dir.Write("main.tscn", R"TSCN(
[gd_scene load_steps=2 format=3]

[ext_resource type="Script" path="res://player.gd" id="1_abc"]

[node name="Main" type="Node3D"]

[node name="Player" type="CharacterBody3D" parent="."]
transform = Transform3D(2, 0, 0, 0, 2, 0, 0, 0, 2, 1, 2, 3)

[node name="Camera" type="Camera3D" parent="Player"]
position = Vector3(0, 1.5, -4)
rotation = Vector3(0, 3.14, 0)
)TSCN");

    std::vector<GodotImporter::GodotNode> roots;
    CHECK(GodotImporter::ParseSceneFile(scene.string(), roots));
    CHECK_EQ(roots.size(), static_cast<size_t>(1));

    const GodotImporter::GodotNode& root = roots[0];
    CHECK_EQ(root.name, std::string("Main"));
    CHECK_EQ(root.type, std::string("Node3D"));
    CHECK_EQ(root.children.size(), static_cast<size_t>(1));

    // parent="." makes Player a child of the root; hierarchy in .tscn is by
    // path attribute, not by nesting.
    const GodotImporter::GodotNode& player = *root.children[0];
    CHECK_EQ(player.name, std::string("Player"));
    CHECK_NEAR(player.position.x, 1.0, 1e-5);
    CHECK_NEAR(player.position.y, 2.0, 1e-5);
    CHECK_NEAR(player.position.z, 3.0, 1e-5);

    // Scale comes from the length of each basis column of the Transform3D.
    CHECK_NEAR(player.scale.x, 2.0, 1e-5);

    // parent="Player" resolves one level deeper.
    CHECK_EQ(player.children.size(), static_cast<size_t>(1));
    const GodotImporter::GodotNode& camera = *player.children[0];
    CHECK_EQ(camera.name, std::string("Camera"));
    CHECK_NEAR(camera.position.z, -4.0, 1e-5);
    CHECK_NEAR(camera.rotation.y, 3.14, 1e-5);
}

NEXUS_TEST(GodotSceneWithNoNodesIsRejected) {
    TempDir dir("godot-empty");
    const fs::path scene = dir.Write("empty.tscn", "[gd_scene format=3]\n");

    std::vector<GodotImporter::GodotNode> roots;
    CHECK(!GodotImporter::ParseSceneFile(scene.string(), roots));
}

// --- JSON output ------------------------------------------------------------

NEXUS_TEST(JsonWriterEscapesAndNeverEmitsTrailingCommas) {
    JsonWriter json;
    json.BeginObject()
        .Value("path", "materials\\brick\"wall\"")
        .Value("count", 2)
        .BeginArray("items")
        .Element("a")
        .Element("b")
        .EndArray()
        .EndObject();

    const std::string text = json.ToString();

    // Backslashes and quotes escaped rather than emitted raw - Source material
    // paths contain both.
    CHECK(text.find("materials\\\\brick\\\"wall\\\"") != std::string::npos);
    CHECK(text.find(",\n  }") == std::string::npos);
    CHECK(text.find(",]") == std::string::npos);
    CHECK(text.find("[\n") != std::string::npos);
}

// --- Project detection ------------------------------------------------------

NEXUS_TEST(DetectsEachSupportedEngine) {
    GameImporter importer;

    TempDir unity("unity");
    unity.MakeDir("Assets");
    unity.Write("ProjectSettings/ProjectVersion.txt", "m_EditorVersion: 2022.3.10f1\n");
    CHECK(importer.DetectEngineType(unity.Str()) == GameImporter::EngineType::Unity);

    TempDir unreal("unreal");
    unreal.MakeDir("Content");
    unreal.Write("MyGame.uproject", "{ \"FileVersion\": 3 }\n");
    CHECK(importer.DetectEngineType(unreal.Str()) == GameImporter::EngineType::UnrealEngine);

    TempDir godot("godot");
    godot.Write("project.godot", "config_version=5\n");
    CHECK(importer.DetectEngineType(godot.Str()) == GameImporter::EngineType::Godot);

    TempDir source("source");
    source.Write("gameinfo.txt", "\"GameInfo\"\n{\n\t\"game\" \"Test\"\n}\n");
    source.MakeDir("maps");
    source.MakeDir("materials");
    CHECK(importer.DetectEngineType(source.Str()) == GameImporter::EngineType::SourceEngine);
}

NEXUS_TEST(UnrealProjectIsNotMisreadWhenItHasNoUprojectAtTheRoot) {
    // The old detector's Unreal branch fell through to the Godot check when no
    // .uproject was found, and then returned Unknown - so a project with
    // Config/DefaultEngine.ini and Content/ was never importable.
    GameImporter importer;

    TempDir dir("unreal-no-uproject");
    dir.MakeDir("Content");
    dir.Write("Config/DefaultEngine.ini", "[/Script/Engine.Engine]\n");
    CHECK(importer.DetectEngineType(dir.Str()) == GameImporter::EngineType::UnrealEngine);
}

NEXUS_TEST(UnrelatedDirectoriesAreNotDetectedAsProjects) {
    GameImporter importer;

    TempDir dir("not-a-project");
    dir.MakeDir("Content");     // One weak signal on its own is not enough.
    CHECK(importer.DetectEngineType(dir.Str()) == GameImporter::EngineType::Unknown);

    CHECK(importer.DetectEngineType((dir.Path() / "does-not-exist").string()) ==
          GameImporter::EngineType::Unknown);
}

NEXUS_TEST(UnityProjectDetectedByAssetsAloneAlsoValidates) {
    // Detection used to succeed on Assets/ + Library/ while validation demanded
    // ProjectSettings/, so the import refused to start on a project it had just
    // identified.
    GameImporter importer;

    TempDir dir("unity-assets-only");
    dir.MakeDir("Assets");
    dir.MakeDir("Library");
    dir.MakeDir("Packages");

    const GameImporter::EngineType type = importer.DetectEngineType(dir.Str());
    CHECK(type == GameImporter::EngineType::Unity);
    CHECK(importer.ValidateProjectStructure(dir.Str(), type));
}

// --- End to end -------------------------------------------------------------

NEXUS_TEST(ImportingAGodotProjectWritesSceneAndManifest) {
    TempDir project("godot-project");
    project.Write("project.godot", "config_version=5\n");
    project.Write("main.tscn",
                  "[gd_scene format=3]\n\n"
                  "[node name=\"World\" type=\"Node3D\"]\n\n"
                  "[node name=\"Light\" type=\"DirectionalLight3D\" parent=\".\"]\n"
                  "position = Vector3(0, 10, 0)\n");

    TempDir output("godot-output");

    GameImporter importer;
    GameImporter::ImportSettings settings;
    settings.outputDirectory = output.Str() + "/";

    const GameImporter::ImportResult result = importer.ImportProject(project.Str(), settings);

    CHECK(result.success);
    CHECK_EQ(result.importedAssets.size(), static_cast<size_t>(1));

    const std::string scene = ReadFile(result.importedAssets[0].nexusPath);
    CHECK(scene.find("\"World\"") != std::string::npos);
    CHECK(scene.find("\"Light\"") != std::string::npos);
    CHECK(scene.find("nexus.scene") != std::string::npos);

    const std::string manifest = ReadFile(fs::path(settings.outputDirectory) / "import_manifest.json");
    CHECK(manifest.find("nexus.import-manifest") != std::string::npos);
    CHECK(manifest.find("Godot") != std::string::npos);
}

NEXUS_TEST(ImportingAnEmptyProjectDoesNotReportSuccess) {
    // The single most important behaviour here: an import that converts nothing
    // must not come back successful. Every per-engine importer used to set
    // success=true before doing any work and never clear it.
    TempDir project("godot-empty-project");
    project.Write("project.godot", "config_version=5\n");

    TempDir output("godot-empty-output");

    GameImporter importer;
    GameImporter::ImportSettings settings;
    settings.outputDirectory = output.Str() + "/";

    const GameImporter::ImportResult result = importer.ImportProject(project.Str(), settings);
    CHECK(!result.success);
    CHECK(result.importedAssets.empty());
}

NEXUS_TEST(AssetsSharingAStemGetDistinctOutputPaths) {
    // player.tscn and player.gd both reduced to "<out>/scenes/player" and
    // "<out>/scripts/player" before - and two materials of the same name in
    // different folders collided outright, each overwriting the last while both
    // were reported imported.
    GameImporter importer;

    const std::string first =
        importer.GetNexusAssetPath("/game/materials/brick/wall.vmt", GameImporter::AssetType::Material);
    const std::string second =
        importer.GetNexusAssetPath("/game/materials/stone/wall.vmt", GameImporter::AssetType::Material);

    CHECK(first != second);

    // The extension is preserved, so the output is a file a tool can recognise.
    CHECK(first.find(".nxmat") != std::string::npos);

    // Asking again for the same source is stable.
    CHECK_EQ(importer.GetNexusAssetPath("/game/materials/brick/wall.vmt",
                                        GameImporter::AssetType::Material),
             first);
}

} // namespace

int main() {
    RUN_TEST(ExtractsTopLevelBlocksWithTheirNames);
    RUN_TEST(KeyValuesOfABlockExcludeNestedBlocks);
    RUN_TEST(KeyLookupIgnoresCase);
    RUN_TEST(BracesInsideQuotesAndCommentsDoNotUnbalanceTheScan);
    RUN_TEST(UnbalancedInputYieldsNoRunawayBlock);
    RUN_TEST(ParsesVectorsInEveryValveSpelling);

    RUN_TEST(ParsesEveryBrushAndFaceOfAVmf);
    RUN_TEST(MaterialListIsDeduplicatedCaseInsensitively);
    RUN_TEST(ReadsEntityClassOriginAndAngles);
    RUN_TEST(RejectsTextThatIsNotAMap);
    RUN_TEST(ParsesVmtShaderAndTextures);
    RUN_TEST(RepeatedPathSeparatorsCollapse);
    RUN_TEST(PatchMaterialsPickUpTheirNestedOverrides);

    RUN_TEST(ReadsGodotSceneHierarchyAndTransforms);
    RUN_TEST(GodotSceneWithNoNodesIsRejected);

    RUN_TEST(JsonWriterEscapesAndNeverEmitsTrailingCommas);

    RUN_TEST(DetectsEachSupportedEngine);
    RUN_TEST(UnrealProjectIsNotMisreadWhenItHasNoUprojectAtTheRoot);
    RUN_TEST(UnrelatedDirectoriesAreNotDetectedAsProjects);
    RUN_TEST(UnityProjectDetectedByAssetsAloneAlsoValidates);

    RUN_TEST(ImportingAGodotProjectWritesSceneAndManifest);
    RUN_TEST(ImportingAnEmptyProjectDoesNotReportSuccess);
    RUN_TEST(AssetsSharingAStemGetDistinctOutputPaths);

    return NexusTest::Summarize("ImporterTests");
}
