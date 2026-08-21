// wrl/client.h - portable stand-in for the Windows Runtime C++ Template Library
// smart pointer.
//
// Microsoft::WRL::ComPtr is a COM reference-counting smart pointer. Engine
// headers use it to declare members holding Direct3D interfaces. This version
// reproduces the interface used by those declarations so the headers parse; it
// deliberately does NOT call AddRef or Release, because the interfaces it would
// be pointing at are incomplete types off Windows and there is nothing to
// reference-count without a live COM runtime.
#pragma once

#if defined(_WIN32)
#error "compat/win32/wrl/client.h must not be used on Windows - include the SDK header instead."
#endif

#include "../Windows.h"

namespace Microsoft {
namespace WRL {

template <typename T>
class ComPtr {
public:
    ComPtr() noexcept = default;
    ComPtr(decltype(nullptr)) noexcept {}

    T* Get() const noexcept { return ptr_; }
    T* operator->() const noexcept { return ptr_; }
    T** GetAddressOf() noexcept { return &ptr_; }
    T** operator&() noexcept { return &ptr_; }

    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    void Reset() noexcept { ptr_ = nullptr; }

private:
    T* ptr_ = nullptr;
};

} // namespace WRL
} // namespace Microsoft
