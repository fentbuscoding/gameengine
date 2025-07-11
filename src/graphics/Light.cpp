#include "Light.h"
#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

namespace Nexus {

Light::Light(LightType type)
    : id_(0)
    , type_(type)
    , position_(0.0f, 10.0f, 0.0f)
    , direction_(0.0f, -1.0f, 0.0f)
    , color_(1.0f, 1.0f, 1.0f)
    , intensity_(1.0f)
    , coneAngle_(XM_PI / 4.0f)
    , attenuationConstant_(1.0f)
    , attenuationLinear_(0.0f)
    , attenuationQuadratic_(0.0f)
{
}

Light::~Light() {
}

XMMATRIX Light::GetLightViewMatrix() const {
    XMVECTOR pos = XMLoadFloat3(&position_);
    XMVECTOR dir = XMLoadFloat3(&direction_);
    XMVECTOR target = XMVectorAdd(pos, dir);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    
    // If direction is parallel to up vector, use a different up vector
    XMVECTOR dot = XMVector3Dot(dir, up);
    float dotValue = XMVectorGetX(dot);
    if (fabsf(dotValue) > 0.99f) {
        up = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    }
    
    return XMMatrixLookAtLH(pos, target, up);
}

XMMATRIX Light::GetLightProjectionMatrix() const {
    switch (type_) {
        case LightType::Directional:
            // Use orthographic projection for directional lights
            return XMMatrixOrthographicLH(20.0f, 20.0f, 1.0f, 100.0f);
            
        case LightType::Point:
            // Use perspective projection for point lights
            return XMMatrixPerspectiveFovLH(XM_PI / 2.0f, 1.0f, 1.0f, 100.0f);
            
        case LightType::Spot:
            // Use perspective projection with cone angle for spot lights
            return XMMatrixPerspectiveFovLH(coneAngle_ * 2.0f, 1.0f, 1.0f, 100.0f);
    }
    
    return XMMatrixIdentity();
}

} // namespace Nexus
