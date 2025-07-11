#include "UnrealTextureLoader.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

namespace Nexus {

// Asset loader implementations
std::unique_ptr<UnrealAssetLoader::UnrealAsset> UnrealAssetLoader::LoadUAsset(const std::string& filename) {
    Logger::Info("Loading Unreal Asset: " + filename);
    
    auto asset = std::make_unique<UnrealAsset>();
    asset->filename = filename;
    asset->assetType = "StaticMesh";
    
    // Create a placeholder mesh
    UnrealMesh mesh;
    mesh.name = "PlaceholderMesh";
    
    // Create a simple cube mesh
    mesh.vertices = {
        {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f},
        {-1.0f, -1.0f, 1.0f}, {1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}
    };
    
    mesh.normals = {
        {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}
    };
    
    mesh.uvs = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
    };
    
    mesh.indices = {
        0, 1, 2, 0, 2, 3,  // Front face
        4, 6, 5, 4, 7, 6,  // Back face
        0, 4, 5, 0, 5, 1,  // Bottom face
        2, 6, 7, 2, 7, 3,  // Top face
        0, 3, 7, 0, 7, 4,  // Left face
        1, 5, 6, 1, 6, 2   // Right face
    };
    
    mesh.boundingBoxMin = {-1.0f, -1.0f, -1.0f};
    mesh.boundingBoxMax = {1.0f, 1.0f, 1.0f};
    
    // Create a placeholder material
    UnrealMaterial material;
    material.name = "PlaceholderMaterial";
    material.textureSlots["BaseColor"] = "T_Default_BaseColor";
    material.textureSlots["Normal"] = "T_Default_Normal";
    material.textureSlots["Roughness"] = "T_Default_Roughness";
    material.floatParameters["Metallic"] = 0.0f;
    material.floatParameters["Roughness"] = 0.5f;
    material.floatParameters["Specular"] = 0.5f;
    material.colorParameters["BaseColor"] = {0.8f, 0.8f, 0.8f, 1.0f};
    
    mesh.materials.push_back(material);
    mesh.materialIndices.resize(mesh.indices.size() / 3, 0);
    
    asset->meshes.push_back(mesh);
    asset->materials.push_back(material);
    asset->isValid = true;
    
    Logger::Info("Created placeholder Unreal Asset with " + std::to_string(mesh.vertices.size()) + " vertices");
    return asset;
}

