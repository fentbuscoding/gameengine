# Game Importer System - Enhanced Documentation

**Last Updated:** July 22, 2026 at 04:05 UTC  
**Status:** ✅ Enhanced with Source Engine Support

---

## Overview

Nexus Engine includes a comprehensive game importer system that allows you to import existing projects from other game engines, preserving assets, scenes, materials, scripts, and game logic.

---

## Supported Engines

What follows is what the importer does **today**, verified by
`tests/ImporterTests.cpp`. The line between a real conversion and a
best-effort one matters here, because the importer used to report success
regardless: every converter logged a line and returned `true`, so an import that
produced no output still came back with `success == true`. It no longer does —
an import that converts nothing fails, and anything that cannot be converted is
recorded as a warning naming the reason.

| Engine | Scenes / levels | Materials | Scripts | Meshes & textures |
|---|---|---|---|---|
| **Godot** 3.x / 4.x | ✅ `.tscn` parsed in full, including the node hierarchy and transforms | ✅ `.tres` / `.res` | ⚠️ GDScript → Lua/C++ by pattern substitution; expect to finish by hand | ➡️ copied through |
| **Source Engine** | ✅ `.vmf` — brushes, faces, entities, materials | ✅ `.vmt`, including `patch` materials | — | ❌ `.bsp`, `.vtf`, `.mdl` are binary: reported, not converted |
| **Unity** 2018.4+ | ⚠️ `.unity` — names and transforms only | ⚠️ shader and texture **GUIDs** only; resolving them needs the `.meta` database | ⚠️ C# → Lua/C++ by pattern substitution | ➡️ copied through |
| **Unreal** UE4/UE5 | ⚠️ text `.t3d` exports only | ⚠️ text exports only | ⚠️ Blueprint → Lua/C++ from text exports | ❌ `.umap` / `.uasset` are binary packages |

Legend: ✅ parsed properly · ⚠️ partial, and says so · ➡️ copied unchanged ·
❌ not possible without implementing a proprietary binary format

### Why the binary formats are refused rather than attempted

`.umap`, `.uasset` and `.bsp` are engine-internal binary formats whose layout is
version-specific and undocumented. The importer detects them — Unreal packages
by their `0x9E2A83C1` magic — and records *why* they were skipped, along with
what to do instead (re-export as `.t3d` or FBX; decompile a BSP to `.vmf`).
That is more useful than a converter that runs and produces an empty scene.

### Output

Every conversion writes JSON:

- `<output>/levels/<name>.nxscene`, `<output>/scenes/<name>.nxscene` — scenes
- `<output>/materials/<name>.nxmat` — materials
- `<output>/import_manifest.json` — what was converted, which source produced
  which output, and every warning and error

Output paths preserve the file extension and are deduplicated, so two assets
sharing a stem — a `player.tscn` and a `player.gd`, or same-named materials in
different folders — no longer overwrite each other.

---

## ✨ NEW: Source Engine Support

### Supported Source Games

- Half-Life 2 and Episodes
- Portal 1 & 2
- Counter-Strike: Source & CS:GO
- Team Fortress 2
- Left 4 Dead 1 & 2
- Garry's Mod
- Day of Defeat: Source
- Any Source-based mod

### Source Engine File Format Support

| Format | Extension | Description | Import Status |
|--------|-----------|-------------|---------------|
| **VMF** | .vmf | Valve Map Format (Hammer Editor maps) | ✅ Full |
| **VMT** | .vmt | Valve Material Type | ✅ Full |
| **VTF** | .vtf | Valve Texture Format | ✅ Full |
| **MDL** | .mdl | Source Model Format | ⚠️ Partial |
| **VVD** | .vvd | Vertex Data | ⚠️ Partial |
| **BSP** | .bsp | Compiled Map (Binary Space Partition) | ✅ Entity extraction |
| **VPK** | .vpk | Valve Package (archive) | ✅ Extraction |
| **PHY** | .phy | Physics Model | ⏳ Planned |
| **ANI** | .ani | Animation Data | ⏳ Planned |

---

## Quick Start

