#include "Camera.h"
#include <cmath>

namespace Nexus {

Camera::Camera()
    : position_(0.0f, 0.0f, -5.0f)
    , target_(0.0f, 0.0f, 0.0f)
    , up_(0.0f, 1.0f, 0.0f)
    , pitch_(0.0f)
    , yaw_(0.0f)
{
    viewMatrix_ = DirectX::XMMatrixIdentity();
    projectionMatrix_ = DirectX::XMMatrixIdentity();
    UpdateViewMatrix();
}

Camera::~Camera() {
}

void Camera::SetPerspective(float fovY, float aspectRatio, float nearPlane, float farPlane) {
    projectionMatrix_ = DirectX::XMMatrixPerspectiveFovLH(fovY, aspectRatio, nearPlane, farPlane);
}

void Camera::SetOrthographic(float width, float height, float nearPlane, float farPlane) {
    projectionMatrix_ = DirectX::XMMatrixOrthographicLH(width, height, nearPlane, farPlane);
}

void Camera::UpdateViewMatrix() {
    DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&position_);
    DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&target_);
    DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&up_);
    viewMatrix_ = DirectX::XMMatrixLookAtLH(pos, target, up);
}

void Camera::MoveForward(float distance) {
    DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&position_);
    DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&target_);
    DirectX::XMVECTOR forward = DirectX::XMVectorSubtract(target, pos);
    forward = DirectX::XMVector3Normalize(forward);
    
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(forward, distance));
    target = DirectX::XMVectorAdd(target, DirectX::XMVectorScale(forward, distance));
    
    DirectX::XMStoreFloat3(&position_, pos);
    DirectX::XMStoreFloat3(&target_, target);
    UpdateViewMatrix();
}

void Camera::MoveRight(float distance) {
    DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&position_);
    DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&target_);
    DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&up_);
    
    DirectX::XMVECTOR forward = DirectX::XMVectorSubtract(target, pos);
    forward = DirectX::XMVector3Normalize(forward);
    DirectX::XMVECTOR right = DirectX::XMVector3Cross(forward, up);
    right = DirectX::XMVector3Normalize(right);
    
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(right, distance));
    target = DirectX::XMVectorAdd(target, DirectX::XMVectorScale(right, distance));
    
    DirectX::XMStoreFloat3(&position_, pos);
    DirectX::XMStoreFloat3(&target_, target);
    UpdateViewMatrix();
}

void Camera::MoveUp(float distance) {
    DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&position_);
    DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&target_);
    DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&up_);
    
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(up, distance));
    target = DirectX::XMVectorAdd(target, DirectX::XMVectorScale(up, distance));
    
    DirectX::XMStoreFloat3(&position_, pos);
    DirectX::XMStoreFloat3(&target_, target);
    UpdateViewMatrix();
}

void Camera::Rotate(float pitch, float yaw) {
    pitch_ += pitch;
    yaw_ += yaw;
    
    // Clamp pitch
    if (pitch_ > DirectX::XM_PI / 2.0f - 0.1f) pitch_ = DirectX::XM_PI / 2.0f - 0.1f;
    if (pitch_ < -DirectX::XM_PI / 2.0f + 0.1f) pitch_ = -DirectX::XM_PI / 2.0f + 0.1f;
    
    // Calculate new target based on spherical coordinates
    float cosY = cosf(yaw_);
    float sinY = sinf(yaw_);
    float cosP = cosf(pitch_);
    float sinP = sinf(pitch_);
    
    DirectX::XMFLOAT3 direction(cosY * cosP, sinP, sinY * cosP);
    DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&position_);
    DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&direction);
    DirectX::XMVECTOR target = DirectX::XMVectorAdd(pos, dir);
    
    DirectX::XMStoreFloat3(&target_, target);
    UpdateViewMatrix();
}

} // namespace Nexus
