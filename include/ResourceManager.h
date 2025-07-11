#pragma once

#include "Platform.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Nexus {

class Texture;
class Mesh;

/**
 * Resource management system for textures, meshes, sounds, etc.
 */
class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    // Initialization
    bool Initialize(ID3D11Device* device = nullptr);
    void Shutdown();

    // Texture management
    std::shared_ptr<Texture> LoadTexture(const std::string& name, const std::string& filename);
    std::shared_ptr<Texture> GetTexture(const std::string& name);
    void UnloadTexture(const std::string& name);

    // Mesh management
    std::shared_ptr<Mesh> LoadMesh(const std::string& name, const std::string& filename);
    std::shared_ptr<Mesh> GetMesh(const std::string& name);
    void UnloadMesh(const std::string& name);

    // Resource paths
    void AddResourcePath(const std::string& path);
    std::string FindResourceFile(const std::string& filename);

    // Memory management
    void ClearUnusedResources();
    size_t GetMemoryUsage() const;

private:
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures_;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes_;
    std::vector<std::string> resourcePaths_;
    
    bool initialized_;
    size_t memoryUsage_;
    ID3D11Device* device_;  // Graphics device for resource loading
};

} // namespace Nexus
