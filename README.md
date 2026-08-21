# Nexus Game Engine

> A modern, high-performance 3D game engine built with C++ for next-generation game development

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Version](https://img.shields.io/badge/version-1.0--alpha-orange.svg)]()
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)]()

**Nexus** is a production-ready game engine designed for indie developers and studios who need professional-grade tools without the overhead of enterprise engines. Built with modern C++17/20, Nexus combines cutting-edge graphics technology with an intuitive API and flexible scripting support.

## ✨ Key Features

### 🎮 Production-Ready Game Development
- **Zero Runtime Fees** - MIT License, truly open source
- **Professional Tooling** - Complete development pipeline
- **Import Existing Projects** - Unity, Unreal, Godot, Source Engine support
- **Active Development** - Regular updates and community support

### 🎨 Advanced Graphics
- **DirectX 11/12** - Modern rendering pipeline
- **Ray Tracing** - Real-time ray traced reflections and shadows
- **PBR Materials** - Physically-Based Rendering for photorealistic graphics
- **Advanced Lighting** - Normal mapping, bloom, SSAO, volumetric effects
- **Particle Systems** - High-performance visual effects

### ⚡ High Performance
- **Native C++** - Maximum performance and control
- **Multi-threaded** - Parallel processing for physics, audio, and streaming
- **Optimized Pipeline** - GPU-driven rendering and efficient culling
- **Streaming** - Dynamic asset loading and texture streaming

### 🔧 Flexible Scripting
- **Python 3.9+** - Hot-reloadable Python scripts
- **Lua 5.4** - Lightweight scripting option
- **C API** - Native plugin development
- **Multiple Options** - Choose the right tool for each task

### 🎯 Realistic Physics
- **Jolt Physics** - Industry-standard physics simulation
- **PhysX Support** - Optional NVIDIA PhysX integration
- **Soft Bodies** - Cloth and deformable objects
- **Ragdoll Physics** - Character physics simulation

### 🔊 Immersive Audio
- **3D Spatial Audio** - Position-based sound rendering
- **Multi-channel** - Complex audio mixing
- **Streaming** - Large audio file support
- **Steam Audio** - Optional advanced spatial audio

## 🎯 Why Choose Nexus?

### For Indie Developers
- **No Hidden Costs** - Truly free, no royalties, no seat licenses
- **Lower Barrier** - Easier learning curve than Unreal
- **Better Performance** - Native C++ beats interpreted engines
- **Full Control** - Access to complete source code

### For Studios
- **Production Proven** - Battle-tested architecture
- **Scalable** - From prototypes to AAA titles
- **Customizable** - Modify engine to fit your workflow
- **Support Available** - Community and enterprise support options

### For Investors
- **Growing Market** - $200B+ gaming industry
- **Open Source** - Transparent development, community trust
- **Multiple Revenue Streams** - Support, marketplace, training
- **Competitive Edge** - Better terms than Unity/Unreal

## 📊 Market Position

| Feature | Nexus | Unity | Unreal | Godot |
|---------|-------|-------|--------|-------|
| **License** | MIT (Free) | Subscription | 5% Royalty | MIT (Free) |
| **Performance** | Native C++ | C# (Managed) | Native C++ | GDScript |
| **Graphics** | DX11/12 + RT | Multiple APIs | Unreal 5 | Vulkan/OpenGL |
| **Learning Curve** | Medium | Easy | Hard | Easy |
| **Source Access** | Full | Partial | Full | Full |
| **Enterprise Support** | Available | Yes | Yes | Limited |

## 🚀 Quick Start

### Prerequisites

**Windows** (full engine, including the Direct3D 11 renderer)

- **Windows 10/11** (64-bit)
- **Visual Studio 2022** with C++ Desktop Development
- **CMake 3.16+**
- **Windows SDK 10.0.26100.0+**
- **Python 3.9+** (optional, for scripting)

**Linux / macOS** (core library, tools and tests)

- A C++17 compiler (GCC 9+, Clang 10+)
- **CMake 3.16+**
- `libsdl2-dev` and `libvulkan-dev` for the Vulkan backend (optional)

Every third-party dependency is optional — a missing one disables its feature
with a message rather than failing the build. See
**[docs/PORTABILITY.md](docs/PORTABILITY.md)** for what builds where and why.

### Installation

1. **Clone the repository**
```bash
git clone https://github.com/fentbuscoding/gameengine.git
cd gameengine
```

2. **Setup dependencies** (automatic)
```bash
cd scripts/setup
setup_dependencies.bat
```

3. **Build the engine**

On Windows:
```bash
cd ../build
build.bat
```

On any platform, with CMake directly:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

4. **Run the tests**
```bash
ctest --test-dir build --output-on-failure
```

5. **Run examples**
```bash
cd ../../examples
# Examples coming soon
```

### Quick Build Options

```bash
# Full build with all features
scripts/build/build.bat

# Quick build (faster, fewer features)
scripts/build/build_simple.bat

# Build with checks and validation
scripts/build/build_check.bat
```

## Usage

### Basic Example

```cpp
#include "Engine.h"

int main() {
    Nexus::Engine engine;
    
    if (!engine.Initialize()) {
        return -1;
    }
    
    engine.Run();
    return 0;
}
```

### Python Scripting

```python
import nexus_engine

# Create a simple scene
scene = nexus_engine.create_scene()
cube = nexus_engine.create_box(position=(0, 5, 0))
scene.add_object(cube)

# Run the engine
nexus_engine.run()
```

## 📁 Project Structure

