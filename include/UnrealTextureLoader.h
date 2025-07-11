#pragma once

#include "Platform.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Nexus {

// Texture formats supported by Unreal Engine
enum class TextureFormat {
    // Common formats
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    R8G8B8_UNORM,
    R8G8B8_SRGB,
    R16G16B16A16_FLOAT,
    R32G32B32A32_FLOAT,
    
    // Compressed formats
    DXT1,
    DXT3,
    DXT5,
    BC4,
    BC5,
    BC6H,
    BC7,
    
    // Unreal specific formats
    ASTC_4x4,
    ASTC_6x6,
    ASTC_8x8,
    ASTC_10x10,
    ASTC_12x12,
    ETC2_RGB,
    ETC2_RGBA,
    
    // HDR formats
    HDR_RGB16F,
    HDR_RGB32F,
    EXR,
    
    // Normal map formats
    NORMAL_MAP,
    
    UNKNOWN
};

// Texture metadata
struct TextureMetadata {
    int width = 0;
    int height = 0;
    int depth = 1;
    int mipLevels = 1;
    int arraySize = 1;
    TextureFormat format = TextureFormat::UNKNOWN;
    bool isCubemap = false;
    bool isVolumeTexture = false;
    bool hasAlpha = false;
    bool isSRGB = false;
    
    // Unreal specific metadata
    std::string compressionSettings;
    std::string textureGroup;
    float lodBias = 0.0f;
    int maxTextureSize = 0;
    bool powerOfTwo = false;
    bool padToPowerOfTwo = false;
    
    // File-specific metadata
    std::string originalFilename;
    std::string sourceFilePath;
    size_t fileSize = 0;
    uint32_t crc32 = 0;
};

// Texture data container
struct TextureData {
    std::vector<uint8_t> data;
    TextureMetadata metadata;
    std::vector<std::vector<uint8_t>> mipLevels;
    
    bool IsValid() const {
        return !data.empty() && metadata.width > 0 && metadata.height > 0;
    }
};

// Unreal Engine texture loader
class UnrealTextureLoader {
public:
    static std::unique_ptr<TextureData> LoadUnrealTexture(const std::string& filename);
    static std::unique_ptr<TextureData> LoadDDS(const std::string& filename);
    static std::unique_ptr<TextureData> LoadTGA(const std::string& filename);
    static std::unique_ptr<TextureData> LoadBMP(const std::string& filename);
    static std::unique_ptr<TextureData> LoadPNG(const std::string& filename);
    static std::unique_ptr<TextureData> LoadJPG(const std::string& filename);
    static std::unique_ptr<TextureData> LoadHDR(const std::string& filename);
    static std::unique_ptr<TextureData> LoadEXR(const std::string& filename);
    
    // Unreal asset parsing
    static std::unique_ptr<TextureData> LoadUasset(const std::string& filename);
    static std::unique_ptr<TextureData> LoadUmap(const std::string& filename);
    
    // Texture format conversion
    static std::unique_ptr<TextureData> ConvertFormat(const TextureData& source, TextureFormat targetFormat);
    static std::unique_ptr<TextureData> GenerateMipmaps(const TextureData& source);
    static std::unique_ptr<TextureData> DecompressTexture(const TextureData& source);
    
    // Utility functions
    static TextureFormat GetFormatFromExtension(const std::string& filename);
    static bool IsFormatSupported(TextureFormat format);
    static size_t GetFormatBytesPerPixel(TextureFormat format);
    static std::string GetFormatName(TextureFormat format);
    
private:
    // DDS parsing helpers
    static std::unique_ptr<TextureData> ParseDDSHeader(const std::vector<uint8_t>& data);
    static TextureFormat DDSFormatToTextureFormat(uint32_t fourCC, uint32_t flags);
    
    // TGA parsing helpers
    static std::unique_ptr<TextureData> ParseTGAHeader(const std::vector<uint8_t>& data);
    static void DecodeTGAPixels(const std::vector<uint8_t>& data, TextureData& texture);
    
    // BMP parsing helpers
    static std::unique_ptr<TextureData> ParseBMPHeader(const std::vector<uint8_t>& data);
    static void DecodeBMPPixels(const std::vector<uint8_t>& data, TextureData& texture);
    
    // Unreal asset parsing helpers
    static std::unique_ptr<TextureData> ParseUAssetHeader(const std::vector<uint8_t>& data);
    static std::unique_ptr<TextureData> ExtractTextureFromUAsset(const std::vector<uint8_t>& data);
    static std::map<std::string, std::string> ParseUAssetProperties(const std::vector<uint8_t>& data);
    
    // Compression/decompression helpers
    static std::vector<uint8_t> DecompressDXT1(const std::vector<uint8_t>& data, int width, int height);
    static std::vector<uint8_t> DecompressDXT3(const std::vector<uint8_t>& data, int width, int height);
    static std::vector<uint8_t> DecompressDXT5(const std::vector<uint8_t>& data, int width, int height);
    static std::vector<uint8_t> DecompressBC4(const std::vector<uint8_t>& data, int width, int height);
    static std::vector<uint8_t> DecompressBC5(const std::vector<uint8_t>& data, int width, int height);
    static std::vector<uint8_t> DecompressBC6H(const std::vector<uint8_t>& data, int width, int height);
    static std::vector<uint8_t> DecompressBC7(const std::vector<uint8_t>& data, int width, int height);
    
    // Mipmap generation
    static std::vector<uint8_t> GenerateMipLevel(const std::vector<uint8_t>& data, int width, int height, int bytesPerPixel);
    static std::vector<uint8_t> BoxFilter(const std::vector<uint8_t>& data, int width, int height, int bytesPerPixel);
    static std::vector<uint8_t> LanczoFilter(const std::vector<uint8_t>& data, int width, int height, int bytesPerPixel);
    
