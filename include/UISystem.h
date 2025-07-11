#pragma once

#include <string>
#include <functional>
#include <memory>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct HWND__;
typedef HWND__* HWND;

namespace Nexus {

class Engine;

class UISystem {
public:
    UISystem();
    ~UISystem();
    
    bool Initialize(Engine* engine, ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd);
    void Shutdown();
    
    void NewFrame();
    void Render();
    void EndFrame();
    
    void SetVisible(bool visible) { isVisible_ = visible; }
    bool IsVisible() const { return isVisible_; }
    
    void ToggleVisibility() { isVisible_ = !isVisible_; }
    
    // Event handlers
    void HandleKeyPress(int key);
    void HandleMouseMove(int x, int y);
    void HandleMouseClick(int button, int x, int y);
    
private:
    bool initialized_;
    bool isVisible_;
    
    Engine* engine_;
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    HWND hwnd_;
    
    void* imguiContext_;
};

} // namespace Nexus