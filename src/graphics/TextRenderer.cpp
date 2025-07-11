#include "TextRenderer.h"
#include "Logger.h"
#include <d3d11.h>
#include <DirectXMath.h>

namespace Nexus {

TextRenderer::TextRenderer() : device_(nullptr), context_(nullptr), initialized_(false) {}

TextRenderer::~TextRenderer() {
    Shutdown();
}

bool TextRenderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    device_ = device;
    context_ = context;
    
    // Create a simple bitmap font (for demo purposes)
    if (!CreateBitmapFont()) {
        Logger::Error("Failed to create bitmap font");
        return false;
    }
    
    initialized_ = true;
    Logger::Info("Text renderer initialized successfully");
    return true;
}

void TextRenderer::Shutdown() {
    if (fontTexture_) {
        fontTexture_->Release();
        fontTexture_ = nullptr;
    }
    initialized_ = false;
}

void TextRenderer::RenderText(const std::string& text, float x, float y, float scale, const DirectX::XMFLOAT4& color) {
    if (!initialized_) return;
    
    // Simple text rendering - for now just log the text
    Logger::Info("Rendering text: " + text + " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
}

bool TextRenderer::CreateBitmapFont() {
    // Create a simple 8x8 pixel font texture (placeholder)
    const int width = 128;
    const int height = 128;
    const int channels = 4;
    
    std::vector<unsigned char> fontData(width * height * channels, 255);
    
    // Create texture
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;
    
    D3D11_SUBRESOURCE_DATA textureData = {};
    textureData.pSysMem = fontData.data();
    textureData.SysMemPitch = width * channels;
    textureData.SysMemSlicePitch = 0;
    
    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device_->CreateTexture2D(&textureDesc, &textureData, &texture);
    if (FAILED(hr)) {
        Logger::Error("Failed to create font texture");
        return false;
    }
    
    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    
    hr = device_->CreateShaderResourceView(texture, &srvDesc, &fontTexture_);
    texture->Release();
    
    if (FAILED(hr)) {
        Logger::Error("Failed to create font shader resource view");
        return false;
    }
    
    return true;
}

}