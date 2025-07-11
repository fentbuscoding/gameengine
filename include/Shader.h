#pragma once

#include "Platform.h"
#include <string>
#include <map>
#include <memory>

namespace Nexus {

/**
 * HLSL Shader wrapper for DirectX 11
 */
class Shader {
public:
    Shader();
    ~Shader();

    // Loading
    bool LoadFromFile(const std::string& vertexShaderFile, 
                     const std::string& pixelShaderFile, 
                     ID3D11Device* device);
    bool LoadFromSource(const std::string& vertexShaderSource,
                       const std::string& pixelShaderSource,
                       ID3D11Device* device);

    // Binding
    void Bind(ID3D11DeviceContext* deviceContext);
    void Unbind(ID3D11DeviceContext* deviceContext);

    // Parameter setting
    void SetMatrix(const std::string& name, const XMMATRIX& matrix);
    void SetVector(const std::string& name, const XMFLOAT4& vector);
    void SetFloat(const std::string& name, float value);
    void SetInt(const std::string& name, int value);
    void SetBool(const std::string& name, bool value);
    void SetTexture(const std::string& name, ID3D11ShaderResourceView* texture);

    // Common matrices (automatically set by graphics device)
    void SetWorldMatrix(const XMMATRIX& world) { SetMatrix("worldMatrix", world); }
    void SetViewMatrix(const XMMATRIX& view) { SetMatrix("viewMatrix", view); }
    void SetProjectionMatrix(const XMMATRIX& projection) { SetMatrix("projectionMatrix", projection); }
    void SetWorldViewProjectionMatrix(const XMMATRIX& wvp) { SetMatrix("worldViewProjectionMatrix", wvp); }

    // Lighting parameters
    void SetLightDirection(const XMFLOAT3& direction);
    void SetLightColor(const XMFLOAT3& color);
    void SetLightPosition(const XMFLOAT3& position);
    void SetEyePosition(const XMFLOAT3& position);

    // Normal mapping parameters
    void SetNormalMapEnabled(bool enabled) { SetBool("normalMapEnabled", enabled); }
    void SetNormalMapStrength(float strength) { SetFloat("normalMapStrength", strength); }

    // Post-processing parameters
    void SetBloomThreshold(float threshold) { SetFloat("bloomThreshold", threshold); }
    void SetBloomIntensity(float intensity) { SetFloat("bloomIntensity", intensity); }
    void SetHeatHazeStrength(float strength) { SetFloat("heatHazeStrength", strength); }
    void SetHeatHazeSpeed(float speed) { SetFloat("heatHazeSpeed", speed); }

    // Shadow mapping parameters
    void SetShadowMapEnabled(bool enabled) { SetBool("shadowMapEnabled", enabled); }
    void SetShadowMap(ID3D11ShaderResourceView* shadowMap) { SetTexture("shadowMap", shadowMap); }
    void SetLightViewProjectionMatrix(const XMMATRIX& lightVP) { SetMatrix("lightViewProjectionMatrix", lightVP); }

    bool IsValid() const { return vertexShader_ != nullptr && pixelShader_ != nullptr; }

private:
    bool CompileShader(const std::string& source, const std::string& target, ID3DBlob** shader);
    void CreateConstantBuffers(ID3D11Device* device);
    void UpdateConstantBuffer(ID3D11DeviceContext* deviceContext, const std::string& name, const void* data, size_t size);

    ID3D11VertexShader* vertexShader_;
    ID3D11PixelShader* pixelShader_;
    ID3D11InputLayout* inputLayout_;
    ID3D11Buffer* constantBuffer_;
    
    ID3D11Device* device_;
    std::map<std::string, size_t> constantBufferOffsets_;
    std::unique_ptr<char[]> constantBufferData_;
    size_t constantBufferSize_;
};

} // namespace Nexus
