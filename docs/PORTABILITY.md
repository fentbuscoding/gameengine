# Platform support and the compatibility layer

## Where things stand

Nexus was originally written directly against the Windows SDK: DirectXMath for
all vector and matrix maths, Direct3D 11 for rendering, DirectInput for input,
XAudio2 for sound, and Win32 for windowing. Those dependencies reached into
almost every header, so nothing outside Windows compiled at all.

That is no longer the case. The split now looks like this:

| Component | Windows | Linux / macOS |
|---|---|---|
| Engine headers (`include/`) | ✅ | ✅ |
| Math (`XMFLOAT3`, `XMMATRIX`, …) | ✅ SDK DirectXMath | ✅ `compat/win32/DirectXMath.h` |
| Core library `NexusCore` | ✅ | ✅ |
| ECS, components, systems | ✅ | ✅ |
| Platform layer (timing, filesystem, memory) | ✅ | ✅ |
| Scripting (Lua, Python) | ✅ | ✅ |
| Asset importers and `NexusAssetConverter` | ✅ | ✅ |
| Test suite | ✅ | ✅ |
| Windowing (`Nexus::Window`) | ✅ | ✅ (needs SDL2) |
| RHI: Vulkan backend | ✅ | ✅ Linux; ✅ macOS via MoltenVK |
| RHI: OpenGL backend | ✅ | ✅ (needs SDL2 + GLAD) |
| RHI: Direct3D 9 / 10 / 11 | ✅ | ❌ Windows-only by nature |
| Legacy D3D11 renderer, Win32 UI, DirectInput, XAudio2 | ✅ | ❌ see below |
| `NexusRHIDemo` (window + RHI) | ✅ | ✅ |
| `NexusEngine` executable | ✅ | ❌ depends on the legacy renderer |

## The compatibility layer: `compat/win32/`

Added to the include path **only on non-Windows builds**. Windows keeps using
the real Windows SDK, unchanged.

### `DirectXMath.h`

A scalar, dependency-free implementation of the DirectXMath subset the engine
uses. It reproduces the original *conventions* exactly, because that is what
determines whether the renderer produces a correct image:

- Row-major storage, **row-vector** convention — a point transforms as
  `v' = v * M`, so translation lives in the fourth **row**.
- `XMMatrixMultiply(A, B)` means "apply A, then B".
- Left-handed projections, with Direct3D's `z ∈ [0, 1]` clip range.
- `XMQuaternionMultiply(q1, q2)` means "q1 then q2" — the reverse of the usual
  Hamilton product, matching the SDK.

`tests/MathTests.cpp` asserts every one of these directly. On Windows that file
compiles against the real SDK header, which makes it a conformance test of the
portable version against the original rather than a test of it against itself.

Two deliberate differences from the SDK, both strictly safer:

- Storage types (`XMFLOAT3` and friends) zero-initialise by default instead of
  leaving members uninitialised.
- Normalising a zero-length vector returns the zero vector rather than
  propagating `NaN` or infinity through the rest of the frame.

### `Windows.h` and the DirectX headers

`Windows.h` supplies the Win32 **typedefs** the engine's declarations use —
`HWND`, `HRESULT`, `DWORD`, `LARGE_INTEGER`, the `VK_*` key codes — with the
same sizes and values as the SDK.

It deliberately declares **no Win32 functions**. Anything that genuinely needs
the Windows API belongs behind `#ifdef _WIN32` or in a platform backend, and
leaving the functions undeclared keeps that boundary honest: a stray
`CreateWindowEx` call fails to compile rather than failing mysteriously at link
time.

`d3d11.h`, `d3d12.h`, `dxgi.h`, `dinput.h`, `dsound.h`, `xaudio2.h`, `xinput.h`
and friends are **declaration-only**. The COM interfaces are forward-declared
but never defined, so a pointer to one can be stored and passed — which is all
the engine's headers need — while any attempt to actually *call* a Direct3D
method off Windows is a compile error.

## What is still Windows-only, and why

`src/CMakeLists.txt` lists these under `NEXUS_WINDOWS_ONLY_SOURCES`. They call
Direct3D 11, DirectInput, XAudio2, the Kinect SDK or Win32 windowing directly —
not through a typedef, but as real API calls — so they can only be compiled
against the Windows SDK:

`Engine.cpp`, `EngineErrorRecovery.cpp`, `NexusC.cpp`, `GraphicsDevice.cpp`,
`Texture.cpp`, `Shader.cpp`, `Mesh.cpp`, `TextRenderer.cpp`,
`LightingEngine.cpp`, `AnimationSystem.cpp`, `ResourceManager.cpp`,
`InputManager.cpp`, `AudioSystem.cpp`, `MotionControlSystem.cpp`,
`EngineUI.cpp`, `EngineUI_Modern.cpp`, `GameImporterUI.cpp`.