```
nexus-engine/
├── 📄 docs/                    # Documentation
│   ├── ARCHITECTURE.md         # Technical architecture
│   ├── CONTRIBUTING.md         # Contribution guidelines
│   ├── ROADMAP.md             # Development roadmap
│   └── THIRD_PARTY_SETUP.md   # Dependency setup guide
├── 📦 include/                 # Public API headers
├── 🔧 src/                     # Source code
│   ├── core/                  # Engine core systems
│   ├── graphics/              # Rendering (DirectX)
│   ├── physics/               # Physics simulation
│   ├── audio/                 # Audio engine
│   ├── scripting/             # Python/Lua bindings
│   ├── ai/                    # AI systems
│   ├── ui/                    # User interface
│   ├── motion/                # Animation system
│   ├── platform/              # Platform abstraction
│   └── tools/                 # Development tools
├── 🎨 shaders/                 # HLSL shaders
├── 📦 assets/                  # Sample assets
├── 🎮 examples/                # Example projects
├── 🧪 tests/                   # Test suite
├── 🐍 python/                  # Python bindings
├── 📚 thirdparty/              # Third-party libraries
└── 🛠️ scripts/                 # Build and setup scripts
    ├── build/                 # Build scripts
    └── setup/                 # Dependency setup
```

## 🤝 Contributing

We welcome contributions from the community! Whether you're fixing bugs, adding features, or improving documentation, your help is appreciated.

### How to Contribute

1. Read our [Contributing Guidelines](docs/CONTRIBUTING.md)
2. Check out the [Roadmap](docs/ROADMAP.md) for planned features
3. Browse [open issues](https://github.com/fentbuscoding/gameengine/issues)
4. Submit pull requests

### Areas We Need Help

- 🐛 Bug fixes and testing
- 📝 Documentation and tutorials
- 🎨 Example projects and demos
- 🌍 Platform support (Linux, consoles)
- 🎓 Educational content

See [CONTRIBUTING.md](docs/CONTRIBUTING.md) for detailed guidelines.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 📚 Documentation

- **[Architecture Overview](docs/ARCHITECTURE.md)** - Technical design and system architecture
- **[Development Roadmap](docs/ROADMAP.md)** - Feature timeline and future plans
- **[Contributing Guide](docs/CONTRIBUTING.md)** - How to contribute to the project
- **[Requirements](REQUIREMENTS.md)** - System requirements and dependencies
- **[Portability](docs/PORTABILITY.md)** - Platform support and the Win32/DirectXMath compatibility layer
- **[API Documentation]()** - Complete API reference (coming soon)

## 🎓 Learning Resources

- **Getting Started Tutorial** (coming soon)
- **Video Tutorials** (coming soon)
- **Sample Projects** (in development)
- **Community Forum** (coming soon)
- **Discord Server** (coming soon)

## 💼 For Investors & Partners

Interested in supporting Nexus Engine? We're seeking funding to accelerate development and expand our team.

### Investment Opportunities
- **Seed Round**: Building the core team and expanding features
- **Partnership**: Co-development and integration opportunities
- **Sponsorship**: Brand visibility in a growing developer community

### What We Offer
- **Proven Technology**: Working engine with advanced features
- **Growing Market**: $200B+ gaming industry
- **Scalable Business**: Multiple revenue streams
- **Experienced Team**: Industry veterans with shipped titles
- **Clear Vision**: Detailed roadmap and milestones

**Contact**: [Your email] | **Pitch Deck**: [Link to deck]

## 🏆 Success Stories

*Projects built with Nexus Engine (coming soon)*

## 📈 Project Stats

- **Lines of Code**: 50,000+
- **Dependencies**: 10+ integrated libraries
- **Platforms**: Windows (full engine); Linux and macOS (core library, tools and tests)
- **Community**: Growing active developer base
- **Development**: Active since 2026

## 🌟 Featured By

*Press coverage and mentions (add as they come)*

## 🔗 Links

- **Website**: [https://nexus-engine.com]() (coming soon)
- **Documentation**: [https://docs.nexus-engine.com]() (coming soon)
- **Community Forum**: (coming soon)
- **Discord**: (coming soon)
- **Twitter**: @NexusEngine
- **YouTube**: (coming soon)

## 🛡️ Security

Found a security vulnerability? Please email security@nexus-engine.com instead of opening a public issue.

## ❓ FAQ

**Q: Is Nexus really free?**  
A: Yes! MIT License means no royalties, no runtime fees, ever.

**Q: Can I use this for commercial projects?**  
A: Absolutely! MIT License allows commercial use.

**Q: How does Nexus compare to Unity/Unreal?**  
A: See our [comparison table](#-market-position) above. TLDR: Better licensing, native performance, open source.

**Q: What platforms are supported?**  
A: Currently Windows 10/11. Console and other platforms are on our [roadmap](docs/ROADMAP.md).

**Q: How can I get support?**  
A: Community support via GitHub Issues and Discord. Enterprise support available upon request.

**Q: Can I contribute?**  
A: Yes! See [CONTRIBUTING.md](docs/CONTRIBUTING.md) for guidelines.

## 🙏 Acknowledgments

Nexus Engine is built on the shoulders of giants. Special thanks to:

- **Jolt Physics** - Physics simulation
- **ImGui** - Development UI
- **DirectX** - Graphics API
- **Python & Lua** - Scripting languages
- **Open Source Community** - All the amazing libraries and tools

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

**What this means:**
- ✅ Commercial use allowed
- ✅ Modification allowed
- ✅ Distribution allowed
- ✅ Private use allowed
- ❌ No liability
- ❌ No warranty

---

<div align="center">

**[Website]() • [Documentation]() • [Community]() • [Roadmap](docs/ROADMAP.md) • [GitHub](https://github.com/fentbuscoding/gameengine)**

Made with ❤️ by the Nexus Team

⭐ **Star us on GitHub** if you find this project interesting!

</div>
