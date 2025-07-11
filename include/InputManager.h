#pragma once

#include "Platform.h"
#include <dinput.h>
#include <vector>
#include <array>

namespace Nexus {

enum class KeyCode {
    A = 0x1E, B = 0x30, C = 0x2E, D = 0x20, E = 0x12, F = 0x21, G = 0x22, H = 0x23,
    I = 0x17, J = 0x24, K = 0x25, L = 0x26, M = 0x32, N = 0x31, O = 0x18, P = 0x19,
    Q = 0x10, R = 0x13, S = 0x1F, T = 0x14, U = 0x16, V = 0x2F, W = 0x11, X = 0x2D,
    Y = 0x15, Z = 0x2C,
    Space = 0x39, Enter = 0x1C, Escape = 0x01,
    Left = 0xCB, Right = 0xCD, Up = 0xC8, Down = 0xD0
};

enum class MouseButton {
    Left = 0, Right = 1, Middle = 2
};

/**
 * Input management system for keyboard, mouse, and game controllers
 */
class InputManager {
public:
    InputManager();
    ~InputManager();

    // Initialization
    bool Initialize(HWND hwnd);
    void Shutdown();
    void Update();

    // Keyboard input
    bool IsKeyDown(KeyCode key) const;
    bool IsKeyPressed(KeyCode key) const;  // True only on the frame key was pressed
    bool IsKeyReleased(KeyCode key) const; // True only on the frame key was released

    // Mouse input
    bool IsMouseButtonDown(MouseButton button) const;
    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;
    void GetMousePosition(int& x, int& y) const;
    void GetMouseDelta(int& deltaX, int& deltaY) const;
    int GetMouseWheelDelta() const;

    // Controller support
    int GetConnectedControllerCount() const;
    bool IsControllerConnected(int controllerId) const;

private:
    void UpdateKeyboard();
    void UpdateMouse();
    void UpdateControllers();

    HWND hwnd_;
    IDirectInput8* directInput_;
    IDirectInputDevice8* keyboard_;
    IDirectInputDevice8* mouse_;

    // Keyboard state
    std::array<unsigned char, 256> keyboardState_;
    std::array<unsigned char, 256> prevKeyboardState_;

    // Mouse state
    DIMOUSESTATE mouseState_;
    DIMOUSESTATE prevMouseState_;
    int mouseX_, mouseY_;
    int prevMouseX_, prevMouseY_;

    bool initialized_;
};

} // namespace Nexus
