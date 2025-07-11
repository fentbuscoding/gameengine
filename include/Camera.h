#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

namespace Nexus {

class Camera {
public:
    Camera();
    ~Camera();

    // Positioning
    void SetPosition(const DirectX::XMFLOAT3& position) { position_ = position; UpdateViewMatrix(); }
    void SetTarget(const DirectX::XMFLOAT3& target) { target_ = target; UpdateViewMatrix(); }
    void SetUp(const DirectX::XMFLOAT3& up) { up_ = up; UpdateViewMatrix(); }

    const DirectX::XMFLOAT3& GetPosition() const { return position_; }
    const DirectX::XMFLOAT3& GetTarget() const { return target_; }
    const DirectX::XMFLOAT3& GetUp() const { return up_; }

    // Projection
    void SetPerspective(float fovY, float aspectRatio, float nearPlane, float farPlane);
    void SetOrthographic(float width, float height, float nearPlane, float farPlane);

    // Matrices
    const DirectX::XMMATRIX& GetViewMatrix() const { return viewMatrix_; }
    const DirectX::XMMATRIX& GetProjectionMatrix() const { return projectionMatrix_; }
    DirectX::XMMATRIX GetViewProjectionMatrix() const { return DirectX::XMMatrixMultiply(viewMatrix_, projectionMatrix_); }

    // Movement
    void MoveForward(float distance);
    void MoveRight(float distance);
    void MoveUp(float distance);
    void Rotate(float pitch, float yaw);

private:
    void UpdateViewMatrix();

    DirectX::XMFLOAT3 position_;
    DirectX::XMFLOAT3 target_;
    DirectX::XMFLOAT3 up_;
    
    DirectX::XMMATRIX viewMatrix_;
    DirectX::XMMATRIX projectionMatrix_;
    
    float pitch_;
    float yaw_;
};

} // namespace Nexus
