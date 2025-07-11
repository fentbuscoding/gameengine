#include "Mesh.h"
#include "Logger.h"

namespace Nexus {

Mesh::Mesh()
    : vertexBuffer_(nullptr)
    , indexBuffer_(nullptr)
    , vertexCount_(0)
    , indexCount_(0)
{
    worldMatrix_ = XMMatrixIdentity();
}

Mesh::~Mesh() {
    if (indexBuffer_) {
        indexBuffer_->Release();
        indexBuffer_ = nullptr;
    }
    if (vertexBuffer_) {
        vertexBuffer_->Release();
        vertexBuffer_ = nullptr;
    }
}

bool Mesh::LoadFromFile(const std::string& filename, ID3D11Device* device) {
    Logger::Info("Loading mesh from file: " + filename);
    
    // For now, create a simple cube as a placeholder
    std::vector<Vertex> vertices = {
        // Front face
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        
        // Back face
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}
    };
    
    std::vector<unsigned int> indices = {
        0, 1, 2, 0, 2, 3,  // Front
        4, 5, 6, 4, 6, 7,  // Back
        5, 0, 3, 5, 3, 6,  // Left
        1, 4, 7, 1, 7, 2,  // Right
        3, 2, 7, 3, 7, 6,  // Top
        5, 4, 1, 5, 1, 0   // Bottom
    };
    
    return CreateFromVertices(vertices, indices, device);
}

bool Mesh::CreateFromVertices(const std::vector<Vertex>& vertices, 
                             const std::vector<unsigned int>& indices,
                             ID3D11Device* device) {
    if (!device) return false;
    
    vertexCount_ = static_cast<int>(vertices.size());
    indexCount_ = static_cast<int>(indices.size());
    
    // Create vertex buffer
    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = vertexCount_ * sizeof(Vertex);
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = vertices.data();
    
    HRESULT hr = device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create vertex buffer");
        return false;
    }
    
    // Create index buffer
    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = indexCount_ * sizeof(unsigned int);
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices.data();
    
    hr = device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create index buffer");
        return false;
    }
    
    Logger::Info("Mesh created successfully - Vertices: " + std::to_string(vertexCount_) + 
                 ", Triangles: " + std::to_string(indexCount_ / 3));
    
    return true;
}

void Mesh::Render(ID3D11DeviceContext* context) {
    if (!context || !vertexBuffer_ || !indexBuffer_) return;
    
    // Set vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vertexBuffer_, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer_, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Draw
    context->DrawIndexed(indexCount_, 0, 0);
}

} // namespace Nexus
