#include "UnrealTextureLoader.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iostream>

// Placeholder for STB image - we'll implement basic loading without it for now
namespace stb_image_placeholder {
    unsigned char* stbi_load(const char* filename, int* x, int* y, int* channels_in_file, int desired_channels) {
        // Return nullptr to indicate failure - will use placeholder instead
        return nullptr;
    }
    
    float* stbi_loadf(const char* filename, int* x, int* y, int* channels_in_file, int desired_channels) {
        // Return nullptr to indicate failure - will use placeholder instead
        return nullptr;
    }
    
    void stbi_image_free(void* retval_from_stbi_load) {
        // Nothing to free for placeholders
    }
    
    const char* stbi_failure_reason() {
        return "STB Image not available - using placeholder implementation";
    }
}

using namespace stb_image_placeholder;

namespace Nexus {

// Static utility functions
std::vector<uint8_t> UnrealTextureLoader::ReadFile(const std::string& filename) {
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
    
    LogInfo("Successfully read " + std::to_string(fileSize) + " bytes from " + filename);
    return data;
}

bool UnrealTextureLoader::WriteFile(const std::string& filename, const std::vector<uint8_t>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        LogError("Failed to create file: " + filename);
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
    
    LogInfo("Successfully wrote " + std::to_string(data.size()) + " bytes to " + filename);
    return true;
}

TextureFormat UnrealTextureLoader::GetFormatFromExtension(const std::string& filename) {
    std::string extension = filename.substr(filename.find_last_of('.') + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "dds") return TextureFormat::DXT5;
    if (extension == "tga") return TextureFormat::R8G8B8A8_UNORM;
    if (extension == "bmp") return TextureFormat::R8G8B8_UNORM;
    if (extension == "png") return TextureFormat::R8G8B8A8_UNORM;
    if (extension == "jpg" || extension == "jpeg") return TextureFormat::R8G8B8_UNORM;
    if (extension == "hdr") return TextureFormat::HDR_RGB32F;
    if (extension == "exr") return TextureFormat::EXR;
    if (extension == "uasset") return TextureFormat::DXT5;
    if (extension == "umap") return TextureFormat::DXT5;
    
    return TextureFormat::UNKNOWN;
}

bool UnrealTextureLoader::IsFormatSupported(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8G8B8A8_UNORM:
        case TextureFormat::R8G8B8A8_SRGB:
        case TextureFormat::R8G8B8_UNORM:
        case TextureFormat::R8G8B8_SRGB:
        case TextureFormat::DXT1:
        case TextureFormat::DXT3:
        case TextureFormat::DXT5:
        case TextureFormat::HDR_RGB32F:
        case TextureFormat::NORMAL_MAP:
            return true;
        default:
            return false;
    }
}

size_t UnrealTextureLoader::GetFormatBytesPerPixel(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8G8B8A8_UNORM:
        case TextureFormat::R8G8B8A8_SRGB:
            return 4;
        case TextureFormat::R8G8B8_UNORM:
        case TextureFormat::R8G8B8_SRGB:
            return 3;
        case TextureFormat::R16G16B16A16_FLOAT:
            return 8;
        case TextureFormat::R32G32B32A32_FLOAT:
            return 16;
        case TextureFormat::HDR_RGB32F:
            return 12;
        case TextureFormat::DXT1:
            return 0; // Compressed format
        case TextureFormat::DXT3:
        case TextureFormat::DXT5:
            return 0; // Compressed format
        default:
            return 0;
    }
}

std::string UnrealTextureLoader::GetFormatName(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8G8B8A8_UNORM: return "R8G8B8A8_UNORM";
        case TextureFormat::R8G8B8A8_SRGB: return "R8G8B8A8_SRGB";
        case TextureFormat::R8G8B8_UNORM: return "R8G8B8_UNORM";
        case TextureFormat::R8G8B8_SRGB: return "R8G8B8_SRGB";
        case TextureFormat::DXT1: return "DXT1";
        case TextureFormat::DXT3: return "DXT3";
        case TextureFormat::DXT5: return "DXT5";
        case TextureFormat::HDR_RGB32F: return "HDR_RGB32F";
        case TextureFormat::EXR: return "EXR";
        case TextureFormat::NORMAL_MAP: return "NORMAL_MAP";
        default: return "UNKNOWN";
    }
}

