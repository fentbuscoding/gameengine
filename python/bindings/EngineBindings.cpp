#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "Engine.h"
#include "GraphicsDevice.h"
#include "ScriptingEngine.h"

namespace py = pybind11;
using namespace Nexus;

// Forward declarations
void init_math_bindings(py::module& m);

PYBIND11_MODULE(nexus_engine, m) {
    m.doc() = "Nexus Game Engine Python Bindings";
    
    // Initialize math bindings
    init_math_bindings(m);
    
    // Engine class
    py::class_<Engine>(m, "Engine")
        .def(py::init<>())
        .def("initialize", &Engine::Initialize, "Initialize the engine")
        .def("run", &Engine::Run, "Run the main engine loop")
        .def("shutdown", &Engine::Shutdown, "Shutdown the engine")
        .def("is_running", &Engine::IsRunning, "Check if engine is running")
        .def("request_exit", &Engine::RequestExit, "Request engine to exit")
        .def("get_fps", &Engine::GetFPS, "Get current FPS")
        .def("get_delta_time", &Engine::GetDeltaTime, "Get delta time")
        .def("set_target_fps", &Engine::SetTargetFPS, "Set target FPS")
        .def("get_graphics", &Engine::GetGraphics, 
             py::return_value_policy::reference_internal, "Get graphics device")
        .def("get_scripting", &Engine::GetScripting,
             py::return_value_policy::reference_internal, "Get scripting engine");
    
    // Graphics Device class
    py::class_<GraphicsDevice>(m, "GraphicsDevice")
        .def("begin_frame", &GraphicsDevice::BeginFrame, "Begin frame rendering")
        .def("end_frame", &GraphicsDevice::EndFrame, "End frame rendering")
        .def("present", &GraphicsDevice::Present, "Present frame to screen")
        .def("clear", [](GraphicsDevice& gd, float r, float g, float b, float a) {
            XMFLOAT4 color(r, g, b, a);
            gd.Clear(color);
        }, "Clear screen with color")
        .def("set_viewport", &GraphicsDevice::SetViewport, "Set viewport dimensions");
    
    // Scripting Engine class
    py::class_<ScriptingEngine>(m, "ScriptingEngine")
        .def("execute_file", &ScriptingEngine::ExecuteFile, "Execute Python file")
        .def("execute_string", &ScriptingEngine::ExecuteString, "Execute Python code string")
        .def("enable_hot_reload", &ScriptingEngine::EnableHotReload, "Enable hot reloading")
        .def("register_event_callback", &ScriptingEngine::RegisterEventCallback, 
             "Register event callback")
        .def("trigger_event", &ScriptingEngine::TriggerEvent, "Trigger event");
}
