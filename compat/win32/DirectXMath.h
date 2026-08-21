// ---------------------------------------------------------------------------
// DirectXMath.h - portable stand-in for the Windows SDK header of the same name
// ---------------------------------------------------------------------------
//
// Nexus Engine was originally written directly against DirectXMath, which ships
// only with the Windows SDK. That made every header in the engine unbuildable on
// Linux and macOS. This file provides a scalar, dependency-free implementation
// of the subset of DirectXMath the engine actually uses, matching the original
// API and - importantly - the original *conventions*:
//
//   * Row-major storage.
//   * Row-vector convention, i.e. a point is transformed as v' = v * M, so
//     matrix products read left-to-right in the order transforms are applied.
//   * Left-handed coordinate system for the LH projection/view helpers.
//   * Angles in radians.
//
// Keeping those conventions identical means the same scene data and the same
// shader constants produce the same image on every platform. This header is
// only placed on the include path for non-Windows builds; Windows keeps using
// the real DirectXMath from the SDK.
//
// The implementation is deliberately scalar rather than SIMD: correctness and
// portability matter more here than peak throughput, and compilers vectorise
// most of this well at -O2. XMVECTOR remains 16-byte aligned so that any code
// storing it in structures keeps the same layout expectations.
//
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

// The Windows SDK spells the calling convention out on every math function and
// uses a family of parameter-passing aliases to get vectors into registers.
// Neither matters for a scalar build, but sources that name them must compile.
#ifndef XM_CALLCONV
#define XM_CALLCONV
#endif

#ifndef XM_DEPRECATED
#define XM_DEPRECATED
#endif

namespace DirectX {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

constexpr float XM_PI       = 3.141592654f;
constexpr float XM_2PI      = 6.283185307f;
constexpr float XM_1DIVPI   = 0.318309886f;
constexpr float XM_1DIV2PI  = 0.159154943f;
constexpr float XM_PIDIV2   = 1.570796327f;
constexpr float XM_PIDIV4   = 0.785398163f;

constexpr float XMConvertToRadians(float degrees) noexcept {
    return degrees * (XM_PI / 180.0f);
}

constexpr float XMConvertToDegrees(float radians) noexcept {
    return radians * (180.0f / XM_PI);
}

// ---------------------------------------------------------------------------
// Storage types
//
// These mirror the SDK layouts exactly so they can be memcpy'd into constant
// buffers. Unlike the SDK versions they zero-initialise by default: reading an
// uninitialised transform is undefined behaviour that is very easy to write by
// accident, and zeroing costs nothing next to the work these feed into.
// ---------------------------------------------------------------------------

struct XMFLOAT2 {
    float x = 0.0f;
    float y = 0.0f;

    XMFLOAT2() = default;
    constexpr XMFLOAT2(float _x, float _y) noexcept : x(_x), y(_y) {}
    explicit XMFLOAT2(const float* a) noexcept : x(a[0]), y(a[1]) {}
};

struct XMFLOAT3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    XMFLOAT3() = default;
    constexpr XMFLOAT3(float _x, float _y, float _z) noexcept : x(_x), y(_y), z(_z) {}
    explicit XMFLOAT3(const float* a) noexcept : x(a[0]), y(a[1]), z(a[2]) {}
};

struct XMFLOAT4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    XMFLOAT4() = default;
    constexpr XMFLOAT4(float _x, float _y, float _z, float _w) noexcept
        : x(_x), y(_y), z(_z), w(_w) {}
    explicit XMFLOAT4(const float* a) noexcept : x(a[0]), y(a[1]), z(a[2]), w(a[3]) {}
};

// Aligned variants. The SDK distinguishes these for SIMD loads; here they are
// the same layout with a stricter alignment request.
struct alignas(16) XMFLOAT4A : public XMFLOAT4 {
    using XMFLOAT4::XMFLOAT4;
};

struct XMFLOAT3X3 {
    union {
        struct {
            float _11, _12, _13;
            float _21, _22, _23;
            float _31, _32, _33;
        };
        float m[3][3];
    };

    XMFLOAT3X3() noexcept : m{} {}
    float  operator()(size_t row, size_t col) const noexcept { return m[row][col]; }
    float& operator()(size_t row, size_t col)       noexcept { return m[row][col]; }
};

struct XMFLOAT4X4 {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };

    XMFLOAT4X4() noexcept : m{} {}
    constexpr XMFLOAT4X4(float m00, float m01, float m02, float m03,
                         float m10, float m11, float m12, float m13,
                         float m20, float m21, float m22, float m23,
                         float m30, float m31, float m32, float m33) noexcept
        : _11(m00), _12(m01), _13(m02), _14(m03),
          _21(m10), _22(m11), _23(m12), _24(m13),
          _31(m20), _32(m21), _33(m22), _34(m23),
          _41(m30), _42(m31), _43(m32), _44(m33) {}

    float  operator()(size_t row, size_t col) const noexcept { return m[row][col]; }
    float& operator()(size_t row, size_t col)       noexcept { return m[row][col]; }
};

