#include "Window.h"

#include "Logger.h"
#include "SDLVulkanCompat.h"

#include <algorithm>
#include <vector>

namespace Nexus {

#ifdef NEXUS_SDL2_ENABLED

struct Window::Impl {
    SDL_Window* window = nullptr;
    Uint32 windowID = 0;
    bool closeRequested = false;
    bool closeOnEscape = false;
    bool minimized = false;
    bool resizePending = false;
};

namespace {

// Every live window, so PumpEvents can route an event to the window it names.
// SDL delivers one queue for the whole process, so a window that drained only
// the events addressed to itself would still consume the others' events -
// reading an event removes it for everyone - and they would never be seen.
std::vector<Window::Impl*>& LiveWindows() {
    static std::vector<Window::Impl*> windows;
    return windows;
}

// SDL refuses to create a window before its video subsystem is up. Doing it
// here rather than requiring a Platform::Initialize call first means a tool or
// test can open a window without booting the whole engine; SDL refcounts
// subsystem init, so pairing each success with a quit stays correct.
bool EnsureVideoSubsystem() {
    if (SDL_WasInit(SDL_INIT_VIDEO) != 0) {
        return true;
    }
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        Logger::Error("SDL video initialisation failed: " + std::string(SDL_GetError()));
        return false;
    }
    return true;
}

int ToScancode(Window::Key key) {
    switch (key) {
        case Window::Key::Escape: return SDL_SCANCODE_ESCAPE;
        case Window::Key::Space:  return SDL_SCANCODE_SPACE;
        case Window::Key::Enter:  return SDL_SCANCODE_RETURN;
        case Window::Key::Tab:    return SDL_SCANCODE_TAB;
        case Window::Key::Left:   return SDL_SCANCODE_LEFT;
        case Window::Key::Right:  return SDL_SCANCODE_RIGHT;
        case Window::Key::Up:     return SDL_SCANCODE_UP;
        case Window::Key::Down:   return SDL_SCANCODE_DOWN;
        case Window::Key::F1:     return SDL_SCANCODE_F1;
        case Window::Key::F11:    return SDL_SCANCODE_F11;
    }
    return SDL_SCANCODE_UNKNOWN;
}

} // namespace

bool Window::IsSupported() {
    return true;
}

std::unique_ptr<Window> Window::Create(const Desc& desc) {
    if (!EnsureVideoSubsystem()) {
        return nullptr;
    }

    Uint32 flags = SDL_WINDOW_SHOWN;
    if (desc.resizable)  flags |= SDL_WINDOW_RESIZABLE;
    if (desc.highDPI)    flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    if (desc.fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    switch (desc.backend) {
        case Backend::Vulkan: flags |= SDL_WINDOW_VULKAN; break;
        case Backend::OpenGL: flags |= SDL_WINDOW_OPENGL; break;
        case Backend::None:   break;
    }

    SDL_Window* sdlWindow = SDL_CreateWindow(
        desc.title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        std::max(1, desc.width),
        std::max(1, desc.height),
        flags);

    if (!sdlWindow) {
        // Worth naming the backend: "Vulkan window creation failed" on a machine
        // with no Vulkan ICD is a very different problem from SDL not having a
        // video driver, and the SDL message distinguishes them.
        Logger::Error("Failed to create window: " + std::string(SDL_GetError()));
        return nullptr;
    }

    std::unique_ptr<Window> window(new Window());
    window->impl_->window = sdlWindow;
    window->impl_->windowID = SDL_GetWindowID(sdlWindow);
    window->impl_->closeOnEscape = desc.closeOnEscape;

    LiveWindows().push_back(window->impl_.get());

    int drawableWidth = 0;
    int drawableHeight = 0;
    window->GetDrawableSize(drawableWidth, drawableHeight);
    Logger::Info("Created " + std::to_string(desc.width) + "x" + std::to_string(desc.height) +
                 " window (drawable " + std::to_string(drawableWidth) + "x" +
                 std::to_string(drawableHeight) + ")");

    return window;
}

Window::Window() : impl_(new Impl()) {}

Window::~Window() {
    auto& windows = LiveWindows();
    windows.erase(std::remove(windows.begin(), windows.end(), impl_.get()), windows.end());

    if (impl_->window) {
        SDL_DestroyWindow(impl_->window);
        impl_->window = nullptr;
    }

    // Matching the SDL_InitSubSystem in Create keeps SDL's refcount balanced, so
    // closing one window in a multi-window process does not tear down video for
    // the others.
    if (windows.empty() && SDL_WasInit(SDL_INIT_VIDEO) != 0) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
}

void* Window::GetNativeHandle() const {
    return impl_->window;
}

bool Window::PumpEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                // A quit request is addressed to the application, not to one
                // window, so every window sees it.
                for (Impl* impl : LiveWindows()) {
                    impl->closeRequested = true;
                }
                break;

            case SDL_WINDOWEVENT: {
                for (Impl* impl : LiveWindows()) {
                    if (impl->windowID != event.window.windowID) {
                        continue;
                    }
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_CLOSE:
                            impl->closeRequested = true;
                            break;
                        case SDL_WINDOWEVENT_MINIMIZED:
                            impl->minimized = true;
                            break;
                        case SDL_WINDOWEVENT_RESTORED:
                        case SDL_WINDOWEVENT_MAXIMIZED:
                            impl->minimized = false;
                            impl->resizePending = true;
                            break;
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            // SIZE_CHANGED rather than RESIZED: the former also
                            // fires when the window moves to a display with a
                            // different scale factor, which changes the drawable
                            // size without changing the logical size. A swap
                            // chain keyed off the logical size alone goes stale
                            // there and renders at the wrong resolution.
                            impl->resizePending = true;
                            impl->minimized = false;
                            break;
                        default:
                            break;
                    }
                }
                break;
            }

            case SDL_KEYDOWN: {
                if (event.key.keysym.scancode != SDL_SCANCODE_ESCAPE) {
                    break;
                }
                for (Impl* impl : LiveWindows()) {
                    if (impl->closeOnEscape && impl->windowID == event.key.windowID) {
                        impl->closeRequested = true;
                    }
                }
                break;
            }

            default:
                break;
        }
    }

    return !impl_->closeRequested;
}