This is the pre-RHI renderer. The cross-platform path forward is the RHI
abstraction under `include/RHI/` and `src/rhi/`, which already has Vulkan and
OpenGL backends. Porting the engine's runtime onto the RHI is what would make
`NexusEngine` itself build everywhere; until then Linux and macOS build the core
library, the tools and the tests.

## Guarding against regression

Two things keep this from quietly reverting:

1. **The test suite** (`ctest --test-dir build --output-on-failure`) runs on
   Windows, Linux and macOS in CI, and covers the math conventions, the ECS, the
   platform layer and the Valve asset parser.

2. **The header self-containment check** compiles every public header as the
   first include in its own translation unit. This catches the exact class of
   bug that made the engine Windows-only: a header that happens to compile
   because some *other* header included `<algorithm>` or `<d3d11.h>` first, and
   breaks the moment include order or platform changes. Two headers are skipped
   because they need third-party SDKs this repository does not ship
   (`PhysXEngine.h` needs PhysX; `AdvancedPhysicsEngine.h` needs Bullet); both
   now fail with an explicit `#error` naming the missing dependency.

## Windowing and the RHI

`Nexus::Window` (`include/Window.h`) is the cross-platform window, backed by
SDL2. It is deliberately opaque — no SDL type appears in the header — and
`GetNativeHandle()` returns the `SDL_Window*` that the Vulkan backend needs for
surface creation. `Platform::CreateGameWindow` forwards to it.

Two sizes, and the distinction matters:

- `GetSize` is the logical window size the window manager works in.
- `GetDrawableSize` is the backing store in **pixels**, and is what a swap chain
  must be created at. On a HiDPI display they differ by the display scale: a
  swap chain built from the logical size renders at a fraction of the window's
  real resolution on every Retina Mac.

`ConsumeResize` reports a pending resize once and hands back the new drawable
size, so a host loop recreates its swap chain exactly once per resize. It is
driven by `SDL_WINDOWEVENT_SIZE_CHANGED` rather than `RESIZED`, so dragging a
window between displays with different scale factors — which changes the
drawable size while the logical size stays put — also invalidates the swap
chain.

## Running something

`NexusRHIDemo` is the runnable target on Linux and macOS. It opens a window,
brings up an RHI device on it and presents frames:

```bash
./build/bin/NexusRHIDemo                # run until the window is closed
./build/bin/NexusRHIDemo --frames 60    # present 60 frames then exit (CI smoke test)
./build/bin/NexusRHIDemo --probe        # report which backends are compiled in
./build/bin/NexusRHIDemo --api opengl   # pick a backend explicitly
```

With `--frames`, exiting before that many frames have been presented is a
failure, so a broken swap chain cannot pass silently.

`src/tools/rhi_demo.cpp` is about 250 lines and is the reference for how window,
swap chain, resize handling and device loss fit together.

## Vulkan on macOS

macOS has no native Vulkan driver. MoltenVK translates Vulkan to Metal, and the
Vulkan loader finds it through an ICD manifest. Three things follow, all handled
in `src/rhi/vulkan/VulkanDevice.cpp`:

- **Instance API version is negotiated, not assumed.** MoltenVK reports Vulkan
  1.2; requesting more than the loader supports fails `vkCreateInstance`
  outright.
- **`VK_KHR_portability_enumeration` is enabled** along with
  `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR`. Loaders from 1.3.216
  onwards hide non-conformant drivers unless an instance opts in, so without it
  `vkEnumeratePhysicalDevices` reports zero GPUs on every Mac.
- **`VK_KHR_portability_subset` is enabled at device creation** whenever the
  physical device exposes it. The specification requires this; omitting it is
  invalid usage.

Configure-time output warns if the headers are present but no MoltenVK ICD
manifest is installed — that combination builds cleanly and then finds no GPUs
at runtime.

## Building

### Linux

```bash
sudo apt-get install -y build-essential cmake libsdl2-dev libvulkan-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For a headless check (CI, containers) install `mesa-vulkan-drivers` and
`xvfb`, then:

```bash
xvfb-run -s "-screen 0 1280x720x24" ./build/bin/NexusRHIDemo --frames 30
```

### macOS

```bash
brew install cmake sdl2 molten-vk vulkan-headers vulkan-loader
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Homebrew's prefix is added to CMake's search path automatically, which is what
makes Apple Silicon (`/opt/homebrew`) work without extra flags.

### Windows

```bat
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Every third-party dependency is optional. A missing one disables its feature
with a message; none of them stop the build. The one exception worth knowing:
the Vulkan backend needs SDL2 as well as the Vulkan headers, because it creates
its surface through SDL — configuring with Vulkan but no SDL2 reports that
rather than producing a wall of compile errors.
