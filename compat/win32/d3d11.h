// ---------------------------------------------------------------------------
// d3d11.h - portable declaration-only stand-in for the Direct3D 11 header
// ---------------------------------------------------------------------------
//
// Core Nexus headers (GraphicsDevice, Texture, EngineUI, ...) store Direct3D 11
// interface pointers as members. Those declarations only need the interface
// *names* to exist, so this header forward-declares them as incomplete types.
//
// That incompleteness is the point. A pointer to an incomplete type can be
// stored, passed and compared, but any attempt to actually call a D3D method
// off Windows is a compile error rather than a link failure or a silent stub
// that pretends the GPU is there. Real Direct3D work lives in the D3D backends
// under src/rhi/, which the build excludes on non-Windows platforms.
//
#pragma once

#if defined(_WIN32)
#error "compat/win32/d3d11.h must not be used on Windows - include the SDK header instead."
#endif

#include "Windows.h"
#include "dxgi.h"

// Direct3D 11 interfaces, declared but never defined.
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;
struct ID3D11Texture1D;
struct ID3D11Texture2D;
struct ID3D11Texture3D;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11GeometryShader;
struct ID3D11ComputeShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11InputLayout;
struct ID3D11SamplerState;
struct ID3D11BlendState;
struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11Query;
struct ID3D11Resource;
struct ID3D11ClassLinkage;
struct ID3D11CommandList;

// Feature levels and filter modes appear by value in engine headers, so these
// need real definitions. Values match the SDK.
typedef enum D3D_FEATURE_LEVEL {
    D3D_FEATURE_LEVEL_9_1  = 0x9100,
    D3D_FEATURE_LEVEL_9_2  = 0x9200,
    D3D_FEATURE_LEVEL_9_3  = 0x9300,
    D3D_FEATURE_LEVEL_10_0 = 0xa000,
    D3D_FEATURE_LEVEL_10_1 = 0xa100,
    D3D_FEATURE_LEVEL_11_0 = 0xb000,
    D3D_FEATURE_LEVEL_11_1 = 0xb100,
    D3D_FEATURE_LEVEL_12_0 = 0xc000,
    D3D_FEATURE_LEVEL_12_1 = 0xc100
} D3D_FEATURE_LEVEL;

typedef enum D3D11_FILTER {
    D3D11_FILTER_MIN_MAG_MIP_POINT               = 0,
    D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR        = 0x1,
    D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT  = 0x4,
    D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR        = 0x5,
    D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT        = 0x10,
    D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,
    D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT        = 0x14,
    D3D11_FILTER_MIN_MAG_MIP_LINEAR              = 0x15,
    D3D11_FILTER_ANISOTROPIC                     = 0x55
} D3D11_FILTER;

typedef enum D3D11_USAGE {
    D3D11_USAGE_DEFAULT   = 0,
    D3D11_USAGE_IMMUTABLE = 1,
    D3D11_USAGE_DYNAMIC   = 2,
    D3D11_USAGE_STAGING   = 3
} D3D11_USAGE;

typedef enum D3D11_PRIMITIVE_TOPOLOGY {
    D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED     = 0,
    D3D11_PRIMITIVE_TOPOLOGY_POINTLIST     = 1,
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST      = 2,
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP     = 3,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST  = 4,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP = 5
} D3D11_PRIMITIVE_TOPOLOGY;

typedef struct D3D11_VIEWPORT {
    FLOAT TopLeftX;
    FLOAT TopLeftY;
    FLOAT Width;
    FLOAT Height;
    FLOAT MinDepth;
    FLOAT MaxDepth;
} D3D11_VIEWPORT;
