#include "Shader.h"
#include "Logger.h"
#include <d3dcompiler.h>
#include <fstream>
#include <sstream>

namespace Nexus {

Shader::Shader()
    : vertexShader_(nullptr)
    , pixelShader_(nullptr)
    , inputLayout_(nullptr)
    , constantBuffer_(nullptr)
    , device_(nullptr)
    , constantBufferSize_(0)
{
}

Shader::~Shader() {
    if (constantBuffer_) { constantBuffer_->Release(); constantBuffer_ = nullptr; }
    if (inputLayout_) { inputLayout_->Release(); inputLayout_ = nullptr; }
    if (pixelShader_) { pixelShader_->Release(); pixelShader_ = nullptr; }
    if (vertexShader_) { vertexShader_->Release(); vertexShader_ = nullptr; }
}

bool Shader::LoadFromFile(const std::string& vertexShaderFile, 
                         const std::string& pixelShaderFile, 
                         ID3D11Device* device) {
    device_ = device;
    
    Logger::Info("Loading shaders: " + vertexShaderFile + ", " + pixelShaderFile);
    
    // Load shader source from files
    std::ifstream vsFile(vertexShaderFile);
    std::ifstream psFile(pixelShaderFile);
    
    if (!vsFile.is_open() || !psFile.is_open()) {
        Logger::Error("Failed to open shader files");
        return false;
    }
    
    std::stringstream vsStream, psStream;
    vsStream << vsFile.rdbuf();
    psStream << psFile.rdbuf();
    
    return LoadFromSource(vsStream.str(), psStream.str(), device);
}

bool Shader::LoadFromSource(const std::string& vertexShaderSource,
                           const std::string& pixelShaderSource,
                           ID3D11Device* device) {
    device_ = device;
    
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    
    // Compile vertex shader
    if (!CompileShader(vertexShaderSource, "vs_5_0", &vsBlob)) {
        Logger::Error("Failed to compile vertex shader");
        return false;
    }
    
    // Compile pixel shader
    if (!CompileShader(pixelShaderSource, "ps_5_0", &psBlob)) {
        Logger::Error("Failed to compile pixel shader");
        if (vsBlob) vsBlob->Release();
        return false;
    }
    
    // Create vertex shader
    HRESULT hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create vertex shader");
        vsBlob->Release();
        psBlob->Release();
        return false;
    }
    
    // Create pixel shader
    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create pixel shader");
        vsBlob->Release();
        psBlob->Release();
        return false;
    }
    
    // Create input layout (basic layout for now)
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    
    hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout_);
    
    vsBlob->Release();
    psBlob->Release();
    
    if (FAILED(hr)) {
        Logger::Error("Failed to create input layout");
        return false;
    }
    
    // Create constant buffers
    CreateConstantBuffers(device);
    
    Logger::Info("Shaders loaded successfully");
    return true;
}

void Shader::Bind(ID3D11DeviceContext* deviceContext) {
    if (!deviceContext) return;
    
    // Set shaders
    deviceContext->VSSetShader(vertexShader_, nullptr, 0);
    deviceContext->PSSetShader(pixelShader_, nullptr, 0);
    deviceContext->IASetInputLayout(inputLayout_);
    
    // Set constant buffers
    if (constantBuffer_) {
        deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer_);
        deviceContext->PSSetConstantBuffers(0, 1, &constantBuffer_);
    }
}

void Shader::Unbind(ID3D11DeviceContext* deviceContext) {
    if (!deviceContext) return;
    
    deviceContext->VSSetShader(nullptr, nullptr, 0);
    deviceContext->PSSetShader(nullptr, nullptr, 0);
    deviceContext->IASetInputLayout(nullptr);
}

void Shader::SetMatrix(const std::string& name, const XMMATRIX& matrix) {
    // Store matrix in constant buffer data
    // For now, this is a stub implementation
}

void Shader::SetVector(const std::string& name, const XMFLOAT4& vector) {
    // Store vector in constant buffer data
    // For now, this is a stub implementation
}

void Shader::SetFloat(const std::string& name, float value) {
    // Store float in constant buffer data
    // For now, this is a stub implementation
}

void Shader::SetInt(const std::string& name, int value) {
    // Store int in constant buffer data
    // For now, this is a stub implementation
}

void Shader::SetBool(const std::string& name, bool value) {
    // Store bool in constant buffer data
    // For now, this is a stub implementation
}

void Shader::SetTexture(const std::string& name, ID3D11ShaderResourceView* texture) {
    // Set texture - this would need device context
    // For now, this is a stub implementation
}

void Shader::SetLightDirection(const XMFLOAT3& direction) {
    SetVector("lightDirection", XMFLOAT4(direction.x, direction.y, direction.z, 0.0f));
}

void Shader::SetLightColor(const XMFLOAT3& color) {
    SetVector("lightColor", XMFLOAT4(color.x, color.y, color.z, 1.0f));
}

void Shader::SetLightPosition(const XMFLOAT3& position) {
    SetVector("lightPosition", XMFLOAT4(position.x, position.y, position.z, 1.0f));
}

void Shader::SetEyePosition(const XMFLOAT3& position) {
    SetVector("eyePosition", XMFLOAT4(position.x, position.y, position.z, 1.0f));
}

bool Shader::CompileShader(const std::string& source, const std::string& target, ID3DBlob** shader) {
    ID3DBlob* errorBlob = nullptr;
    
    HRESULT hr = D3DCompile(
        source.c_str(),
        source.length(),
        nullptr,
        nullptr,
        nullptr,
        "main",
        target.c_str(),
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        shader,
        &errorBlob
    );
    
    if (FAILED(hr)) {
        if (errorBlob) {
            Logger::Error("Shader compilation error: " + std::string(static_cast<char*>(errorBlob->GetBufferPointer())));
            errorBlob->Release();
        }
        return false;
    }
    
    if (errorBlob) {
        errorBlob->Release();
    }
    
    return true;
}

void Shader::CreateConstantBuffers(ID3D11Device* device) {
    // Create a basic constant buffer
    // This is a simplified implementation
    constantBufferSize_ = 1024; // 1KB buffer
    constantBufferData_ = std::make_unique<char[]>(constantBufferSize_);
    
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = constantBufferSize_;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    device->CreateBuffer(&bufferDesc, nullptr, &constantBuffer_);
}

void Shader::UpdateConstantBuffer(ID3D11DeviceContext* deviceContext, const std::string& name, const void* data, size_t size) {
    // Update constant buffer with new data
    // This is a stub implementation
}

} // namespace Nexus