struct alignas(16) XMFLOAT4X4A : public XMFLOAT4X4 {
    using XMFLOAT4X4::XMFLOAT4X4;
};

// ---------------------------------------------------------------------------
// XMVECTOR
//
// The SDK type is an opaque 4-lane SIMD register accessed through XMVectorGetX
// and friends. Exposing the lanes as a plain array keeps that access pattern
// working while making the type usable in constant expressions.
// ---------------------------------------------------------------------------

struct alignas(16) XMVECTOR {
    float f[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    float  operator[](size_t i) const noexcept { return f[i]; }
    float& operator[](size_t i)       noexcept { return f[i]; }
};

// Parameter-passing aliases. On Windows these select between register and
// reference passing depending on position; a scalar build just passes by value.
using FXMVECTOR = XMVECTOR;
using GXMVECTOR = XMVECTOR;
using HXMVECTOR = XMVECTOR;
using CXMVECTOR = const XMVECTOR&;

inline XMVECTOR XM_CALLCONV XMVectorSet(float x, float y, float z, float w) noexcept {
    XMVECTOR v;
    v.f[0] = x; v.f[1] = y; v.f[2] = z; v.f[3] = w;
    return v;
}

inline XMVECTOR XM_CALLCONV XMVectorReplicate(float value) noexcept {
    return XMVectorSet(value, value, value, value);
}

inline XMVECTOR XM_CALLCONV XMVectorZero() noexcept {
    return XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
}

inline float XM_CALLCONV XMVectorGetX(FXMVECTOR v) noexcept { return v.f[0]; }
inline float XM_CALLCONV XMVectorGetY(FXMVECTOR v) noexcept { return v.f[1]; }
inline float XM_CALLCONV XMVectorGetZ(FXMVECTOR v) noexcept { return v.f[2]; }
inline float XM_CALLCONV XMVectorGetW(FXMVECTOR v) noexcept { return v.f[3]; }

inline XMVECTOR XM_CALLCONV XMVectorSetX(FXMVECTOR v, float x) noexcept {
    XMVECTOR r = v; r.f[0] = x; return r;
}
inline XMVECTOR XM_CALLCONV XMVectorSetY(FXMVECTOR v, float y) noexcept {
    XMVECTOR r = v; r.f[1] = y; return r;
}
inline XMVECTOR XM_CALLCONV XMVectorSetZ(FXMVECTOR v, float z) noexcept {
    XMVECTOR r = v; r.f[2] = z; return r;
}
inline XMVECTOR XM_CALLCONV XMVectorSetW(FXMVECTOR v, float w) noexcept {
    XMVECTOR r = v; r.f[3] = w; return r;
}

// ---------------------------------------------------------------------------
// Component-wise vector arithmetic
// ---------------------------------------------------------------------------

inline XMVECTOR XM_CALLCONV XMVectorAdd(FXMVECTOR a, FXMVECTOR b) noexcept {
    return XMVectorSet(a.f[0] + b.f[0], a.f[1] + b.f[1], a.f[2] + b.f[2], a.f[3] + b.f[3]);
}

inline XMVECTOR XM_CALLCONV XMVectorSubtract(FXMVECTOR a, FXMVECTOR b) noexcept {
    return XMVectorSet(a.f[0] - b.f[0], a.f[1] - b.f[1], a.f[2] - b.f[2], a.f[3] - b.f[3]);
}

inline XMVECTOR XM_CALLCONV XMVectorMultiply(FXMVECTOR a, FXMVECTOR b) noexcept {
    return XMVectorSet(a.f[0] * b.f[0], a.f[1] * b.f[1], a.f[2] * b.f[2], a.f[3] * b.f[3]);
}

inline XMVECTOR XM_CALLCONV XMVectorDivide(FXMVECTOR a, FXMVECTOR b) noexcept {
    return XMVectorSet(a.f[0] / b.f[0], a.f[1] / b.f[1], a.f[2] / b.f[2], a.f[3] / b.f[3]);
}

inline XMVECTOR XM_CALLCONV XMVectorScale(FXMVECTOR v, float s) noexcept {
    return XMVectorSet(v.f[0] * s, v.f[1] * s, v.f[2] * s, v.f[3] * s);
}

inline XMVECTOR XM_CALLCONV XMVectorNegate(FXMVECTOR v) noexcept {
    return XMVectorSet(-v.f[0], -v.f[1], -v.f[2], -v.f[3]);
}

inline XMVECTOR XM_CALLCONV XMVectorLerp(FXMVECTOR a, FXMVECTOR b, float t) noexcept {
    return XMVectorAdd(a, XMVectorScale(XMVectorSubtract(b, a), t));
}

inline XMVECTOR XM_CALLCONV XMVectorSqrt(FXMVECTOR v) noexcept {
    return XMVectorSet(std::sqrt(v.f[0]), std::sqrt(v.f[1]),
                       std::sqrt(v.f[2]), std::sqrt(v.f[3]));
}

inline XMVECTOR XM_CALLCONV XMVectorMin(FXMVECTOR a, FXMVECTOR b) noexcept {
    return XMVectorSet(a.f[0] < b.f[0] ? a.f[0] : b.f[0],
                       a.f[1] < b.f[1] ? a.f[1] : b.f[1],
                       a.f[2] < b.f[2] ? a.f[2] : b.f[2],
                       a.f[3] < b.f[3] ? a.f[3] : b.f[3]);
}

inline XMVECTOR XM_CALLCONV XMVectorMax(FXMVECTOR a, FXMVECTOR b) noexcept {
    return XMVectorSet(a.f[0] > b.f[0] ? a.f[0] : b.f[0],
                       a.f[1] > b.f[1] ? a.f[1] : b.f[1],
                       a.f[2] > b.f[2] ? a.f[2] : b.f[2],
                       a.f[3] > b.f[3] ? a.f[3] : b.f[3]);
}

inline XMVECTOR operator+(FXMVECTOR a, FXMVECTOR b) noexcept { return XMVectorAdd(a, b); }
inline XMVECTOR operator-(FXMVECTOR a, FXMVECTOR b) noexcept { return XMVectorSubtract(a, b); }
inline XMVECTOR operator*(FXMVECTOR a, FXMVECTOR b) noexcept { return XMVectorMultiply(a, b); }
inline XMVECTOR operator/(FXMVECTOR a, FXMVECTOR b) noexcept { return XMVectorDivide(a, b); }
inline XMVECTOR operator*(FXMVECTOR v, float s) noexcept { return XMVectorScale(v, s); }
inline XMVECTOR operator*(float s, FXMVECTOR v) noexcept { return XMVectorScale(v, s); }
inline XMVECTOR operator-(FXMVECTOR v) noexcept { return XMVectorNegate(v); }

// ---------------------------------------------------------------------------
// Load / store
//
// Loading a 2- or 3-component value leaves the unused lanes at zero, matching
// the SDK. That detail matters: XMVector3Dot on a loaded XMFLOAT3 relies on the
// w lane being zero rather than garbage.
// ---------------------------------------------------------------------------

inline XMVECTOR XM_CALLCONV XMLoadFloat2(const XMFLOAT2* src) noexcept {
    return XMVectorSet(src->x, src->y, 0.0f, 0.0f);
}

inline XMVECTOR XM_CALLCONV XMLoadFloat3(const XMFLOAT3* src) noexcept {
    return XMVectorSet(src->x, src->y, src->z, 0.0f);
}

inline XMVECTOR XM_CALLCONV XMLoadFloat4(const XMFLOAT4* src) noexcept {
    return XMVectorSet(src->x, src->y, src->z, src->w);
}

inline void XM_CALLCONV XMStoreFloat2(XMFLOAT2* dst, FXMVECTOR v) noexcept {
    dst->x = v.f[0];
    dst->y = v.f[1];
}

inline void XM_CALLCONV XMStoreFloat3(XMFLOAT3* dst, FXMVECTOR v) noexcept {
    dst->x = v.f[0];
    dst->y = v.f[1];
    dst->z = v.f[2];
}

inline void XM_CALLCONV XMStoreFloat4(XMFLOAT4* dst, FXMVECTOR v) noexcept {
    dst->x = v.f[0];
    dst->y = v.f[1];
    dst->z = v.f[2];
    dst->w = v.f[3];
}

// ---------------------------------------------------------------------------
// Geometric operations
//
// The SDK returns dot products and lengths *splatted* across all four lanes
// rather than as a scalar, so callers can feed them straight back into vector
// arithmetic. That behaviour is reproduced here; code doing
// XMVectorGetX(XMVector3Dot(a, b)) works unchanged.
// ---------------------------------------------------------------------------

inline XMVECTOR XM_CALLCONV XMVector2Dot(FXMVECTOR a, FXMVECTOR b) noexcept {
    return XMVectorReplicate(a.f[0] * b.f[0] + a.f[1] * b.f[1]);
}

inline XMVECTOR XM_CALLCONV XMVector3Dot(FXMVECTOR a, FXMVECTOR b) noexcept {
    return XMVectorReplicate(a.f[0] * b.f[0] + a.f[1] * b.f[1] + a.f[2] * b.f[2]);
}

inline XMVECTOR XM_CALLCONV XMVector4Dot(FXMVECTOR a, FXMVECTOR b) noexcept {
    return XMVectorReplicate(a.f[0] * b.f[0] + a.f[1] * b.f[1] +
                             a.f[2] * b.f[2] + a.f[3] * b.f[3]);
}

inline XMVECTOR XM_CALLCONV XMVector3Cross(FXMVECTOR a, FXMVECTOR b) noexcept {
    return XMVectorSet(a.f[1] * b.f[2] - a.f[2] * b.f[1],
                       a.f[2] * b.f[0] - a.f[0] * b.f[2],
                       a.f[0] * b.f[1] - a.f[1] * b.f[0],
                       0.0f);
}

inline XMVECTOR XM_CALLCONV XMVector2LengthSq(FXMVECTOR v) noexcept { return XMVector2Dot(v, v); }
inline XMVECTOR XM_CALLCONV XMVector3LengthSq(FXMVECTOR v) noexcept { return XMVector3Dot(v, v); }
inline XMVECTOR XM_CALLCONV XMVector4LengthSq(FXMVECTOR v) noexcept { return XMVector4Dot(v, v); }

inline XMVECTOR XM_CALLCONV XMVector2Length(FXMVECTOR v) noexcept {
    return XMVectorReplicate(std::sqrt(v.f[0] * v.f[0] + v.f[1] * v.f[1]));
}

inline XMVECTOR XM_CALLCONV XMVector3Length(FXMVECTOR v) noexcept {
    return XMVectorReplicate(std::sqrt(v.f[0] * v.f[0] + v.f[1] * v.f[1] + v.f[2] * v.f[2]));
}

inline XMVECTOR XM_CALLCONV XMVector4Length(FXMVECTOR v) noexcept {
    return XMVectorReplicate(std::sqrt(v.f[0] * v.f[0] + v.f[1] * v.f[1] +
                                       v.f[2] * v.f[2] + v.f[3] * v.f[3]));
}

// Normalising a degenerate vector has no meaningful answer. The SDK propagates
// infinities and NaNs; returning the zero vector instead keeps a single bad
// input from poisoning an entire frame of transforms.
inline XMVECTOR XM_CALLCONV XMVector2Normalize(FXMVECTOR v) noexcept {
    const float len = std::sqrt(v.f[0] * v.f[0] + v.f[1] * v.f[1]);
    if (len <= 0.0f || !std::isfinite(len)) {
        return XMVectorZero();
    }
    return XMVectorScale(v, 1.0f / len);
}

inline XMVECTOR XM_CALLCONV XMVector3Normalize(FXMVECTOR v) noexcept {
    const float len = std::sqrt(v.f[0] * v.f[0] + v.f[1] * v.f[1] + v.f[2] * v.f[2]);
    if (len <= 0.0f || !std::isfinite(len)) {
        return XMVectorZero();
    }
    return XMVectorScale(v, 1.0f / len);
}

inline XMVECTOR XM_CALLCONV XMVector4Normalize(FXMVECTOR v) noexcept {
    const float len = std::sqrt(v.f[0] * v.f[0] + v.f[1] * v.f[1] +
                                v.f[2] * v.f[2] + v.f[3] * v.f[3]);
    if (len <= 0.0f || !std::isfinite(len)) {
        return XMVectorZero();
    }
    return XMVectorScale(v, 1.0f / len);
}

// ---------------------------------------------------------------------------
// XMMATRIX
//
// Four row vectors, row-major, row-vector convention: r[3] holds the
// translation. XMMatrixMultiply(A, B) applies A first, then B.
// ---------------------------------------------------------------------------

struct alignas(16) XMMATRIX {
    XMVECTOR r[4];

