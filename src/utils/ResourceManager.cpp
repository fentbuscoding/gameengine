#include "ResourceManager.h"
#include "Texture.h"
#include "Mesh.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>

namespace Nexus {

ResourceManager::ResourceManager()
    : initialized_(false)
    , memoryUsage_(0)
    , device_(nullptr)
{
}

ResourceManager::~ResourceManager() {
    Shutdown();
}

bool ResourceManager::Initialize(ID3D11Device* device) {
    if (initialized_) return true;
    
    device_ = device;
    
    // Add default resource paths
    AddResourcePath("assets");
    AddResourcePath("textures");
    AddResourcePath("meshes");
    AddResourcePath("sounds");
    
    initialized_ = true;
    Logger::Info("Resource manager initialized");
    return true;
}

void ResourceManager::Shutdown() {
    if (!initialized_) return;
    
    // Clear all resources
    textures_.clear();
    meshes_.clear();
    
    initialized_ = false;
    memoryUsage_ = 0;
    device_ = nullptr;
    Logger::Info("Resource manager shutdown");
}

std::shared_ptr<Texture> ResourceManager::LoadTexture(const std::string& name, const std::string& filename) {
    // Check if already loaded
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        return it->second;
    }
    
    // Find the file
    std::string fullPath = FindResourceFile(filename);
    if (fullPath.empty()) {
        Logger::Error("Could not find texture file: " + filename);
        return nullptr;
    }
    
    // Load the texture
    auto texture = std::make_shared<Texture>();
    if (texture->LoadFromFile(fullPath, device_)) {
        textures_[name] = texture;
        memoryUsage_ += texture->GetMemoryUsage();
        Logger::Info("Loaded texture: " + name + " (" + std::to_string(texture->GetMemoryUsage()) + " bytes)");
        return texture;
    }
    
    Logger::Error("Failed to load texture: " + filename);
    return nullptr;
}

std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string& name) {
    auto it = textures_.find(name);
    return (it != textures_.end()) ? it->second : nullptr;
}

void ResourceManager::UnloadTexture(const std::string& name) {
    textures_.erase(name);
}

std::shared_ptr<Mesh> ResourceManager::LoadMesh(const std::string& name, const std::string& filename) {
    // Check if already loaded
    auto it = meshes_.find(name);
    if (it != meshes_.end()) {
        return it->second;
    }
    
    // Find the file
    std::string fullPath = FindResourceFile(filename);
    if (fullPath.empty()) {
        Logger::Error("Could not find mesh file: " + filename);
        return nullptr;
    }
    
    // Load the mesh
    auto mesh = std::make_shared<Mesh>();
    if (mesh->LoadFromFile(fullPath, device_)) {
        meshes_[name] = mesh;
        memoryUsage_ += mesh->GetMemoryUsage();
        Logger::Info("Loaded mesh: " + name + " (" + std::to_string(mesh->GetMemoryUsage()) + " bytes)");
        return mesh;
    }
    
    Logger::Error("Failed to load mesh: " + filename);
    return nullptr;
}

std::shared_ptr<Mesh> ResourceManager::GetMesh(const std::string& name) {
    auto it = meshes_.find(name);
    return (it != meshes_.end()) ? it->second : nullptr;
}

void ResourceManager::UnloadMesh(const std::string& name) {
    meshes_.erase(name);
}

void ResourceManager::AddResourcePath(const std::string& path) {
    resourcePaths_.push_back(path);
}

std::string ResourceManager::FindResourceFile(const std::string& filename) {
    // Try the filename as-is first
    std::ifstream file(filename);
    if (file.good()) {
        return filename;
    }
    
    // Try each resource path
    for (const auto& path : resourcePaths_) {
        std::string fullPath = path + "/" + filename;
        std::ifstream testFile(fullPath);
        if (testFile.good()) {
            return fullPath;
        }
    }
    
    return "";
}

void ResourceManager::ClearUnusedResources() {
    size_t freedMemory = 0;
    
    // Clear unused textures
    for (auto it = textures_.begin(); it != textures_.end();) {
        if (it->second.use_count() == 1) {
            freedMemory += it->second->GetMemoryUsage();
            it = textures_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Clear unused meshes
    for (auto it = meshes_.begin(); it != meshes_.end();) {
        if (it->second.use_count() == 1) {
            freedMemory += it->second->GetMemoryUsage();
            it = meshes_.erase(it);
        } else {
            ++it;
        }
    }
    
    memoryUsage_ -= freedMemory;
    
    if (freedMemory > 0) {
        Logger::Info("Freed " + std::to_string(freedMemory) + " bytes of unused resources");
    }
}

size_t ResourceManager::GetMemoryUsage() const {
    return memoryUsage_;
}

} // namespace Nexus
