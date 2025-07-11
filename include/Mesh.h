#pragma once

// Use DirectX 11 by default
#ifndef NEXUS_USE_DIRECTX11
#define NEXUS_USE_DIRECTX11
#endif

#ifdef NEXUS_USE_DIRECTX11
    #include <d3d11.h>
    #include <DirectXMath.h>
    using namespace DirectX;
#else
    #include <d3d9.h>
    #include <d3dx9.h>
#endif
#include <vector>
#include <string>

namespace Nexus {

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT3 tangent;
    XMFLOAT2 texCoord;
};

class Mesh {
public:
    Mesh();
    ~Mesh();

    // Loading
    bool LoadFromFile(const std::string& filename, ID3D11Device* device);
    bool CreateFromVertices(const std::vector<Vertex>& vertices, 
                           const std::vector<unsigned int>& indices,
                           ID3D11Device* device);

    // Rendering
    void Render(ID3D11DeviceContext* context);
    void SetWorldMatrix(const XMMATRIX& world) { worldMatrix_ = world; }

    // Properties
    int GetVertexCount() const { return vertexCount_; }
    int GetTriangleCount() const { return indexCount_ / 3; }
    size_t GetMemoryUsage() const { 
        return vertices_.size() * sizeof(Vertex) + indices_.size() * sizeof(uint32_t); 
    }

private:
    ID3D11Buffer* vertexBuffer_;
    ID3D11Buffer* indexBuffer_;
    int vertexCount_;
    int indexCount_;
    XMMATRIX worldMatrix_;
    std::vector<Vertex> vertices_;
    std::vector<unsigned int> indices_;
};

} // namespace Nexus