    XMMATRIX() = default;

    XMMATRIX(FXMVECTOR r0, FXMVECTOR r1, FXMVECTOR r2, FXMVECTOR r3) noexcept {
        r[0] = r0; r[1] = r1; r[2] = r2; r[3] = r3;
    }

    XMMATRIX(float m00, float m01, float m02, float m03,
             float m10, float m11, float m12, float m13,
             float m20, float m21, float m22, float m23,
             float m30, float m31, float m32, float m33) noexcept {
        r[0] = XMVectorSet(m00, m01, m02, m03);
        r[1] = XMVectorSet(m10, m11, m12, m13);
        r[2] = XMVectorSet(m20, m21, m22, m23);
        r[3] = XMVectorSet(m30, m31, m32, m33);
    }
};

using FXMMATRIX = XMMATRIX;
using CXMMATRIX = const XMMATRIX&;

inline XMMATRIX XM_CALLCONV XMMatrixIdentity() noexcept {
    return XMMATRIX(1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

inline XMMATRIX XM_CALLCONV XMMatrixMultiply(FXMMATRIX a, CXMMATRIX b) noexcept {
    XMMATRIX out;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            out.r[i].f[j] = a.r[i].f[0] * b.r[0].f[j] +
                            a.r[i].f[1] * b.r[1].f[j] +
                            a.r[i].f[2] * b.r[2].f[j] +
                            a.r[i].f[3] * b.r[3].f[j];
        }
    }
    return out;
}

inline XMMATRIX operator*(FXMMATRIX a, CXMMATRIX b) noexcept { return XMMatrixMultiply(a, b); }

inline XMMATRIX XM_CALLCONV XMMatrixTranspose(FXMMATRIX m) noexcept {
    XMMATRIX out;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            out.r[i].f[j] = m.r[j].f[i];
        }
    }
    return out;
}

