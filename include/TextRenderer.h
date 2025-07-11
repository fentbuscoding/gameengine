#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

namespace Nexus {

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();
    
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();
    
    void RenderText(const std::string& text, float x, float y, float scale = 1.0f, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    
private:
    bool CreateBitmapFont();
    
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    ID3D11ShaderResourceView* fontTexture_;
    bool initialized_;
};

}