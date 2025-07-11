#include "Texture.h"
#include "Logger.h"

namespace Nexus {

// Texture implementation
Texture::Texture()
    : texture_(nullptr)
    , shaderResourceView_(nullptr)
    , width_(0)
    , height_(0)
    , format_(DXGI_FORMAT_UNKNOWN)
    , isNormalMap_(false)
    , hasMipMaps_(false)
    , minFilter_(D3D11_FILTER_MIN_MAG_MIP_LINEAR)
    , magFilter_(D3D11_FILTER_MIN_MAG_MIP_LINEAR)
    , mipFilter_(D3D11_FILTER_MIN_MAG_MIP_LINEAR)
    , maxAnisotropy_(1)
{
}

Texture::~Texture() {
    if (shaderResourceView_) { shaderResourceView_->Release(); shaderResourceView_ = nullptr; }
    if (texture_) { texture_->Release(); texture_ = nullptr; }
}

bool Texture::LoadFromFile(const std::string& filename, ID3D11Device* device) {
    if (!device) return false;
    
    Logger::Info("Loading texture: " + filename);
    
    // For now, create a simple placeholder texture
    // In a real implementation, you'd use DirectXTex or similar library
    width_ = 256;
    height_ = 256;
    format_ = DXGI_FORMAT_R8G8B8A8_UNORM;
    
    // Create a simple 2D texture
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width_;
    textureDesc.Height = height_;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = format_;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    
    // Create simple checkerboard pattern
    std::vector<uint32_t> pixels(width_ * height_);
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            bool checker = ((x / 32) % 2) ^ ((y / 32) % 2);
            pixels[y * width_ + x] = checker ? 0xFFFFFFFF : 0xFF808080;
        }
    }
    
    D3D11_SUBRESOURCE_DATA textureData = {};
    textureData.pSysMem = pixels.data();
    textureData.SysMemPitch = width_ * 4;
    
    HRESULT hr = device->CreateTexture2D(&textureDesc, &textureData, &texture_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create texture: " + filename);
        return false;
    }
    
    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    
    hr = device->CreateShaderResourceView(texture_, &srvDesc, &shaderResourceView_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create shader resource view: " + filename);
        return false;
    }
    
    // Auto-detect normal maps
    DetectNormalMap();
    
    Logger::Info("Texture loaded successfully: " + std::to_string(width_) + "x" + std::to_string(height_));
    return true;
}

bool Texture::CreateRenderTarget(int width, int height, DXGI_FORMAT format, ID3D11Device* device) {
    if (!device) return false;
    
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    
    HRESULT hr = device->CreateTexture2D(&textureDesc, nullptr, &texture_);
    if (SUCCEEDED(hr)) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        
        hr = device->CreateShaderResourceView(texture_, &srvDesc, &shaderResourceView_);
        if (SUCCEEDED(hr)) {
            width_ = width;
            height_ = height;
            format_ = format;
            return true;
        }
    }
    
    return false;
}

void Texture::DetectNormalMap() {
    // Simple heuristic: check if filename contains "normal" or "norm"
    // A more sophisticated approach would analyze the texture data
    isNormalMap_ = false; // For now
}

void Texture::Bind(ID3D11DeviceContext* context, UINT slot) const {
    if (context && shaderResourceView_) {
        context->PSSetShaderResources(slot, 1, &shaderResourceView_);
    }
}

void Texture::SetupSamplerState(ID3D11DeviceContext* context, UINT slot) const {
    // In DirectX 11, sampler states are handled differently
    // They would be set via context->PSSetSamplers() with a sampler state object
    // For now, this is a stub implementation
}

// Material implementation
Material::Material()
    : ambientColor_(0.2f, 0.2f, 0.2f, 1.0f)
    , diffuseColor_(1.0f, 1.0f, 1.0f, 1.0f)
    , specularColor_(1.0f, 1.0f, 1.0f, 1.0f)
    , emissiveColor_(0.0f, 0.0f, 0.0f, 1.0f)
    , specularPower_(32.0f)
{
}

Material::~Material() {
}

void Material::Bind(ID3D11DeviceContext* context) const {
    if (!context) return;
    
    // Bind textures
    if (diffuseTexture_) diffuseTexture_->Bind(context, 0);
    if (normalTexture_) normalTexture_->Bind(context, 1);
    if (specularTexture_) specularTexture_->Bind(context, 2);
    if (emissiveTexture_) emissiveTexture_->Bind(context, 3);
}

void Material::Unbind(ID3D11DeviceContext* context) const {
    if (!context) return;
    
    ID3D11ShaderResourceView* nullSRV[4] = { nullptr, nullptr, nullptr, nullptr };
    context->PSSetShaderResources(0, 4, nullSRV);
}

} // namespace Nexus