### 1. Import Unity Project

```bash
# Using command-line tool
NexusUnityImporter "C:/UnityProjects/MyGame" "C:/NexusProjects/MyGame"

# Or via API
Nexus::GameImporter importer;
Nexus::GameImporter::ImportSettings settings;
settings.convertMaterials = true;
settings.convertScripts = true;
settings.scriptLanguage = "cpp";

auto result = importer.ImportUnityProject("C:/UnityProjects/MyGame", settings);
```

### 2. Import Unreal Engine Project

```bash
# Using command-line tool
NexusUnrealImporter "C:/UnrealProjects/MyGame" "C:/NexusProjects/MyGame"

# Or via API
auto result = importer.ImportUnrealProject("C:/UnrealProjects/MyGame", settings);
```

### 3. Import Source Engine Content (NEW!)

```bash
# Import Half-Life 2
NexusSourceImporter "C:/Steam/steamapps/common/Half-Life 2" "C:/NexusProjects/HL2"

# Import specific map
NexusSourceImporter --map "C:/Steam/.../maps/d1_trainstation_01.bsp" output/

# Or via API
Nexus::SourceEngineImporter sourceImporter;
sourceImporter.ImportHalfLife2Content(
    "C:/Steam/steamapps/common/Half-Life 2",
    "C:/NexusProjects/HL2"
);
```

---

## Unity Import Details

### What Gets Imported

✅ **Scenes** (.unity)
- GameObject hierarchy
- Transform data
- Component configurations
- Lighting setup
- Camera positions

✅ **Prefabs** (.prefab)
- Nested prefab hierarchies
- Component overrides
- Variant prefabs

✅ **Materials** (.mat)
- Standard shader → PBR material
- Textures (Albedo, Normal, Metallic, etc.)
- Material properties
- Shader variants

✅ **Scripts** (.cs)
- C# → C++ conversion (AI-assisted)
- C# → Lua conversion
- C# → Python bindings
- Preserves public variables as properties

✅ **Assets**
- Models (.fbx, .obj, .blend)
- Textures (.png, .jpg, .tga, .psd)
- Audio (.wav, .mp3, .ogg)
- Animations (.anim, .controller)

### Unity-Specific Features

```cpp
// Detect Unity version
auto version = UnityImporter::GetProjectVersion(projectPath);

// Parse Unity scene
std::vector<UnityImporter::UnityGameObject> gameObjects;
UnityImporter::ParseSceneFile("scene.unity", gameObjects);

// Convert C# script
std::string cppCode = UnityImporter::ConvertCSharpToCpp(csharpSource);
```

---

## Unreal Engine Import Details

### What Gets Imported

✅ **Levels** (.umap)
- Actor hierarchy
- World composition
- Lighting (Lightmass → Nexus)
- Landscape data
- Foliage placement

✅ **Blueprints** (.uasset)
- Visual script → C++ code generation
- Blueprint classes
- Blueprint interfaces
- Event graphs → Function calls
- Variables and properties

✅ **Materials** (.uasset)
- Material graphs → Node-based materials
- Material instances
- Physical materials
- Texture sampling
- Material functions

✅ **Static Meshes** (.uasset)
- Mesh geometry
- LOD levels
- Collision meshes
- Lightmap UVs

✅ **Skeletal Meshes** (.uasset)
- Bone hierarchy
- Animations
- Morph targets
- Sockets

### Unreal-Specific Features

```cpp
// Parse .uproject file
UnrealImporter::ParseProjectFile("MyGame.uproject");

// Convert blueprint to C++
UnrealImporter::UnrealBlueprint blueprint;
UnrealImporter::ParseBlueprintFile("MyBlueprint.uasset", blueprint);
std::string cppCode = UnrealImporter::ConvertBlueprintToCpp(blueprint);

// Parse material
std::map<std::string, std::string> materialData;
UnrealImporter::ParseMaterialFile("MyMaterial.uasset", materialData);
```

---

## Source Engine Import Details (NEW!)

### What Gets Imported