// Main texture loading functions
std::unique_ptr<TextureData> UnrealTextureLoader::LoadUnrealTexture(const std::string& filename) {
    LogInfo("Loading Unreal texture: " + filename);
    
    TextureFormat format = GetFormatFromExtension(filename);
    if (format == TextureFormat::UNKNOWN) {
        LogError("Unsupported texture format: " + filename);
        return nullptr;
    }
    
    std::string extension = filename.substr(filename.find_last_of('.') + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "dds") return LoadDDS(filename);
    if (extension == "tga") return LoadTGA(filename);
    if (extension == "bmp") return LoadBMP(filename);
    if (extension == "png") return LoadPNG(filename);
    if (extension == "jpg" || extension == "jpeg") return LoadJPG(filename);
    if (extension == "hdr") return LoadHDR(filename);
    if (extension == "exr") return LoadEXR(filename);
    if (extension == "uasset") return LoadUasset(filename);
    if (extension == "umap") return LoadUmap(filename);
    
    LogError("Unsupported texture extension: " + extension);
    return nullptr;
}

std::unique_ptr<TextureData> UnrealTextureLoader::LoadDDS(const std::string& filename) {
    LogInfo("Loading DDS texture: " + filename);
    
    std::vector<uint8_t> fileData = ReadFile(filename);
    if (fileData.empty()) {
        return nullptr;
    }
    
    // For now, create a placeholder checker pattern
    auto texture = std::make_unique<TextureData>();
    texture->metadata.width = 256;
    texture->metadata.height = 256;
    texture->metadata.format = TextureFormat::DXT5;
    texture->metadata.originalFilename = filename;
    
    // Create a red and white checkerboard pattern
    int size = texture->metadata.width * texture->metadata.height * 4;
    texture->data.resize(size);
    
    for (int y = 0; y < texture->metadata.height; ++y) {
        for (int x = 0; x < texture->metadata.width; ++x) {
            int idx = (y * texture->metadata.width + x) * 4;
            bool checker = ((x / 32) + (y / 32)) % 2 == 0;
            
            if (checker) {
                texture->data[idx] = 255;     // R
                texture->data[idx + 1] = 0;   // G
                texture->data[idx + 2] = 0;   // B
                texture->data[idx + 3] = 255; // A
            } else {
                texture->data[idx] = 255;     // R
                texture->data[idx + 1] = 255; // G
                texture->data[idx + 2] = 255; // B
                texture->data[idx + 3] = 255; // A
            }
        }
    }
    
    LogInfo("Created DDS placeholder texture: " + std::to_string(texture->metadata.width) + "x" + std::to_string(texture->metadata.height));
    return texture;
}

std::unique_ptr<TextureData> UnrealTextureLoader::LoadTGA(const std::string& filename) {
    LogInfo("Loading TGA texture: " + filename);
    
    std::vector<uint8_t> fileData = ReadFile(filename);
    if (fileData.empty()) {
        return nullptr;
    }
    
    // For now, create a placeholder green pattern
    auto texture = std::make_unique<TextureData>();
    texture->metadata.width = 256;
    texture->metadata.height = 256;
    texture->metadata.format = TextureFormat::R8G8B8A8_UNORM;
    texture->metadata.originalFilename = filename;
    
    // Create a green gradient pattern
    int size = texture->metadata.width * texture->metadata.height * 4;
    texture->data.resize(size);
    
    for (int y = 0; y < texture->metadata.height; ++y) {
        for (int x = 0; x < texture->metadata.width; ++x) {
            int idx = (y * texture->metadata.width + x) * 4;
            
            texture->data[idx] = 0;                              // R
            texture->data[idx + 1] = (x * 255) / texture->metadata.width; // G
            texture->data[idx + 2] = (y * 255) / texture->metadata.height; // B
            texture->data[idx + 3] = 255;                        // A
        }
    }
    
    LogInfo("Created TGA placeholder texture: " + std::to_string(texture->metadata.width) + "x" + std::to_string(texture->metadata.height));
    return texture;
}