std::unique_ptr<UnrealAssetLoader::UnrealAsset> UnrealAssetLoader::LoadUMap(const std::string& filename) {
    Logger::Info("Loading Unreal Map: " + filename);
    
    auto asset = std::make_unique<UnrealAsset>();
    asset->filename = filename;
    asset->assetType = "World";
    
    // Create multiple placeholder meshes to represent a level
    for (int i = 0; i < 3; ++i) {
        UnrealMesh mesh;
        mesh.name = "LevelMesh_" + std::to_string(i);
        
        // Create different shapes for variety
        if (i == 0) {
            // Ground plane
            mesh.vertices = {
                {-10.0f, 0.0f, -10.0f}, {10.0f, 0.0f, -10.0f}, {10.0f, 0.0f, 10.0f}, {-10.0f, 0.0f, 10.0f}
            };
            mesh.normals = {
                {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}
            };
            mesh.uvs = {
                {0.0f, 0.0f}, {10.0f, 0.0f}, {10.0f, 10.0f}, {0.0f, 10.0f}
            };
            mesh.indices = {0, 1, 2, 0, 2, 3};
        } else {
            // Pillars or structures
            float x = (i - 1) * 5.0f;
            mesh.vertices = {
                {x - 0.5f, 0.0f, -0.5f}, {x + 0.5f, 0.0f, -0.5f}, {x + 0.5f, 3.0f, -0.5f}, {x - 0.5f, 3.0f, -0.5f},
                {x - 0.5f, 0.0f, 0.5f}, {x + 0.5f, 0.0f, 0.5f}, {x + 0.5f, 3.0f, 0.5f}, {x - 0.5f, 3.0f, 0.5f}
            };
            mesh.normals = {
                {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f},
                {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}
            };
            mesh.uvs = {
                {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
                {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
            };
            mesh.indices = {
                0, 1, 2, 0, 2, 3,  // Front face
                4, 6, 5, 4, 7, 6,  // Back face
                0, 4, 5, 0, 5, 1,  // Bottom face
                2, 6, 7, 2, 7, 3,  // Top face
                0, 3, 7, 0, 7, 4,  // Left face
                1, 5, 6, 1, 6, 2   // Right face
            };
        }
        
        mesh.boundingBoxMin = {-10.0f, 0.0f, -10.0f};
        mesh.boundingBoxMax = {10.0f, 3.0f, 10.0f};
        
        UnrealMaterial material;
        material.name = "LevelMaterial_" + std::to_string(i);
        material.textureSlots["BaseColor"] = "T_Level_BaseColor_" + std::to_string(i);
        material.floatParameters["Roughness"] = 0.8f;
        material.colorParameters["BaseColor"] = {0.5f + i * 0.2f, 0.5f, 0.5f, 1.0f};
        
        mesh.materials.push_back(material);
        mesh.materialIndices.resize(mesh.indices.size() / 3, 0);
        
        asset->meshes.push_back(mesh);
        asset->materials.push_back(material);
    }
    
    asset->isValid = true;
    Logger::Info("Created placeholder Unreal Map with " + std::to_string(asset->meshes.size()) + " meshes");
    return asset;
}

std::unique_ptr<UnrealAssetLoader::UnrealAsset> UnrealAssetLoader::LoadFBX(const std::string& filename) {
    Logger::Info("Loading FBX: " + filename);
    
    auto asset = std::make_unique<UnrealAsset>();
    asset->filename = filename;
    asset->assetType = "FBX";
    
    // Create a placeholder mesh representing an FBX model
    UnrealMesh mesh;
    mesh.name = "FBX_Mesh";
    
    // Create a more complex mesh (tetrahedron)
    mesh.vertices = {
        {0.0f, 1.0f, 0.0f},    // Top vertex
        {-1.0f, -1.0f, 1.0f},  // Front left
        {1.0f, -1.0f, 1.0f},   // Front right
        {0.0f, -1.0f, -1.0f}   // Back center
    };
    
    mesh.normals = {
        {0.0f, 1.0f, 0.0f},
        {-0.5f, -0.5f, 0.5f},
        {0.5f, -0.5f, 0.5f},
        {0.0f, -0.5f, -0.5f}
    };
    
    mesh.uvs = {
        {0.5f, 1.0f},
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.5f, 0.0f}
    };
    
    mesh.indices = {
        0, 1, 2,  // Top triangle
        0, 2, 3,  // Right triangle
        0, 3, 1,  // Left triangle
        1, 3, 2   // Bottom triangle
    };
    
    mesh.boundingBoxMin = {-1.0f, -1.0f, -1.0f};
    mesh.boundingBoxMax = {1.0f, 1.0f, 1.0f};
    
    UnrealMaterial material;
    material.name = "FBX_Material";
    material.textureSlots["BaseColor"] = "T_FBX_BaseColor";
    material.textureSlots["Normal"] = "T_FBX_Normal";
    material.floatParameters["Metallic"] = 0.2f;
    material.floatParameters["Roughness"] = 0.7f;
    material.colorParameters["BaseColor"] = {0.6f, 0.8f, 0.9f, 1.0f};
    
    mesh.materials.push_back(material);
    mesh.materialIndices.resize(mesh.indices.size() / 3, 0);
    
    asset->meshes.push_back(mesh);
    asset->materials.push_back(material);
    asset->isValid = true;
    
    Logger::Info("Created placeholder FBX asset with " + std::to_string(mesh.vertices.size()) + " vertices");
    return asset;
}

std::unique_ptr<UnrealAssetLoader::UnrealAsset> UnrealAssetLoader::LoadOBJ(const std::string& filename) {
    Logger::Info("Loading OBJ: " + filename);
    
    auto asset = std::make_unique<UnrealAsset>();
    asset->filename = filename;
    asset->assetType = "OBJ";
    
    // Create a placeholder mesh representing an OBJ model
    UnrealMesh mesh;
    mesh.name = "OBJ_Mesh";
    
    // Create an octahedron
    mesh.vertices = {
        {0.0f, 1.0f, 0.0f},   // Top
        {1.0f, 0.0f, 0.0f},   // Right
        {0.0f, 0.0f, 1.0f},   // Front
        {-1.0f, 0.0f, 0.0f},  // Left
        {0.0f, 0.0f, -1.0f},  // Back
        {0.0f, -1.0f, 0.0f}   // Bottom
    };
    
    mesh.normals = {
        {0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f},
        {0.0f, -1.0f, 0.0f}
    };
    
    mesh.uvs = {
        {0.5f, 1.0f},
        {1.0f, 0.5f},
        {0.5f, 0.5f},
        {0.0f, 0.5f},
        {0.5f, 0.0f},
        {0.5f, 0.0f}
    };
    
    mesh.indices = {
        0, 1, 2,  // Top front right
        0, 2, 3,  // Top front left
        0, 3, 4,  // Top back left
        0, 4, 1,  // Top back right
        5, 2, 1,  // Bottom front right
        5, 3, 2,  // Bottom front left
        5, 4, 3,  // Bottom back left
        5, 1, 4   // Bottom back right
    };
    
    mesh.boundingBoxMin = {-1.0f, -1.0f, -1.0f};
    mesh.boundingBoxMax = {1.0f, 1.0f, 1.0f};
    
    UnrealMaterial material;
    material.name = "OBJ_Material";
    material.textureSlots["BaseColor"] = "T_OBJ_BaseColor";
    material.floatParameters["Roughness"] = 0.6f;
    material.colorParameters["BaseColor"] = {0.9f, 0.6f, 0.3f, 1.0f};
    
    mesh.materials.push_back(material);
    mesh.materialIndices.resize(mesh.indices.size() / 3, 0);
    
    asset->meshes.push_back(mesh);
    asset->materials.push_back(material);
    asset->isValid = true;
    
    Logger::Info("Created placeholder OBJ asset with " + std::to_string(mesh.vertices.size()) + " vertices");
    return asset;
}

std::unique_ptr<UnrealAssetLoader::UnrealAsset> UnrealAssetLoader::LoadDAE(const std::string& filename) {
    Logger::Info("Loading DAE (Collada): " + filename);
    
    auto asset = std::make_unique<UnrealAsset>();
    asset->filename = filename;
    asset->assetType = "DAE";
    
    // Create a placeholder mesh representing a DAE model
    UnrealMesh mesh;
    mesh.name = "DAE_Mesh";
    
    // Create a simple pyramid
    mesh.vertices = {
        {0.0f, 1.0f, 0.0f},     // Top
        {-1.0f, 0.0f, 1.0f},    // Front left
        {1.0f, 0.0f, 1.0f},     // Front right
        {1.0f, 0.0f, -1.0f},    // Back right
        {-1.0f, 0.0f, -1.0f}    // Back left
    };
    
    mesh.normals = {
        {0.0f, 1.0f, 0.0f},
        {-0.5f, 0.0f, 0.5f},
        {0.5f, 0.0f, 0.5f},
        {0.5f, 0.0f, -0.5f},
        {-0.5f, 0.0f, -0.5f}
    };
    
    mesh.uvs = {
        {0.5f, 1.0f},
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };
    
    mesh.indices = {
        0, 1, 2,  // Front face
        0, 2, 3,  // Right face
        0, 3, 4,  // Back face
        0, 4, 1,  // Left face
        1, 4, 3,  // Bottom face triangle 1
        1, 3, 2   // Bottom face triangle 2
    };
    
    mesh.boundingBoxMin = {-1.0f, 0.0f, -1.0f};
    mesh.boundingBoxMax = {1.0f, 1.0f, 1.0f};
    
    UnrealMaterial material;
    material.name = "DAE_Material";
    material.textureSlots["BaseColor"] = "T_DAE_BaseColor";
    material.textureSlots["Normal"] = "T_DAE_Normal";
    material.floatParameters["Metallic"] = 0.1f;
    material.floatParameters["Roughness"] = 0.4f;
    material.colorParameters["BaseColor"] = {0.7f, 0.5f, 0.8f, 1.0f};
    
    mesh.materials.push_back(material);
    mesh.materialIndices.resize(mesh.indices.size() / 3, 0);
    
    asset->meshes.push_back(mesh);
    asset->materials.push_back(material);
    asset->isValid = true;
    
    Logger::Info("Created placeholder DAE asset with " + std::to_string(mesh.vertices.size()) + " vertices");
    return asset;
}

// Validation functions
bool UnrealAssetLoader::ValidateAsset(const UnrealAsset& asset) {
    if (asset.filename.empty()) {
        LogError("Asset filename is empty");
        return false;
    }
    
    if (asset.meshes.empty()) {
        LogWarning("Asset has no meshes");
    }
    
    for (const auto& mesh : asset.meshes) {
        if (!ValidateMesh(mesh)) {
            return false;
        }
    }
    
    for (const auto& material : asset.materials) {
        if (!ValidateMaterial(material)) {
            return false;
        }
    }
    
    return true;
}

bool UnrealAssetLoader::ValidateMesh(const UnrealMesh& mesh) {
    if (mesh.vertices.empty()) {
        LogError("Mesh has no vertices");
        return false;
    }
    
    if (mesh.indices.empty()) {
        LogError("Mesh has no indices");
        return false;
    }
    
    if (mesh.indices.size() % 3 != 0) {
        LogError("Mesh indices count is not a multiple of 3");
        return false;
    }
    
    return true;
}

bool UnrealAssetLoader::ValidateMaterial(const UnrealMaterial& material) {
    if (material.name.empty()) {
        LogError("Material name is empty");
        return false;
    }
    
    return true;
}

// Utility functions
std::vector<uint8_t> UnrealAssetLoader::ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        LogError("Failed to open file: " + filename);
        return {};
    }
    
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    file.close();
    
    return data;
}

std::string UnrealAssetLoader::GetFileExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }
    return filename.substr(dotPos + 1);
}

std::string UnrealAssetLoader::GetBasePath(const std::string& filename) {
    size_t slashPos = filename.find_last_of("/\\");
    if (slashPos == std::string::npos) {
        return ".";
    }
    return filename.substr(0, slashPos);
}

void UnrealAssetLoader::LogError(const std::string& message) {
    Logger::Error("UnrealAssetLoader: " + message);
}

void UnrealAssetLoader::LogWarning(const std::string& message) {
    Logger::Warning("UnrealAssetLoader: " + message);
}

void UnrealAssetLoader::LogInfo(const std::string& message) {
    Logger::Info("UnrealAssetLoader: " + message);
}

} // namespace Nexus