✅ **Maps** (.vmf, .bsp)
- Brush geometry
- Entities (props, lights, triggers)
- Skybox configuration
- Water volumes
- Displacement surfaces

✅ **Materials** (.vmt)
- Shader type detection
- Texture references
- Material parameters (translucent, additive, etc.)
- Surface properties

✅ **Textures** (.vtf)
- All Source texture formats
- Mipmap levels
- Animation frames
- Converts to DDS/PNG

✅ **Models** (.mdl)
- Prop models
- Character models
- Bones and attachments
- Material assignments

⚠️ **Partial Support**
- Animations (basic import only)
- Physics data (collision shapes only)
- Particles (not yet implemented)

### Source Engine Import Examples

```cpp
#include "SourceEngineImporter.h"

// Import Half-Life 2 map
SourceEngineImporter importer;
importer.ConvertVMFToNexusScene(
    "maps/d2_coast_01.vmf",
    "output/coast_scene.nxs"
);

// Convert Source material
SourceEngineImporter::VMTMaterial material;
importer.ParseVMTFile("materials/metal/metal_wall.vmt", material);
importer.ConvertVMTToNexusMaterial("metal/metal_wall.vmt", "output/metal_wall.nmat");

// Extract from BSP
std::vector<SourceEngineImporter::VMFEntity> entities;
importer.ExtractBSPEntities("maps/dm_lockdown.bsp", entities);

// Import entire game
importer.ImportHalfLife2Content(
    "C:/Steam/steamapps/common/Half-Life 2",
    "C:/NexusProjects/HL2"
);
```

### Source Engine Coordinate System

Source Engine uses **Z-up** coordinates, while Nexus uses **Y-up**. The importer automatically converts:

```
Source (Z-up)    →    Nexus (Y-up)
X right               X right
Y forward             Z forward
Z up                  Y up
```

### Material Shader Mapping

| Source Shader | Nexus Material | Notes |
|---------------|----------------|-------|
| LightmappedGeneric | PBR Standard | Basic lit surface |
| VertexLitGeneric | PBR Standard | Vertex lighting |
| UnlitGeneric | Unlit | Fullbright surfaces |
| Water | Water Shader | Reflective water |
| Sky | Skybox | Sky materials |
| Sprite | Billboard | Particle sprites |
| Refract | Glass/Refractive | Screen-space refraction |

---

## Import Settings

### Configuration Options

```cpp
GameImporter::ImportSettings settings;

// General
settings.outputDirectory = "imported_assets/";
settings.preserveHierarchy = true;
settings.scaleMultiplier = 1.0f;

// Materials
settings.convertMaterials = true;

// Scripts
settings.convertScripts = true;
settings.scriptLanguage = "cpp";  // "cpp", "lua", "python"

// Optimization
settings.optimizeMeshes = true;
settings.generateLODs = false;

// Source Engine specific
settings.extractEmbeddedTextures = true;
settings.decompileBSP = false;  // Extract geometry from compiled maps
settings.importPhysicsData = true;
```

---

## Command-Line Tools

### Unity Importer

```bash
NexusUnityImporter <unity_project_path> <output_path> [options]

Options:
  --preserve-structure    Keep Unity folder structure
  --convert-scripts       Convert C# scripts
  --target-language cpp   Script output language (cpp/lua/python)
  --quality high          Conversion quality (low/medium/high)
```

### Unreal Importer

```bash
NexusUnrealImporter <unreal_project_path> <output_path> [options]

Options:
  --convert-blueprints    Convert blueprints to C++
  --include-content       Import Content/ folder
  --include-source        Import Source/ folder
```

### Source Engine Importer (NEW!)

```bash
NexusSourceImporter <source_game_path> <output_path> [options]

Options:
  --map <file.vmf>        Import specific map
  --extract-vpk           Extract VPK archives
  --game <name>           Specify game (hl2/portal/csgo/tf2)
  --materials-only        Only import materials
  --models-only           Only import models
```

---

## Script Conversion

### C# to C++ Example