std::unique_ptr<TextureData> UnrealTextureLoader::LoadBMP(const std::string& filename) {
    LogInfo("Loading BMP texture: " + filename);
    
    std::vector<uint8_t> fileData = ReadFile(filename);
    if (fileData.empty()) {
        return nullptr;
    }
    
    // For now, create a placeholder blue pattern
    auto texture = std::make_unique<TextureData>();
    texture->metadata.width = 256;
    texture->metadata.height = 256;
    texture->metadata.format = TextureFormat::R8G8B8_UNORM;
    texture->metadata.originalFilename = filename;
    
    // Create a blue radial pattern
    int size = texture->metadata.width * texture->metadata.height * 3;
    texture->data.resize(size);
    
    int centerX = texture->metadata.width / 2;
    int centerY = texture->metadata.height / 2;
    float maxDist = std::sqrt(centerX * centerX + centerY * centerY);
    
    for (int y = 0; y < texture->metadata.height; ++y) {
        for (int x = 0; x < texture->metadata.width; ++x) {
            int idx = (y * texture->metadata.width + x) * 3;
            
            float dx = x - centerX;
            float dy = y - centerY;
            float dist = std::sqrt(dx * dx + dy * dy);
            float intensity = 1.0f - (dist / maxDist);
            
            texture->data[idx] = (uint8_t)(intensity * 50);      // R
            texture->data[idx + 1] = (uint8_t)(intensity * 100); // G
            texture->data[idx + 2] = (uint8_t)(intensity * 255); // B
        }
    }
    
    LogInfo("Created BMP placeholder texture: " + std::to_string(texture->metadata.width) + "x" + std::to_string(texture->metadata.height));
    return texture;
}

std::unique_ptr<TextureData> UnrealTextureLoader::LoadPNG(const std::string& filename) {
    LogInfo("Loading PNG texture: " + filename);
    
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        LogWarning("Failed to load PNG with STB Image: " + filename + " - " + stbi_failure_reason());
        LogInfo("Creating placeholder PNG texture instead");
        
        // Create a placeholder purple texture
        auto texture = std::make_unique<TextureData>();
        texture->metadata.width = 256;
        texture->metadata.height = 256;
        texture->metadata.format = TextureFormat::R8G8B8A8_UNORM;
        texture->metadata.hasAlpha = true;
        texture->metadata.originalFilename = filename;
        
        int size = texture->metadata.width * texture->metadata.height * 4;
        texture->data.resize(size);
        
        for (int y = 0; y < texture->metadata.height; ++y) {
            for (int x = 0; x < texture->metadata.width; ++x) {
                int idx = (y * texture->metadata.width + x) * 4;
                
                // Create a purple checkered pattern
                bool checker = ((x / 16) + (y / 16)) % 2 == 0;
                
                if (checker) {
                    texture->data[idx] = 128;     // R
                    texture->data[idx + 1] = 0;   // G
                    texture->data[idx + 2] = 128; // B (Purple)
                    texture->data[idx + 3] = 255; // A
                } else {
                    texture->data[idx] = 64;      // R
                    texture->data[idx + 1] = 0;   // G
                    texture->data[idx + 2] = 64;  // B (Dark Purple)
                    texture->data[idx + 3] = 255; // A
                }
            }
        }
        
        LogInfo("Created PNG placeholder texture: " + std::to_string(texture->metadata.width) + "x" + std::to_string(texture->metadata.height));
        return texture;
    }
    
    auto texture = std::make_unique<TextureData>();
    texture->metadata.width = width;
    texture->metadata.height = height;
    texture->metadata.format = TextureFormat::R8G8B8A8_UNORM;
    texture->metadata.hasAlpha = (channels == 4);
    texture->metadata.originalFilename = filename;
    
    size_t dataSize = width * height * 4;
    texture->data.resize(dataSize);
    std::memcpy(texture->data.data(), data, dataSize);
    
    stbi_image_free(data);
    
    LogInfo("Loaded PNG texture: " + std::to_string(width) + "x" + std::to_string(height) + " (" + std::to_string(channels) + " channels)");
    return texture;
}