// Row-vector transform: v' = v * M. The point overload supplies w = 1 so the
// translation row contributes; the normal overload supplies w = 0 so it does not.
inline XMVECTOR XM_CALLCONV XMVector4Transform(FXMVECTOR v, CXMMATRIX m) noexcept {
    return XMVectorSet(
        v.f[0] * m.r[0].f[0] + v.f[1] * m.r[1].f[0] + v.f[2] * m.r[2].f[0] + v.f[3] * m.r[3].f[0],
        v.f[0] * m.r[0].f[1] + v.f[1] * m.r[1].f[1] + v.f[2] * m.r[2].f[1] + v.f[3] * m.r[3].f[1],
        v.f[0] * m.r[0].f[2] + v.f[1] * m.r[1].f[2] + v.f[2] * m.r[2].f[2] + v.f[3] * m.r[3].f[2],
        v.f[0] * m.r[0].f[3] + v.f[1] * m.r[1].f[3] + v.f[2] * m.r[2].f[3] + v.f[3] * m.r[3].f[3]);
}

inline XMVECTOR XM_CALLCONV XMVector3Transform(FXMVECTOR v, CXMMATRIX m) noexcept {
    return XMVector4Transform(XMVectorSetW(v, 1.0f), m);
}

inline XMVECTOR XM_CALLCONV XMVector3TransformNormal(FXMVECTOR v, CXMMATRIX m) noexcept {
    return XMVector4Transform(XMVectorSetW(v, 0.0f), m);
}