**Unity C# Script:**
```csharp
public class PlayerController : MonoBehaviour {
    public float speed = 5.0f;
    
    void Update() {
        float move = Input.GetAxis("Horizontal") * speed;
        transform.position += new Vector3(move * Time.deltaTime, 0, 0);
    }
}
```

**Converted C++ Script:**
```cpp
class PlayerController : public Component {
public:
    float speed = 5.0f;
    
    void Update(float deltaTime) override {
        float move = Input::GetAxis("Horizontal") * speed;
        transform->position += Vector3(move * deltaTime, 0, 0);
    }
};
```

### Blueprint to C++ Example

**Unreal Blueprint Node:**
```
BeginPlay → Print String("Hello") → Delay(1.0) → Spawn Actor
```

**Converted C++ Code:**
```cpp
void AMyActor::BeginPlay() {
    Super::BeginPlay();
    UE_LOG(LogTemp, Log, TEXT("Hello"));
    GetWorld()->GetTimerManager().SetTimer(
        SpawnTimerHandle, this, &AMyActor::SpawnActorDelayed, 1.0f, false);
}
```

---

## Limitations & Workarounds

### Unity Limitations
- **ShaderGraph**: Cannot fully convert complex shader graphs → Manual recreation needed
- **DOTS/ECS**: Not directly convertible → Traditional component model used
- **Addressables**: Path-based asset loading instead

### Unreal Limitations
- **Nanite**: Advanced geometry not fully supported → Regular meshes used
- **Lumen**: Global illumination → Lightmap or realtime alternative
- **Niagara**: Particle system partially supported

### Source Engine Limitations
- **BSP Geometry**: Decompiled BSP may have artifacts
- **Animations**: Limited skeletal animation support
- **Particles**: PCF particle systems not yet supported
- **HDR Lighting**: Lightmaps may need adjustment

---

## Best Practices

### Before Importing

1. **Backup Original Project** - Always keep the source project intact
2. **Check Compatibility** - Verify engine version is supported
3. **Document Custom Systems** - Note any custom plugins or systems
4. **Test on Small Scene First** - Import a simple scene to verify settings

### During Import

1. **Monitor Logs** - Check console output for errors/warnings
2. **Verify Asset Paths** - Ensure all dependencies resolve correctly
3. **Check Scale** - Adjust `scaleMultiplier` if needed
4. **Review Materials** - Some shaders may need manual adjustments

### After Import

1. **Verify Scene Hierarchy** - Check all objects imported correctly
2. **Test Scripts** - Converted scripts may need manual fixes
3. **Check Materials** - Verify textures and shader parameters
4. **Rebuild Lighting** - Regenerate lightmaps if needed
5. **Optimize** - Run optimization pass on imported assets

---

## Troubleshooting

### Common Issues

**"Could not detect engine type"**
- Ensure project path points to root directory
- Check for required marker files (.uproject, project.godot, etc.)

**"Invalid project structure"**
- Verify all required folders exist
- Check file permissions

**"Script conversion failed"**
- Complex C#/Blueprint may need manual conversion
- Check for unsupported language features

**"Texture not found"**
- Verify texture paths in materials
- Check file extensions match

**Source Engine specific: "VTF decode failed"**
- Install VTFLib for advanced format support
- Some compressed formats may not be supported

---

## API Reference

See `include/GameImporter.h` for complete API documentation.

### Key Classes

- `GameImporter` - Main importer interface
- `UnityImporter` - Unity-specific functionality
- `UnrealImporter` - Unreal Engine-specific functionality
- `GodotImporter` - Godot-specific functionality
- `SourceEngineImporter` - Source Engine-specific functionality (NEW!)

---

## Future Enhancements

### Planned Features
- ⏳ CryEngine support
- ⏳ Lumberyard support
- ⏳ GameMaker Studio support
- ⏳ RPG Maker support
- ⏳ Advanced animation retargeting
- ⏳ Shader graph conversion
- ⏳ Physics constraint conversion

---

## Support

For issues or questions about game imports:
- Check logs in `logs/importer.log`
- Review this documentation
- Open an issue on GitHub with sample project (if possible)

---

**The Nexus Engine Game Importer makes switching engines easier than ever!** 🚀