std::unique_ptr<TextureData> UnrealTextureLoader::LoadJPG(const std::string& filename) {
    LogInfo("Loading JPG texture: " + filename);
    
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 3);
    
    if (!data) {
        LogWarning("Failed to load JPG with STB Image: " + filename + " - " + stbi_failure_reason());
        LogInfo("Creating placeholder JPG texture instead");
        
        // Create a placeholder yellow texture
        auto texture = std::make_unique<TextureData>();
        texture->metadata.width = 256;
        texture->metadata.height = 256;
        texture->metadata.format = TextureFormat::R8G8B8_UNORM;
        texture->metadata.hasAlpha = false;
        texture->metadata.originalFilename = filename;
        
        int size = texture->metadata.width * texture->metadata.height * 3;
        texture->data.resize(size);
        
        for (int y = 0; y < texture->metadata.height; ++y) {
            for (int x = 0; x < texture->metadata.width; ++x) {
                int idx = (y * texture->metadata.width + x) * 3;
                
                // Create a yellow diagonal pattern
                bool diagonal = ((x + y) / 32) % 2 == 0;
                
                if (diagonal) {
                    texture->data[idx] = 255;     // R
                    texture->data[idx + 1] = 255; // G (Yellow)
                    texture->data[idx + 2] = 0;   // B
                } else {
                    texture->data[idx] = 200;     // R
                    texture->data[idx + 1] = 200; // G (Light Yellow)
                    texture->data[idx + 2] = 0;   // B
                }
            }
        }
        
        LogInfo("Created JPG placeholder texture: " + std::to_string(texture->metadata.width) + "x" + std::to_string(texture->metadata.height));
        return texture;
    }
    
    auto texture = std::make_unique<TextureData>();
    texture->metadata.width = width;
    texture->metadata.height = height;
    texture->metadata.format = TextureFormat::R8G8B8_UNORM;
    texture->metadata.hasAlpha = false;
    texture->metadata.originalFilename = filename;
    
    size_t dataSize = width * height * 3;
    texture->data.resize(dataSize);
    std::memcpy(texture->data.data(), data, dataSize);
    
    stbi_image_free(data);
    
    LogInfo("Loaded JPG texture: " + std::to_string(width) + "x" + std::to_string(height) + " (" + std::to_string(channels) + " channels)");
    return texture;
}

std::unique_ptr<TextureData> UnrealTextureLoader::LoadHDR(const std::string& filename) {
    LogInfo("Loading HDR texture: " + filename);
    
    int width, height, channels;
    float* data = stbi_loadf(filename.c_str(), &width, &height, &channels, 3);
    
    if (!data) {
        LogWarning("Failed to load HDR with STB Image: " + filename + " - " + stbi_failure_reason());
        LogInfo("Creating placeholder HDR texture instead");
        
        // Create a placeholder HDR-like texture with bright values
        auto texture = std::make_unique<TextureData>();
        texture->metadata.width = 512;
        texture->metadata.height = 512;
        texture->metadata.format = TextureFormat::HDR_RGB32F;
        texture->metadata.hasAlpha = false;
        texture->metadata.originalFilename = filename;
        
        int size = texture->metadata.width * texture->metadata.height * 4;
        texture->data.resize(size);
        
        for (int y = 0; y < texture->metadata.height; ++y) {
            for (int x = 0; x < texture->metadata.width; ++x) {
                int idx = (y * texture->metadata.width + x) * 4;
                
                // Create a bright HDR sky-like pattern
                float fx = (float)x / texture->metadata.width;
                float fy = (float)y / texture->metadata.height;
                float intensity = 1.0f + sin(fx * 3.14159f) * cos(fy * 3.14159f) * 2.0f;
                
                texture->data[idx] = (uint8_t)(std::min(255.0f, intensity * 200.0f));     // R
                texture->data[idx + 1] = (uint8_t)(std::min(255.0f, intensity * 220.0f)); // G
                texture->data[idx + 2] = (uint8_t)(std::min(255.0f, intensity * 255.0f)); // B
                texture->data[idx + 3] = 255;                                             // A
            }
        }
        
        LogInfo("Created HDR placeholder texture: " + std::to_string(texture->metadata.width) + "x" + std::to_string(texture->metadata.height));
        return texture;
    }
    
    auto texture = std::make_unique<TextureData>();
    texture->metadata.width = width;
    texture->metadata.height = height;
    texture->metadata.format = TextureFormat::HDR_RGB32F;
    texture->metadata.hasAlpha = false;
    texture->metadata.originalFilename = filename;
    
    size_t dataSize = width * height * 3 * sizeof(float);
    texture->data.resize(dataSize);
    std::memcpy(texture->data.data(), data, dataSize);
    
    stbi_image_free(data);
    
    LogInfo("Loaded HDR texture: " + std::to_string(width) + "x" + std::to_string(height) + " (" + std::to_string(channels) + " channels)");
    return texture;
}