// Perspective divide after transform, for projecting a point to clip space.
inline XMVECTOR XM_CALLCONV XMVector3TransformCoord(FXMVECTOR v, CXMMATRIX m) noexcept {
    const XMVECTOR t = XMVector4Transform(XMVectorSetW(v, 1.0f), m);
    if (t.f[3] == 0.0f) {
        return XMVectorSet(t.f[0], t.f[1], t.f[2], 0.0f);
    }
    const float invW = 1.0f / t.f[3];
    return XMVectorSet(t.f[0] * invW, t.f[1] * invW, t.f[2] * invW, 1.0f);
}

inline XMMATRIX XM_CALLCONV XMLoadFloat4x4(const XMFLOAT4X4* src) noexcept {
    XMMATRIX out;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            out.r[i].f[j] = src->m[i][j];
        }
    }
    return out;
}

inline void XM_CALLCONV XMStoreFloat4x4(XMFLOAT4X4* dst, FXMMATRIX m) noexcept {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            dst->m[i][j] = m.r[i].f[j];
        }
    }
}

// ---------------------------------------------------------------------------
// Matrix construction
// ---------------------------------------------------------------------------

inline XMMATRIX XM_CALLCONV XMMatrixTranslation(float x, float y, float z) noexcept {
    return XMMATRIX(1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    x,    y,    z,    1.0f);
}

inline XMMATRIX XM_CALLCONV XMMatrixTranslationFromVector(FXMVECTOR offset) noexcept {
    return XMMatrixTranslation(offset.f[0], offset.f[1], offset.f[2]);
}

