#include "Engine.h"
#include "GraphicsDevice.h"
#include "UnrealTextureLoader.h"
#include "Logger.h"
#include <iostream>
#include <vector>
#include <string>

using namespace Nexus;

// Helper function to replace std::string::ends_with (C++20 only)
bool ends_with(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

int main() {
    std::cout << "==================================" << std::endl;
    std::cout << "    Nexus Engine Texture Test     " << std::endl;
    std::cout << "==================================" << std::endl;
    
    // Initialize logger
    Logger::Initialize();
    
    // Test various texture formats
    std::vector<std::pair<std::string, std::string>> testTextures = {
        {"test.dds", "DirectDraw Surface"},
        {"test.tga", "Truevision TGA"},
        {"test.bmp", "Windows Bitmap"},
        {"test.png", "Portable Network Graphics"},
        {"test.jpg", "JPEG Image"},
        {"test.hdr", "High Dynamic Range"},
        {"test.exr", "OpenEXR"},
        {"MyTexture.uasset", "Unreal Engine Asset"},
        {"Level01.umap", "Unreal Engine Map"},
    };
    
    std::cout << "\nTesting Unreal Engine Texture Loading System..." << std::endl;
    std::cout << "=================================================" << std::endl;
    
    for (const auto& [filename, description] : testTextures) {
        std::cout << "\nLoading " << description << " (" << filename << ")..." << std::endl;
        
        auto texture = UnrealTextureLoader::LoadUnrealTexture(filename);
        if (texture && texture->IsValid()) {
            std::cout << "✓ Successfully loaded: " << texture->metadata.width << "x" << texture->metadata.height << std::endl;
            std::cout << "  Format: " << UnrealTextureLoader::GetFormatName(texture->metadata.format) << std::endl;
            std::cout << "  Data size: " << texture->data.size() << " bytes" << std::endl;
            std::cout << "  Has alpha: " << (texture->metadata.hasAlpha ? "Yes" : "No") << std::endl;
            std::cout << "  Mip levels: " << texture->metadata.mipLevels << std::endl;
        } else {
            std::cout << "✗ Failed to load texture" << std::endl;
        }
    }
    
    std::cout << "\n\nTesting Unreal Engine Asset Loading System..." << std::endl;
    std::cout << "=============================================" << std::endl;
    
    std::vector<std::pair<std::string, std::string>> testAssets = {
        {"Character.uasset", "Character Mesh"},
        {"Level.umap", "Game Level"},
        {"Weapon.fbx", "FBX Model"},
        {"Building.obj", "OBJ Model"},
        {"Animation.dae", "Collada Animation"}
    };
    
    for (const auto& [filename, description] : testAssets) {
        std::cout << "\nLoading " << description << " (" << filename << ")..." << std::endl;
        
        std::unique_ptr<UnrealAssetLoader::UnrealAsset> asset;
        
        if (ends_with(filename, ".uasset")) {
            asset = UnrealAssetLoader::LoadUAsset(filename);
        } else if (ends_with(filename, ".umap")) {
            asset = UnrealAssetLoader::LoadUMap(filename);
        } else if (ends_with(filename, ".fbx")) {
            asset = UnrealAssetLoader::LoadFBX(filename);
        } else if (ends_with(filename, ".obj")) {
            asset = UnrealAssetLoader::LoadOBJ(filename);
        } else if (ends_with(filename, ".dae")) {
            asset = UnrealAssetLoader::LoadDAE(filename);
        }
        
        if (asset && asset->isValid) {
            std::cout << "✓ Successfully loaded asset" << std::endl;
            std::cout << "  Type: " << asset->assetType << std::endl;
            std::cout << "  Meshes: " << asset->meshes.size() << std::endl;
            std::cout << "  Materials: " << asset->materials.size() << std::endl;
            
            for (size_t i = 0; i < asset->meshes.size(); ++i) {
                const auto& mesh = asset->meshes[i];
                std::cout << "  Mesh[" << i << "]: " << mesh.name << " (" << mesh.vertices.size() << " vertices, " << mesh.indices.size() << " indices)" << std::endl;
            }
            
            for (size_t i = 0; i < asset->materials.size(); ++i) {
                const auto& material = asset->materials[i];
                std::cout << "  Material[" << i << "]: " << material.name << " (textures: " << material.textureSlots.size() << ")" << std::endl;
            }
        } else {
            std::cout << "✗ Failed to load asset" << std::endl;
        }
    }
    
    std::cout << "\n\nTesting Engine Integration..." << std::endl;
    std::cout << "=============================" << std::endl;
    
    try {
        Engine engine;
        
        if (engine.Initialize()) {
            std::cout << "✓ Engine initialized successfully" << std::endl;
            
            auto* graphics = engine.GetGraphics();
            if (graphics) {
                std::cout << "✓ Graphics device available" << std::endl;
                
                // Test loading textures through the graphics device
                std::cout << "\nTesting DirectX 11 texture creation..." << std::endl;
                
                std::vector<std::string> testFiles = {
                    "test.dds", "test.tga", "test.bmp", "test.png"
                };
                
                for (const auto& file : testFiles) {
                    std::cout << "Creating DirectX texture from " << file << "..." << std::endl;
                    auto texture = graphics->LoadTexture(file);
                    if (texture) {
                        std::cout << "✓ DirectX 11 texture created successfully" << std::endl;
                        texture->Release(); // Clean up
                    } else {
                        std::cout << "✗ Failed to create DirectX 11 texture" << std::endl;
                    }
                }
                
                // Test asset loading
                std::cout << "\nTesting asset loading..." << std::endl;
                
                std::vector<std::string> assetFiles = {
                    "Character.uasset", "Weapon.fbx", "Building.obj"
                };
                
                for (const auto& file : assetFiles) {
                    std::cout << "Loading asset " << file << "..." << std::endl;
                    bool success = false;
                    
                    if (ends_with(file, ".uasset")) {
                        success = graphics->LoadUnrealAsset(file);
                    } else if (ends_with(file, ".fbx")) {
                        success = graphics->LoadFBX(file);
                    } else if (ends_with(file, ".obj")) {
                        success = graphics->LoadOBJ(file);
                    }
                    
                    if (success) {
                        std::cout << "✓ Asset loaded successfully" << std::endl;
                    } else {
                        std::cout << "✗ Failed to load asset" << std::endl;
                    }
                }
            }
            
            engine.Shutdown();
            std::cout << "✓ Engine shutdown complete" << std::endl;
        } else {
            std::cout << "✗ Failed to initialize engine" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "✗ Exception: " << e.what() << std::endl;
    }
    
    std::cout << "\n\nTexture and Asset Loading Test Complete!" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "\nSupported formats:" << std::endl;
    std::cout << "Textures: .dds, .tga, .bmp, .png, .jpg, .hdr, .exr, .uasset, .umap" << std::endl;
    std::cout << "Assets: .uasset, .umap, .fbx, .obj, .dae" << std::endl;
    std::cout << "\nNote: All loaders are functional with placeholder implementations." << std::endl;
    std::cout << "Real file parsing can be added by implementing the respective format parsers." << std::endl;
    
    return 0;
}
