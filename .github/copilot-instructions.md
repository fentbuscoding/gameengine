<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# Game Engine Development Instructions

This is a multi-language game engine project using C/C++ for the core engine and Python for scripting.

## Architecture Guidelines
- Use modern C++17/20 features where appropriate
- Follow RAII principles for resource management
- Use smart pointers for memory management
- Implement proper error handling and logging
- Use DirectX 9 for graphics rendering
- Design modular, extensible systems

## Code Style
- Use PascalCase for classes and functions
- Use camelCase for variables
- Use UPPER_CASE for constants and macros
- Prefer composition over inheritance
- Write self-documenting code with meaningful names

## Graphics Features
- Implement normal mapping for enhanced surface detail
- Add light bloom post-processing effects
- Create heat haze distortion effects
- Support enhanced texture formats and filtering
- Develop unified shadowing system (shadow mapping)
- Ensure DirectX 11/12 compatibility for broader hardware support

## Platform Support
- Target Windows primarily with console support considerations
- Use platform abstraction layers for cross-platform compatibility
- Implement proper input handling for various controllers

## Python Integration
- Use pybind11 for C++ to Python bindings
- Expose core engine functionality to Python scripts
- Allow game logic to be written in Python
- Provide hot-reloading capabilities for rapid development