inline XMMATRIX XM_CALLCONV XMMatrixScaling(float sx, float sy, float sz) noexcept {
    return XMMATRIX(sx,   0.0f, 0.0f, 0.0f,
                    0.0f, sy,   0.0f, 0.0f,
                    0.0f, 0.0f, sz,   0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

inline XMMATRIX XM_CALLCONV XMMatrixScalingFromVector(FXMVECTOR scale) noexcept {
    return XMMatrixScaling(scale.f[0], scale.f[1], scale.f[2]);
}

inline XMMATRIX XM_CALLCONV XMMatrixRotationX(float angle) noexcept {
    const float s = std::sin(angle);
    const float c = std::cos(angle);
    return XMMATRIX(1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f,  c,    s,   0.0f,
                    0.0f, -s,    c,   0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

inline XMMATRIX XM_CALLCONV XMMatrixRotationY(float angle) noexcept {
    const float s = std::sin(angle);
    const float c = std::cos(angle);
    return XMMATRIX( c,   0.0f,  -s,   0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                     s,   0.0f,   c,   0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

inline XMMATRIX XM_CALLCONV XMMatrixRotationZ(float angle) noexcept {
    const float s = std::sin(angle);
    const float c = std::cos(angle);
    return XMMATRIX( c,    s,   0.0f, 0.0f,
                    -s,    c,   0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

// Rotation about a unit axis (Rodrigues' formula, transposed for row vectors).
inline XMMATRIX XM_CALLCONV XMMatrixRotationNormal(FXMVECTOR normalAxis, float angle) noexcept {
    const float s = std::sin(angle);
    const float c = std::cos(angle);
    const float t = 1.0f - c;
    const float x = normalAxis.f[0];
    const float y = normalAxis.f[1];
    const float z = normalAxis.f[2];

    return XMMATRIX(c + x * x * t,     x * y * t + z * s, x * z * t - y * s, 0.0f,
                    x * y * t - z * s, c + y * y * t,     y * z * t + x * s, 0.0f,
                    x * z * t + y * s, y * z * t - x * s, c + z * z * t,     0.0f,
                    0.0f,              0.0f,              0.0f,              1.0f);
}

inline XMMATRIX XM_CALLCONV XMMatrixRotationAxis(FXMVECTOR axis, float angle) noexcept {
    return XMMatrixRotationNormal(XMVector3Normalize(axis), angle);
}

// Intrinsic rotation applied in the order pitch (X), yaw (Y), roll (Z).
inline XMMATRIX XM_CALLCONV XMMatrixRotationRollPitchYaw(float pitch, float yaw, float roll) noexcept {
    return XMMatrixMultiply(XMMatrixMultiply(XMMatrixRotationX(pitch),
                                             XMMatrixRotationY(yaw)),
                            XMMatrixRotationZ(roll));
}

// ---------------------------------------------------------------------------
// Quaternions, stored as (x, y, z, w)
// ---------------------------------------------------------------------------

inline XMVECTOR XM_CALLCONV XMQuaternionIdentity() noexcept {
    return XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
}

inline XMVECTOR XM_CALLCONV XMQuaternionRotationAxis(FXMVECTOR axis, float angle) noexcept {
    const XMVECTOR n = XMVector3Normalize(axis);
    const float half = angle * 0.5f;
    const float s = std::sin(half);
    return XMVectorSet(n.f[0] * s, n.f[1] * s, n.f[2] * s, std::cos(half));
}

inline XMVECTOR XM_CALLCONV XMQuaternionRotationNormal(FXMVECTOR normalAxis, float angle) noexcept {
    const float half = angle * 0.5f;
    const float s = std::sin(half);
    return XMVectorSet(normalAxis.f[0] * s, normalAxis.f[1] * s,
                       normalAxis.f[2] * s, std::cos(half));
}

// Note the argument order: this returns the rotation of Q1 *followed by* Q2,
// which is the transpose of the usual Hamilton convention. Matching the SDK
// here matters because animation blending chains these calls.
inline XMVECTOR XM_CALLCONV XMQuaternionMultiply(FXMVECTOR q1, FXMVECTOR q2) noexcept {
    const float x1 = q1.f[0], y1 = q1.f[1], z1 = q1.f[2], w1 = q1.f[3];
    const float x2 = q2.f[0], y2 = q2.f[1], z2 = q2.f[2], w2 = q2.f[3];

    return XMVectorSet(
        (w2 * x1) + (x2 * w1) + (y2 * z1) - (z2 * y1),
        (w2 * y1) - (x2 * z1) + (y2 * w1) + (z2 * x1),
        (w2 * z1) + (x2 * y1) - (y2 * x1) + (z2 * w1),
        (w2 * w1) - (x2 * x1) - (y2 * y1) - (z2 * z1));
}

inline XMVECTOR XM_CALLCONV XMQuaternionConjugate(FXMVECTOR q) noexcept {
    return XMVectorSet(-q.f[0], -q.f[1], -q.f[2], q.f[3]);
}

inline XMVECTOR XM_CALLCONV XMQuaternionNormalize(FXMVECTOR q) noexcept {
    return XMVector4Normalize(q);
}

inline XMVECTOR XM_CALLCONV XMQuaternionLength(FXMVECTOR q) noexcept {
    return XMVector4Length(q);
}

// Shortest-arc spherical interpolation. Falls back to a normalised lerp when
// the two orientations are nearly identical, where sin(theta) underflows.
inline XMVECTOR XM_CALLCONV XMQuaternionSlerp(FXMVECTOR q0, FXMVECTOR q1, float t) noexcept {
    float cosOmega = q0.f[0] * q1.f[0] + q0.f[1] * q1.f[1] +
                     q0.f[2] * q1.f[2] + q0.f[3] * q1.f[3];

    XMVECTOR end = q1;
    if (cosOmega < 0.0f) {
        end = XMVectorNegate(q1);
        cosOmega = -cosOmega;
    }

    float scale0;
    float scale1;
    if (cosOmega > 0.9995f) {
        scale0 = 1.0f - t;
        scale1 = t;
    } else {
        const float omega = std::acos(cosOmega);
        const float invSinOmega = 1.0f / std::sin(omega);
        scale0 = std::sin((1.0f - t) * omega) * invSinOmega;
        scale1 = std::sin(t * omega) * invSinOmega;
    }

    return XMVector4Normalize(XMVectorAdd(XMVectorScale(q0, scale0),
                                          XMVectorScale(end, scale1)));
}

inline XMMATRIX XM_CALLCONV XMMatrixRotationQuaternion(FXMVECTOR q) noexcept {
    const float x = q.f[0], y = q.f[1], z = q.f[2], w = q.f[3];
    const float xx = x * x, yy = y * y, zz = z * z;
    const float xy = x * y, xz = x * z, yz = y * z;
    const float wx = w * x, wy = w * y, wz = w * z;

    return XMMATRIX(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz),        2.0f * (xz - wy),        0.0f,
                    2.0f * (xy - wz),        1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx),        0.0f,
                    2.0f * (xz + wy),        2.0f * (yz - wx),        1.0f - 2.0f * (xx + yy), 0.0f,
                    0.0f,                    0.0f,                    0.0f,                    1.0f);
}

// ---------------------------------------------------------------------------
// View and projection (left-handed)
// ---------------------------------------------------------------------------

inline XMMATRIX XM_CALLCONV XMMatrixLookToLH(FXMVECTOR eye, FXMVECTOR direction, FXMVECTOR up) noexcept {
    const XMVECTOR zaxis = XMVector3Normalize(direction);
    const XMVECTOR xaxis = XMVector3Normalize(XMVector3Cross(up, zaxis));
    const XMVECTOR yaxis = XMVector3Cross(zaxis, xaxis);

    const float tx = -(xaxis.f[0] * eye.f[0] + xaxis.f[1] * eye.f[1] + xaxis.f[2] * eye.f[2]);
    const float ty = -(yaxis.f[0] * eye.f[0] + yaxis.f[1] * eye.f[1] + yaxis.f[2] * eye.f[2]);
    const float tz = -(zaxis.f[0] * eye.f[0] + zaxis.f[1] * eye.f[1] + zaxis.f[2] * eye.f[2]);

    return XMMATRIX(xaxis.f[0], yaxis.f[0], zaxis.f[0], 0.0f,
                    xaxis.f[1], yaxis.f[1], zaxis.f[1], 0.0f,
                    xaxis.f[2], yaxis.f[2], zaxis.f[2], 0.0f,
                    tx,         ty,         tz,         1.0f);
}

inline XMMATRIX XM_CALLCONV XMMatrixLookAtLH(FXMVECTOR eye, FXMVECTOR focus, FXMVECTOR up) noexcept {
    return XMMatrixLookToLH(eye, XMVectorSubtract(focus, eye), up);
}

inline XMMATRIX XM_CALLCONV XMMatrixPerspectiveFovLH(float fovAngleY, float aspectRatio,
                                                     float nearZ, float farZ) noexcept {
    const float yScale = 1.0f / std::tan(fovAngleY * 0.5f);
    const float xScale = yScale / aspectRatio;
    const float range  = farZ / (farZ - nearZ);

    return XMMATRIX(xScale, 0.0f,   0.0f,           0.0f,
                    0.0f,   yScale, 0.0f,           0.0f,
                    0.0f,   0.0f,   range,          1.0f,
                    0.0f,   0.0f,   -range * nearZ, 0.0f);
}

inline XMMATRIX XM_CALLCONV XMMatrixOrthographicLH(float viewWidth, float viewHeight,
                                                   float nearZ, float farZ) noexcept {
    const float range = 1.0f / (farZ - nearZ);

    return XMMATRIX(2.0f / viewWidth, 0.0f,              0.0f,           0.0f,
                    0.0f,             2.0f / viewHeight, 0.0f,           0.0f,
                    0.0f,             0.0f,              range,          0.0f,
                    0.0f,             0.0f,              -range * nearZ, 1.0f);
}

inline XMMATRIX XM_CALLCONV XMMatrixOrthographicOffCenterLH(float left, float right,
                                                            float bottom, float top,
                                                            float nearZ, float farZ) noexcept {
    const float rw = 1.0f / (right - left);
    const float rh = 1.0f / (top - bottom);
    const float range = 1.0f / (farZ - nearZ);

    return XMMATRIX(rw + rw,              0.0f,                 0.0f,           0.0f,
                    0.0f,                 rh + rh,              0.0f,           0.0f,
                    0.0f,                 0.0f,                 range,          0.0f,
                    -(left + right) * rw, -(top + bottom) * rh, -range * nearZ, 1.0f);
}

// ---------------------------------------------------------------------------
// Determinant and inverse
// ---------------------------------------------------------------------------

inline XMVECTOR XM_CALLCONV XMMatrixDeterminant(FXMMATRIX m) noexcept {
    const float* a = m.r[0].f;
    const float* b = m.r[1].f;
    const float* c = m.r[2].f;
    const float* d = m.r[3].f;

    const float s0 = a[0] * b[1] - b[0] * a[1];
    const float s1 = a[0] * b[2] - b[0] * a[2];
    const float s2 = a[0] * b[3] - b[0] * a[3];
    const float s3 = a[1] * b[2] - b[1] * a[2];
    const float s4 = a[1] * b[3] - b[1] * a[3];
    const float s5 = a[2] * b[3] - b[2] * a[3];

    const float c5 = c[2] * d[3] - d[2] * c[3];
    const float c4 = c[1] * d[3] - d[1] * c[3];
    const float c3 = c[1] * d[2] - d[1] * c[2];
    const float c2 = c[0] * d[3] - d[0] * c[3];
    const float c1 = c[0] * d[2] - d[0] * c[2];
    const float c0 = c[0] * d[1] - d[0] * c[1];

    return XMVectorReplicate(s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);
}

// A singular matrix has no inverse; the SDK writes a zero determinant out and
// returns infinities. Returning identity keeps downstream transforms sane.
inline XMMATRIX XM_CALLCONV XMMatrixInverse(XMVECTOR* determinant, FXMMATRIX m) noexcept {
    const float* a = m.r[0].f;
    const float* b = m.r[1].f;
    const float* c = m.r[2].f;
    const float* d = m.r[3].f;

    const float s0 = a[0] * b[1] - b[0] * a[1];
    const float s1 = a[0] * b[2] - b[0] * a[2];
    const float s2 = a[0] * b[3] - b[0] * a[3];
    const float s3 = a[1] * b[2] - b[1] * a[2];
    const float s4 = a[1] * b[3] - b[1] * a[3];
    const float s5 = a[2] * b[3] - b[2] * a[3];

    const float c5 = c[2] * d[3] - d[2] * c[3];
    const float c4 = c[1] * d[3] - d[1] * c[3];
    const float c3 = c[1] * d[2] - d[1] * c[2];
    const float c2 = c[0] * d[3] - d[0] * c[3];
    const float c1 = c[0] * d[2] - d[0] * c[2];
    const float c0 = c[0] * d[1] - d[0] * c[1];

    const float det = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;

    if (determinant != nullptr) {
        *determinant = XMVectorReplicate(det);
    }

    if (det == 0.0f || !std::isfinite(det)) {
        return XMMatrixIdentity();
    }

    const float invDet = 1.0f / det;

    XMMATRIX out;
    out.r[0] = XMVectorSet(( b[1] * c5 - b[2] * c4 + b[3] * c3) * invDet,
                           (-a[1] * c5 + a[2] * c4 - a[3] * c3) * invDet,
                           ( d[1] * s5 - d[2] * s4 + d[3] * s3) * invDet,
                           (-c[1] * s5 + c[2] * s4 - c[3] * s3) * invDet);
    out.r[1] = XMVectorSet((-b[0] * c5 + b[2] * c2 - b[3] * c1) * invDet,
                           ( a[0] * c5 - a[2] * c2 + a[3] * c1) * invDet,
                           (-d[0] * s5 + d[2] * s2 - d[3] * s1) * invDet,
                           ( c[0] * s5 - c[2] * s2 + c[3] * s1) * invDet);
    out.r[2] = XMVectorSet(( b[0] * c4 - b[1] * c2 + b[3] * c0) * invDet,
                           (-a[0] * c4 + a[1] * c2 - a[3] * c0) * invDet,
                           ( d[0] * s4 - d[1] * s2 + d[3] * s0) * invDet,
                           (-c[0] * s4 + c[1] * s2 - c[3] * s0) * invDet);
    out.r[3] = XMVectorSet((-b[0] * c3 + b[1] * c1 - b[2] * c0) * invDet,
                           ( a[0] * c3 - a[1] * c1 + a[2] * c0) * invDet,
                           (-d[0] * s3 + d[1] * s1 - d[2] * s0) * invDet,
                           ( c[0] * s3 - c[1] * s1 + c[2] * s0) * invDet);
    return out;
}

} // namespace DirectX
