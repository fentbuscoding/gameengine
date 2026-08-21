// dxgi.h - portable declaration-only stand-in for the DXGI header.
// See d3d11.h for the rationale: names exist so declarations parse, but the
// interfaces stay incomplete so no real DXGI call can compile off Windows.
#pragma once

#if defined(_WIN32)
#error "compat/win32/dxgi.h must not be used on Windows - include the SDK header instead."
#endif

#include "Windows.h"

struct IDXGIFactory;
struct IDXGIFactory1;
struct IDXGIAdapter;
struct IDXGIAdapter1;
struct IDXGIOutput;
struct IDXGIDevice;
struct IDXGISurface;
struct IDXGISwapChain;

// DXGI_FORMAT appears by value in Texture and PostProcessing headers. Values
// match the SDK enumeration so serialised asset data stays compatible.
typedef enum DXGI_FORMAT {
    DXGI_FORMAT_UNKNOWN                 = 0,
    DXGI_FORMAT_R32G32B32A32_TYPELESS   = 1,
    DXGI_FORMAT_R32G32B32A32_FLOAT      = 2,
    DXGI_FORMAT_R32G32B32A32_UINT       = 3,
    DXGI_FORMAT_R32G32B32_FLOAT         = 6,
    DXGI_FORMAT_R16G16B16A16_FLOAT      = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM      = 11,
    DXGI_FORMAT_R32G32_FLOAT            = 16,
    DXGI_FORMAT_R10G10B10A2_UNORM       = 24,
    DXGI_FORMAT_R11G11B10_FLOAT         = 26,
    DXGI_FORMAT_R8G8B8A8_TYPELESS       = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM          = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB     = 29,
    DXGI_FORMAT_R16G16_FLOAT            = 34,
    DXGI_FORMAT_R32_TYPELESS            = 39,
    DXGI_FORMAT_D32_FLOAT               = 40,
    DXGI_FORMAT_R32_FLOAT               = 41,
    DXGI_FORMAT_R32_UINT                = 42,
    DXGI_FORMAT_R24G8_TYPELESS          = 44,
    DXGI_FORMAT_D24_UNORM_S8_UINT       = 45,
    DXGI_FORMAT_R16_FLOAT               = 54,
    DXGI_FORMAT_D16_UNORM               = 55,
    DXGI_FORMAT_R16_UINT                = 57,
    DXGI_FORMAT_R8_UNORM                = 61,
    DXGI_FORMAT_BC1_UNORM               = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB          = 72,
    DXGI_FORMAT_BC2_UNORM               = 74,
    DXGI_FORMAT_BC3_UNORM               = 77,
    DXGI_FORMAT_BC4_UNORM               = 80,
    DXGI_FORMAT_BC5_UNORM               = 83,
    DXGI_FORMAT_BC7_UNORM               = 98
} DXGI_FORMAT;

typedef struct DXGI_SAMPLE_DESC {
    UINT Count;
    UINT Quality;
} DXGI_SAMPLE_DESC;