std::unique_ptr<TextureData> UnrealTextureLoader::LoadEXR(const std::string& filename) {
    LogInfo("Loading EXR texture: " + filename);
    
    // For now, create a placeholder HDR-like texture
    auto texture = std::make_unique<TextureData>();
    texture->metadata.width = 512;
    texture->metadata.height = 512;
    texture->metadata.format = TextureFormat::EXR;
    texture->metadata.originalFilename = filename;
    
    // Create a bright gradient pattern
    int size = texture->metadata.width * texture->metadata.height * 4;
    texture->data.resize(size);
    
    for (int y = 0; y < texture->metadata.height; ++y) {
        for (int x = 0; x < texture->metadata.width; ++x) {
            int idx = (y * texture->metadata.width + x) * 4;
            
            float fx = (float)x / texture->metadata.width;
            float fy = (float)y / texture->metadata.height;
            
            // Create a bright HDR-like pattern
            texture->data[idx] = (uint8_t)(fx * 255);           // R
            texture->data[idx + 1] = (uint8_t)(fy * 255);       // G
            texture->data[idx + 2] = (uint8_t)((fx + fy) * 127); // B
            texture->data[idx + 3] = 255;                       // A
        }
    }
    
    LogInfo("Created EXR placeholder texture: " + std::to_string(texture->metadata.width) + "x" + std::to_string(texture->metadata.height));
    return texture;
}

std::unique_ptr<TextureData> UnrealTextureLoader::LoadUasset(const std::string& filename) {
    LogInfo("Loading Unreal Asset (.uasset): " + filename);
    
    std::vector<uint8_t> fileData = ReadFile(filename);
    if (fileData.empty()) {
        return nullptr;
    }
    
    // For now, create a placeholder with Unreal Engine colors
    auto texture = std::make_unique<TextureData>();
    texture->metadata.width = 512;
    texture->metadata.height = 512;
    texture->metadata.format = TextureFormat::DXT5;
    texture->metadata.originalFilename = filename;
    texture->metadata.compressionSettings = "TC_Default";
    texture->metadata.textureGroup = "TEXTUREGROUP_World";
    
    // Create an Unreal Engine logo-like pattern
    int size = texture->metadata.width * texture->metadata.height * 4;
    texture->data.resize(size);
    
    for (int y = 0; y < texture->metadata.height; ++y) {
        for (int x = 0; x < texture->metadata.width; ++x) {
            int idx = (y * texture->metadata.width + x) * 4;
            
            // Create a blue/orange pattern reminiscent of Unreal Engine
            bool isBlue = (x + y) % 64 < 32;
            
            if (isBlue) {
                texture->data[idx] = 0;       // R
                texture->data[idx + 1] = 122; // G
                texture->data[idx + 2] = 204; // B (Unreal Blue)
                texture->data[idx + 3] = 255; // A
            } else {
                texture->data[idx] = 255;     // R
                texture->data[idx + 1] = 140; // G
                texture->data[idx + 2] = 0;   // B (Unreal Orange)
                texture->data[idx + 3] = 255; // A
            }
        }
    }
    
    LogInfo("Created Unreal Asset placeholder texture: " + std::to_string(texture->metadata.width) + "x" + std::to_string(texture->metadata.height));
    return texture;
}

