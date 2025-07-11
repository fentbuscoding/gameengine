#pragma once

#include "Platform.h"
#include <string>
#include <memory>

namespace Nexus {

/**
 * Enhanced texture class with normal mapping and filtering support
 */
class Texture {
public:
    Texture();
    ~Texture();

    // Loading
    bool LoadFromFile(const std::string& filename, ID3D11Device* device);
    bool LoadFromMemory(const void* data, size_t size, ID3D11Device* device);
    bool CreateRenderTarget(int width, int height, DXGI_FORMAT format, ID3D11Device* device);
    bool CreateDepthStencil(int width, int height, DXGI_FORMAT format, ID3D11Device* device);

    // Texture properties
    bool IsNormalMap() const { return isNormalMap_; }
    void SetIsNormalMap(bool value) { isNormalMap_ = value; }
    
    bool HasMipMaps() const { return hasMipMaps_; }
    void GenerateMipMaps();

    // Enhanced filtering
    void SetFilterMode(D3D11_FILTER minFilter, D3D11_FILTER magFilter, D3D11_FILTER mipFilter);
    void SetAnisotropicFiltering(UINT maxAnisotropy);

    // Access
    ID3D11Texture2D* GetTexture() const { return texture_; }
    ID3D11ShaderResourceView* GetShaderResourceView() const { return shaderResourceView_; }
    
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    DXGI_FORMAT GetFormat() const { return format_; }

    // Binding
    void Bind(ID3D11DeviceContext* context, UINT stage) const;
    void Unbind(ID3D11DeviceContext* context, UINT stage) const;

    // Memory usage
    size_t GetMemoryUsage() const { return width_ * height_ * 4; } // Assuming RGBA format

private:
    void DetectNormalMap();
    void SetupSamplerState(ID3D11DeviceContext* context, UINT stage) const;

    ID3D11Texture2D* texture_;
    ID3D11ShaderResourceView* shaderResourceView_;
    
    int width_;
    int height_;
    DXGI_FORMAT format_;
    
    bool isNormalMap_;
    bool hasMipMaps_;
    
    D3D11_FILTER minFilter_;
    D3D11_FILTER magFilter_;
    D3D11_FILTER mipFilter_;
    UINT maxAnisotropy_;
};

/**
 * Material class for managing multiple textures and properties
 */
class Material {
public:
    Material();
    ~Material();

    // Texture management
    void SetDiffuseTexture(std::shared_ptr<Texture> texture) { diffuseTexture_ = texture; }
    void SetNormalTexture(std::shared_ptr<Texture> texture) { normalTexture_ = texture; }
    void SetSpecularTexture(std::shared_ptr<Texture> texture) { specularTexture_ = texture; }
    void SetEmissiveTexture(std::shared_ptr<Texture> texture) { emissiveTexture_ = texture; }

    std::shared_ptr<Texture> GetDiffuseTexture() const { return diffuseTexture_; }
    std::shared_ptr<Texture> GetNormalTexture() const { return normalTexture_; }
    std::shared_ptr<Texture> GetSpecularTexture() const { return specularTexture_; }
    std::shared_ptr<Texture> GetEmissiveTexture() const { return emissiveTexture_; }

    // Material properties
    void SetAmbientColor(const XMFLOAT4& color) { ambientColor_ = color; }
    void SetDiffuseColor(const XMFLOAT4& color) { diffuseColor_ = color; }
    void SetSpecularColor(const XMFLOAT4& color) { specularColor_ = color; }
    void SetEmissiveColor(const XMFLOAT4& color) { emissiveColor_ = color; }
    void SetSpecularPower(float power) { specularPower_ = power; }

    const XMFLOAT4& GetAmbientColor() const { return ambientColor_; }
    const XMFLOAT4& GetDiffuseColor() const { return diffuseColor_; }
    const XMFLOAT4& GetSpecularColor() const { return specularColor_; }
    const XMFLOAT4& GetEmissiveColor() const { return emissiveColor_; }
    float GetSpecularPower() const { return specularPower_; }

    // Binding
    void Bind(ID3D11DeviceContext* context) const;
    void Unbind(ID3D11DeviceContext* context) const;

private:
    std::shared_ptr<Texture> diffuseTexture_;
    std::shared_ptr<Texture> normalTexture_;
    std::shared_ptr<Texture> specularTexture_;
    std::shared_ptr<Texture> emissiveTexture_;

    XMFLOAT4 ambientColor_;
    XMFLOAT4 diffuseColor_;
    XMFLOAT4 specularColor_;
    XMFLOAT4 emissiveColor_;
    float specularPower_;
};

} // namespace Nexus