    // Color space conversion
    static std::vector<uint8_t> ConvertToSRGB(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> ConvertFromSRGB(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> ConvertToLinear(const std::vector<uint8_t>& data);
    
    // File I/O helpers
    static std::vector<uint8_t> ReadFile(const std::string& filename);
    static bool WriteFile(const std::string& filename, const std::vector<uint8_t>& data);
    
    // Validation helpers
    static bool ValidateTextureData(const TextureData& texture);
    static bool ValidateDDSHeader(const std::vector<uint8_t>& data);
    static bool ValidateTGAHeader(const std::vector<uint8_t>& data);
    static bool ValidateBMPHeader(const std::vector<uint8_t>& data);
    static bool ValidateUAssetHeader(const std::vector<uint8_t>& data);
    
    // Error handling
    static void LogError(const std::string& message);
    static void LogWarning(const std::string& message);
    static void LogInfo(const std::string& message);
};

// Unreal asset loader for meshes and materials
class UnrealAssetLoader {
public:
    struct UnrealMaterial {
        std::string name;
        std::map<std::string, std::string> textureSlots;
        std::map<std::string, float> floatParameters;
        std::map<std::string, DirectX::XMFLOAT3> vectorParameters;
        std::map<std::string, DirectX::XMFLOAT4> colorParameters;
        std::string shaderModel;
        bool isTwoSided = false;
        bool isTranslucent = false;
        float roughness = 0.5f;
        float metallic = 0.0f;
        float specular = 0.5f;
        DirectX::XMFLOAT3 emissive = {0.0f, 0.0f, 0.0f};
    };
    
    struct UnrealMesh {
        std::string name;
        std::vector<DirectX::XMFLOAT3> vertices;
        std::vector<DirectX::XMFLOAT3> normals;
        std::vector<DirectX::XMFLOAT2> uvs;
        std::vector<DirectX::XMFLOAT3> tangents;
        std::vector<DirectX::XMFLOAT3> bitangents;
        std::vector<uint32_t> indices;
        std::vector<UnrealMaterial> materials;
        std::vector<int> materialIndices;
        DirectX::XMFLOAT3 boundingBoxMin;
        DirectX::XMFLOAT3 boundingBoxMax;
        int lodCount = 1;
    };
    
    struct UnrealAsset {
        std::string filename;
        std::string assetType;
        std::vector<UnrealMesh> meshes;
        std::vector<UnrealMaterial> materials;
        std::vector<std::string> textureReferences;
        std::map<std::string, std::string> metadata;
        bool isValid = false;
    };
    
    // Asset loading functions
    static std::unique_ptr<UnrealAsset> LoadUAsset(const std::string& filename);
    static std::unique_ptr<UnrealAsset> LoadUMap(const std::string& filename);
    static std::unique_ptr<UnrealAsset> LoadFBX(const std::string& filename);
    static std::unique_ptr<UnrealAsset> LoadOBJ(const std::string& filename);
    static std::unique_ptr<UnrealAsset> LoadDAE(const std::string& filename);
    
    // Asset validation
    static bool ValidateAsset(const UnrealAsset& asset);
    static bool ValidateMesh(const UnrealMesh& mesh);
    static bool ValidateMaterial(const UnrealMaterial& material);
    
    // Asset conversion
    static std::unique_ptr<UnrealAsset> ConvertToUnrealAsset(const UnrealAsset& source);
    static std::unique_ptr<UnrealMesh> OptimizeMesh(const UnrealMesh& source);
    static std::unique_ptr<UnrealMaterial> ConvertMaterial(const UnrealMaterial& source);
    
private:
    // UAsset parsing helpers
    static std::unique_ptr<UnrealAsset> ParseUAssetFile(const std::vector<uint8_t>& data);
    static std::unique_ptr<UnrealMesh> ExtractMeshFromUAsset(const std::vector<uint8_t>& data);
    static std::unique_ptr<UnrealMaterial> ExtractMaterialFromUAsset(const std::vector<uint8_t>& data);
    
    // FBX parsing helpers
    static std::unique_ptr<UnrealAsset> ParseFBXFile(const std::vector<uint8_t>& data);
    static std::unique_ptr<UnrealMesh> ExtractMeshFromFBX(const std::vector<uint8_t>& data);
    static std::unique_ptr<UnrealMaterial> ExtractMaterialFromFBX(const std::vector<uint8_t>& data);
    
    // OBJ parsing helpers
    static std::unique_ptr<UnrealAsset> ParseOBJFile(const std::vector<uint8_t>& data);
    static std::unique_ptr<UnrealMesh> ExtractMeshFromOBJ(const std::vector<uint8_t>& data);
    static std::unique_ptr<UnrealMaterial> ParseMTLFile(const std::string& filename);
    
    // Mesh optimization
    static void OptimizeVertices(UnrealMesh& mesh);
    static void OptimizeIndices(UnrealMesh& mesh);
    static void GenerateNormals(UnrealMesh& mesh);
    static void GenerateTangents(UnrealMesh& mesh);
    static void CalculateBoundingBox(UnrealMesh& mesh);
    
    // Material helpers
    static void ResolveMaterialTextures(UnrealMaterial& material, const std::string& basePath);
    static void ValidateMaterialParameters(UnrealMaterial& material);
    
    // Utility functions
    static std::vector<uint8_t> ReadFile(const std::string& filename);
    static std::string GetFileExtension(const std::string& filename);
    static std::string GetBasePath(const std::string& filename);
    static void LogError(const std::string& message);
    static void LogWarning(const std::string& message);
    static void LogInfo(const std::string& message);
};

} // namespace Nexus