std::unique_ptr<TextureData> UnrealTextureLoader::LoadUmap(const std::string& filename) {
    LogInfo("Loading Unreal Map (.umap): " + filename);
    
    std::vector<uint8_t> fileData = ReadFile(filename);
    if (fileData.empty()) {
        return nullptr;
    }
    
    // For now, create a placeholder representing a map/level
    auto texture = std::make_unique<TextureData>();
    texture->metadata.width = 1024;
    texture->metadata.height = 1024;
    texture->metadata.format = TextureFormat::R8G8B8A8_UNORM;
    texture->metadata.originalFilename = filename;
    
    // Create a top-down map-like pattern
    int size = texture->metadata.width * texture->metadata.height * 4;
    texture->data.resize(size);
    
    for (int y = 0; y < texture->metadata.height; ++y) {
        for (int x = 0; x < texture->metadata.width; ++x) {
            int idx = (y * texture->metadata.width + x) * 4;
            
            // Create a terrain-like pattern
            float noise = sin(x * 0.1f) * cos(y * 0.1f) * 0.5f + 0.5f;
            uint8_t intensity = (uint8_t)(noise * 255);
            
            texture->data[idx] = intensity;     // R
            texture->data[idx + 1] = intensity; // G
            texture->data[idx + 2] = intensity; // B
            texture->data[idx + 3] = 255;       // A
        }
    }
    
    LogInfo("Created Unreal Map placeholder texture: " + std::to_string(texture->metadata.width) + "x" + std::to_string(texture->metadata.height));
    return texture;
}

// Utility and logging functions
void UnrealTextureLoader::LogError(const std::string& message) {
    Logger::Error("UnrealTextureLoader: " + message);
}

void UnrealTextureLoader::LogWarning(const std::string& message) {
    Logger::Warning("UnrealTextureLoader: " + message);
}

void UnrealTextureLoader::LogInfo(const std::string& message) {
    Logger::Info("UnrealTextureLoader: " + message);
}

bool UnrealTextureLoader::ValidateTextureData(const TextureData& texture) {
    if (texture.data.empty()) {
        LogError("Texture data is empty");
        return false;
    }
    
    if (texture.metadata.width <= 0 || texture.metadata.height <= 0) {
        LogError("Invalid texture dimensions");
        return false;
    }
    
    if (texture.metadata.format == TextureFormat::UNKNOWN) {
        LogError("Unknown texture format");
        return false;
    }
    
    return true;
}

// Placeholder implementations for advanced features
std::unique_ptr<TextureData> UnrealTextureLoader::ConvertFormat(const TextureData& source, TextureFormat targetFormat) {
    LogInfo("Converting texture format from " + GetFormatName(source.metadata.format) + " to " + GetFormatName(targetFormat));
    
    // For now, just copy the source data
    auto result = std::make_unique<TextureData>(source);
    result->metadata.format = targetFormat;
    
    return result;
}

std::unique_ptr<TextureData> UnrealTextureLoader::GenerateMipmaps(const TextureData& source) {
    LogInfo("Generating mipmaps for texture: " + std::to_string(source.metadata.width) + "x" + std::to_string(source.metadata.height));
    
    auto result = std::make_unique<TextureData>(source);
    
    // Generate a simple mipmap chain
    int mipWidth = source.metadata.width;
    int mipHeight = source.metadata.height;
    int mipLevel = 0;
    
    while (mipWidth > 1 || mipHeight > 1) {
        mipWidth = std::max(1, mipWidth / 2);
        mipHeight = std::max(1, mipHeight / 2);
        mipLevel++;
        
        // Create a simple downsampled version
        std::vector<uint8_t> mipData(mipWidth * mipHeight * 4);
        // Simple box filter downsampling would go here
        result->mipLevels.push_back(mipData);
    }
    
    result->metadata.mipLevels = mipLevel + 1;
    
    return result;
}

std::unique_ptr<TextureData> UnrealTextureLoader::DecompressTexture(const TextureData& source) {
    LogInfo("Decompressing texture format: " + GetFormatName(source.metadata.format));
    
    // For now, just return a copy
    auto result = std::make_unique<TextureData>(source);
    result->metadata.format = TextureFormat::R8G8B8A8_UNORM;
    
    return result;
}

} // namespace Nexus
