#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

namespace Nexus {

enum class LightType {
    Directional,
    Point,
    Spot
};

class Light {
public:
    Light(LightType type = LightType::Directional);
    ~Light();

    // Type
    LightType GetType() const { return type_; }
    void SetType(LightType type) { type_ = type; }

    // Properties
    void SetPosition(const XMFLOAT3& position) { position_ = position; }
    void SetDirection(const XMFLOAT3& direction) { direction_ = direction; }
    void SetColor(const XMFLOAT3& color) { color_ = color; }
    void SetIntensity(float intensity) { intensity_ = intensity; }

    const XMFLOAT3& GetPosition() const { return position_; }
    const XMFLOAT3& GetDirection() const { return direction_; }
    const XMFLOAT3& GetColor() const { return color_; }
    float GetIntensity() const { return intensity_; }

    // Spotlight specific
    void SetConeAngle(float angle) { coneAngle_ = angle; }
    float GetConeAngle() const { return coneAngle_; }

    // Point light specific
    void SetAttenuation(float constant, float linear, float quadratic) {
        attenuationConstant_ = constant;
        attenuationLinear_ = linear;
        attenuationQuadratic_ = quadratic;
    }

    // Shadow mapping
    XMMATRIX GetLightViewMatrix() const;
    XMMATRIX GetLightProjectionMatrix() const;

    // ID management
    int GetId() const { return id_; }
    void SetId(int id) { id_ = id; }

private:
    int id_;
    LightType type_;
    XMFLOAT3 position_;
    XMFLOAT3 direction_;
    XMFLOAT3 color_;
    float intensity_;
    
    // Spotlight
    float coneAngle_;
    
    // Point light attenuation
    float attenuationConstant_;
    float attenuationLinear_;
    float attenuationQuadratic_;
};

} // namespace Nexus
