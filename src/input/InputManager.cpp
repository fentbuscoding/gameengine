#include "InputManager.h"
#include "Logger.h"
#include <cstring>

namespace Nexus {

InputManager::InputManager()
    : hwnd_(nullptr)
    , directInput_(nullptr)
    , keyboard_(nullptr)
    , mouse_(nullptr)
    , mouseX_(0)
    , mouseY_(0)
    , prevMouseX_(0)
    , prevMouseY_(0)
    , initialized_(false)
{
    memset(&keyboardState_, 0, sizeof(keyboardState_));
    memset(&prevKeyboardState_, 0, sizeof(prevKeyboardState_));
    memset(&mouseState_, 0, sizeof(mouseState_));
    memset(&prevMouseState_, 0, sizeof(prevMouseState_));
}

InputManager::~InputManager() {
    Shutdown();
}

bool InputManager::Initialize(HWND hwnd) {
    if (initialized_) return true;
    
    hwnd_ = hwnd;
    
    try {
        // For now, we'll use simple Windows input without DirectInput
        // to avoid DirectInput SDK dependency
        initialized_ = true;
        Logger::Info("Input manager initialized (using Windows API)");
        return true;
    } catch (const std::exception& e) {
        Logger::Error("Failed to initialize input manager: " + std::string(e.what()));
        return false;
    }
}

void InputManager::Shutdown() {
    if (!initialized_) return;
    
    // Cleanup DirectInput resources if used
    if (keyboard_) {
        keyboard_->Release();
        keyboard_ = nullptr;
    }
    
    if (mouse_) {
        mouse_->Release();
        mouse_ = nullptr;
    }
    
    if (directInput_) {
        directInput_->Release();
        directInput_ = nullptr;
    }
    
    initialized_ = false;
    Logger::Info("Input manager shutdown");
}

void InputManager::Update() {
    if (!initialized_) return;
    
    UpdateKeyboard();
    UpdateMouse();
    UpdateControllers();
}

void InputManager::UpdateKeyboard() {
    // Copy current state to previous
    memcpy(&prevKeyboardState_, &keyboardState_, sizeof(keyboardState_));
    
    // Update keyboard state using Windows API
    for (int i = 0; i < 256; ++i) {
        keyboardState_[i] = (GetAsyncKeyState(i) & 0x8000) ? 0x80 : 0x00;
    }
}

void InputManager::UpdateMouse() {
    // Copy current state to previous
    prevMouseState_ = mouseState_;
    prevMouseX_ = mouseX_;
    prevMouseY_ = mouseY_;
    
    // Get mouse position
    POINT mousePos;
    GetCursorPos(&mousePos);
    ScreenToClient(hwnd_, &mousePos);
    mouseX_ = mousePos.x;
    mouseY_ = mousePos.y;
    
    // Get mouse button states
    mouseState_.rgbButtons[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 0x80 : 0x00;
    mouseState_.rgbButtons[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 0x80 : 0x00;
    mouseState_.rgbButtons[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) ? 0x80 : 0x00;
}

void InputManager::UpdateControllers() {
    // Controller support would be implemented here
    // Using XInput for Xbox controllers
}

bool InputManager::IsKeyDown(KeyCode key) const {
    if (!initialized_) return false;
    return (keyboardState_[static_cast<int>(key)] & 0x80) != 0;
}

bool InputManager::IsKeyPressed(KeyCode key) const {
    if (!initialized_) return false;
    int keyIndex = static_cast<int>(key);
    return (keyboardState_[keyIndex] & 0x80) && !(prevKeyboardState_[keyIndex] & 0x80);
}

bool InputManager::IsKeyReleased(KeyCode key) const {
    if (!initialized_) return false;
    int keyIndex = static_cast<int>(key);
    return !(keyboardState_[keyIndex] & 0x80) && (prevKeyboardState_[keyIndex] & 0x80);
}

bool InputManager::IsMouseButtonDown(MouseButton button) const {
    if (!initialized_) return false;
    return (mouseState_.rgbButtons[static_cast<int>(button)] & 0x80) != 0;
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const {
    if (!initialized_) return false;
    int buttonIndex = static_cast<int>(button);
    return (mouseState_.rgbButtons[buttonIndex] & 0x80) && 
           !(prevMouseState_.rgbButtons[buttonIndex] & 0x80);
}

bool InputManager::IsMouseButtonReleased(MouseButton button) const {
    if (!initialized_) return false;
    int buttonIndex = static_cast<int>(button);
    return !(mouseState_.rgbButtons[buttonIndex] & 0x80) && 
           (prevMouseState_.rgbButtons[buttonIndex] & 0x80);
}

void InputManager::GetMousePosition(int& x, int& y) const {
    x = mouseX_;
    y = mouseY_;
}

void InputManager::GetMouseDelta(int& deltaX, int& deltaY) const {
    deltaX = mouseX_ - prevMouseX_;
    deltaY = mouseY_ - prevMouseY_;
}

int InputManager::GetMouseWheelDelta() const {
    // Mouse wheel delta would be tracked here
    return 0;
}

int InputManager::GetConnectedControllerCount() const {
    // Return number of connected controllers
    return 0;
}

bool InputManager::IsControllerConnected(int controllerId) const {
    // Check if specific controller is connected
    return false;
}

} // namespace Nexus