bool Window::IsCloseRequested() const {
    return impl_->closeRequested;
}

void Window::RequestClose() {
    impl_->closeRequested = true;
}

void Window::GetSize(int& width, int& height) const {
    width = 0;
    height = 0;
    if (impl_->window) {
        SDL_GetWindowSize(impl_->window, &width, &height);
    }
}

void Window::GetDrawableSize(int& width, int& height) const {
    width = 0;
    height = 0;
    if (!impl_->window) {
        return;
    }

    const Uint32 flags = SDL_GetWindowFlags(impl_->window);
    if (flags & SDL_WINDOW_VULKAN) {
        SDL_Vulkan_GetDrawableSize(impl_->window, &width, &height);
    } else if (flags & SDL_WINDOW_OPENGL) {
        SDL_GL_GetDrawableSize(impl_->window, &width, &height);
    } else {
        SDL_GetWindowSize(impl_->window, &width, &height);
    }
}

bool Window::ConsumeResize(int& width, int& height) {
    if (!impl_->resizePending) {
        return false;
    }
    impl_->resizePending = false;
    GetDrawableSize(width, height);
    return true;
}

bool Window::IsMinimized() const {
    if (impl_->minimized) {
        return true;
    }

    // A window can be zero-sized without ever emitting MINIMIZED - tiling window
    // managers and the moment between a resize starting and finishing both do
    // it - and a zero-extent swap chain is invalid, so treat that as minimised
    // too rather than letting the caller create one.
    int width = 0;
    int height = 0;
    GetDrawableSize(width, height);
    return width <= 0 || height <= 0;
}

bool Window::IsKeyDown(Key key) const {
    const int scancode = ToScancode(key);
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        return false;
    }

    int numKeys = 0;
    const Uint8* state = SDL_GetKeyboardState(&numKeys);
    if (!state || scancode >= numKeys) {
        return false;
    }
    return state[scancode] != 0;
}

void Window::SetTitle(const std::string& title) {
    if (impl_->window) {
        SDL_SetWindowTitle(impl_->window, title.c_str());
    }
}

#else // !NEXUS_SDL2_ENABLED

// Without SDL there is no windowing backend. Every entry point still exists and
// behaves predictably, so callers get one clear diagnostic at creation time
// rather than a null dereference somewhere downstream.

struct Window::Impl {};

bool Window::IsSupported() {
    return false;
}

std::unique_ptr<Window> Window::Create(const Desc&) {
    Logger::Error("No windowing backend: this build was configured without SDL2. "
                  "Install SDL2 development headers and reconfigure to open a window.");
    return nullptr;
}

Window::Window() : impl_(new Impl()) {}
Window::~Window() = default;

void* Window::GetNativeHandle() const { return nullptr; }
bool Window::PumpEvents() { return false; }
bool Window::IsCloseRequested() const { return true; }
void Window::RequestClose() {}
void Window::GetSize(int& width, int& height) const { width = 0; height = 0; }
void Window::GetDrawableSize(int& width, int& height) const { width = 0; height = 0; }
bool Window::ConsumeResize(int&, int&) { return false; }
bool Window::IsMinimized() const { return true; }
bool Window::IsKeyDown(Key) const { return false; }
void Window::SetTitle(const std::string&) {}

#endif // NEXUS_SDL2_ENABLED

} // namespace Nexus
