// d3d12.h - portable declaration-only stand-in for the Direct3D 12 header.
// See d3d11.h for the rationale: names exist so declarations parse, interfaces
// stay incomplete so no real D3D12 call compiles off Windows.
#pragma once

#if defined(_WIN32)
#error "compat/win32/d3d12.h must not be used on Windows - include the SDK header instead."
#endif

#include "Windows.h"
#include "dxgi.h"

struct ID3D12Device;
struct ID3D12Device5;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12GraphicsCommandList4;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;
struct ID3D12PipelineState;
struct ID3D12RootSignature;
struct ID3D12StateObject;
struct ID3D12StateObjectProperties;
struct ID3D12Fence;
struct ID3D12Heap;
struct ID3D12QueryHeap;

typedef struct D3D12_GPU_VIRTUAL_ADDRESS_STRUCT { UINT64 value; } D3D12_GPU_VIRTUAL_ADDRESS_STRUCT;
typedef UINT64 D3D12_GPU_VIRTUAL_ADDRESS;

typedef struct D3D12_CPU_DESCRIPTOR_HANDLE { SIZE_T ptr; } D3D12_CPU_DESCRIPTOR_HANDLE;
typedef struct D3D12_GPU_DESCRIPTOR_HANDLE { UINT64 ptr; } D3D12_GPU_DESCRIPTOR_HANDLE;

typedef enum D3D12_RESOURCE_STATES {
    D3D12_RESOURCE_STATE_COMMON                  = 0,
    D3D12_RESOURCE_STATE_GENERIC_READ            = 0x0AC3,
    D3D12_RESOURCE_STATE_RENDER_TARGET           = 0x4,
    D3D12_RESOURCE_STATE_UNORDERED_ACCESS        = 0x8,
    D3D12_RESOURCE_STATE_DEPTH_WRITE             = 0x10,
    D3D12_RESOURCE_STATE_COPY_DEST               = 0x400,
    D3D12_RESOURCE_STATE_COPY_SOURCE             = 0x800
} D3D12_RESOURCE_STATES;

// --- DirectX Raytracing (DXR) ----------------------------------------------
// These live in d3d12.h in the real SDK, not in a separate "dxr.h".
typedef enum D3D12_RAYTRACING_TIER {
    D3D12_RAYTRACING_TIER_NOT_SUPPORTED = 0,
    D3D12_RAYTRACING_TIER_1_0           = 10,
    D3D12_RAYTRACING_TIER_1_1           = 11
} D3D12_RAYTRACING_TIER;

typedef enum D3D12_RAYTRACING_GEOMETRY_FLAGS {
    D3D12_RAYTRACING_GEOMETRY_FLAG_NONE                            = 0,
    D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE                          = 0x1,
    D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION  = 0x2
} D3D12_RAYTRACING_GEOMETRY_FLAGS;

typedef enum D3D12_RAYTRACING_INSTANCE_FLAGS {
    D3D12_RAYTRACING_INSTANCE_FLAG_NONE                          = 0,
    D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE         = 0x1,
    D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE = 0x2,
    D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE                  = 0x4,
    D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE              = 0x8
} D3D12_RAYTRACING_INSTANCE_FLAGS;

typedef enum D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS {
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE             = 0,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE     = 0x1,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE = 0x4,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD = 0x8
} D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS;
