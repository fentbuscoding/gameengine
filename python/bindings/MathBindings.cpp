#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#ifdef NEXUS_DIRECTX_SUPPORT
#include <DirectXMath.h>
using namespace DirectX;
#endif

namespace py = pybind11;

// Math-related bindings
void init_math_bindings(py::module& m) {
#ifdef NEXUS_DIRECTX_SUPPORT
    // Vector3 binding
    py::class_<XMFLOAT3>(m, "Vector3")
        .def(py::init<float, float, float>())
        .def(py::init<>())
        .def_readwrite("x", &XMFLOAT3::x)
        .def_readwrite("y", &XMFLOAT3::y)
        .def_readwrite("z", &XMFLOAT3::z)
        .def("length", [](const XMFLOAT3& v) {
            XMVECTOR vec = XMLoadFloat3(&v);
            return XMVectorGetX(XMVector3Length(vec));
        })
        .def("normalize", [](XMFLOAT3& v) {
            XMVECTOR vec = XMLoadFloat3(&v);
            vec = XMVector3Normalize(vec);
            XMStoreFloat3(&v, vec);
            return v;
        })
        .def("dot", [](const XMFLOAT3& a, const XMFLOAT3& b) {
            XMVECTOR vecA = XMLoadFloat3(&a);
            XMVECTOR vecB = XMLoadFloat3(&b);
            return XMVectorGetX(XMVector3Dot(vecA, vecB));
        })
        .def("cross", [](const XMFLOAT3& a, const XMFLOAT3& b) {
            XMVECTOR vecA = XMLoadFloat3(&a);
            XMVECTOR vecB = XMLoadFloat3(&b);
            XMFLOAT3 result;
            XMStoreFloat3(&result, XMVector3Cross(vecA, vecB));
            return result;
        })
        .def("__repr__", [](const XMFLOAT3& v) {
            return "Vector3(" + std::to_string(v.x) + ", " + 
                   std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
        });

    // Vector4 binding
    py::class_<XMFLOAT4>(m, "Vector4")
        .def(py::init<float, float, float, float>())
        .def(py::init<>())
        .def_readwrite("x", &XMFLOAT4::x)
        .def_readwrite("y", &XMFLOAT4::y)
        .def_readwrite("z", &XMFLOAT4::z)
        .def_readwrite("w", &XMFLOAT4::w)
        .def("__repr__", [](const XMFLOAT4& v) {
            return "Vector4(" + std::to_string(v.x) + ", " + 
                   std::to_string(v.y) + ", " + std::to_string(v.z) + ", " +
                   std::to_string(v.w) + ")";
        });

    // Matrix binding
    py::class_<XMFLOAT4X4>(m, "Matrix")
        .def(py::init<>())
        .def("identity", []() {
            XMFLOAT4X4 result;
            XMStoreFloat4x4(&result, XMMatrixIdentity());
            return result;
        })
        .def("__repr__", [](const XMFLOAT4X4& m) {
            return "Matrix4x4";
        });
#else
    // Fallback math classes when DirectX is not available
    py::class_<std::array<float, 3>>(m, "Vector3")
        .def(py::init<>())
        .def("__getitem__", [](const std::array<float, 3>& v, int i) {
            return v[i];
        })
        .def("__setitem__", [](std::array<float, 3>& v, int i, float val) {
            v[i] = val;
        });

    py::class_<std::array<float, 4>>(m, "Vector4")
        .def(py::init<>())
        .def("__getitem__", [](const std::array<float, 4>& v, int i) {
            return v[i];
        })
        .def("__setitem__", [](std::array<float, 4>& v, int i, float val) {
            v[i] = val;
        });

    py::class_<std::array<float, 16>>(m, "Matrix")
        .def(py::init<>())
        .def("__getitem__", [](const std::array<float, 16>& m, int i) {
            return m[i];
        })
        .def("__setitem__", [](std::array<float, 16>& m, int i, float val) {
            m[i] = val;
        });
#endif
}
