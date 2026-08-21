#pragma once

#include <memory>
#include <string>

namespace Nexus {

/**
 * A single application window, backed by SDL2.
 *
 * The engine previously had no cross-platform window at all:
 * Platform::CreateGameWindow was declared, returned nullptr off Windows, and
 * the only real implementation was the Win32 code inside Engine.cpp. That is
 * what made "runs on Linux/macOS" untrue in practice even though the core
 * library compiled - there was nothing for a swap chain to attach to.
 *
 * Deliberately opaque: no SDL type appears in this header, so including it
 * costs nothing and public headers stay self-contained on builds without SDL.
 * GetNativeHandle() returns the SDL_Window*, which is what the Vulkan backend
 * needs for surface creation.
 */
class Window {
public:
    /// Which client API the window's surface must be compatible with. SDL needs
    /// to know at creation time; it cannot be changed afterwards.
    enum class Backend {
        None,       ///< Plain window, no 3D surface (tools, tests).
        Vulkan,
        OpenGL
    };

    /// The subset of keys the window layer reports directly. Anything richer
    /// belongs in InputManager; this exists so a host loop can honour ESC and
    /// basic navigation without a full input stack.
    enum class Key {
        Escape,
        Space,
        Enter,
        Tab,
        Left,
        Right,
        Up,
        Down,
        F1,
        F11
    };

    struct Desc {
        std::string title = "Nexus Engine";
        int width = 1280;
        int height = 720;
        bool fullscreen = false;
        bool resizable = true;

        /// Ask for a pixel-dense backing store where the display has one. On
        /// macOS this is the difference between a crisp window and a blurry
        /// upscaled one, and it makes GetDrawableSize() differ from GetSize().
        bool highDPI = true;

        /// Treat ESC as a close request. Convenient for demos and tools; off by
        /// default so a game can bind ESC to its own menu.
        bool closeOnEscape = false;

        Backend backend = Backend::Vulkan;
    };

    /// True when this build has a windowing backend compiled in at all.
    /// Callers can use it to fail with a useful message instead of a null check.
    static bool IsSupported();

    /// Returns nullptr on failure, having logged why.
    static std::unique_ptr<Window> Create(const Desc& desc);

    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /// The SDL_Window*, for backends that need it (Vulkan surface creation).
    void* GetNativeHandle() const;

    /// Drains the event queue for every open window and updates this window's
    /// state. Returns false once the window has been asked to close.
    bool PumpEvents();

    bool IsCloseRequested() const;
    void RequestClose();

    /// Logical window size, in the coordinate space the window manager uses.
    void GetSize(int& width, int& height) const;

    /// Size of the backing store in pixels. This - not GetSize - is what a swap
    /// chain must be created at: on a HiDPI display they differ by the display
    /// scale, and using the logical size yields a swap chain that is half
    /// resolution on macOS Retina and blurry everywhere it is scaled up.
    void GetDrawableSize(int& width, int& height) const;

    /// True once between a resize and the caller acknowledging it, so a host
    /// loop can recreate its swap chain exactly once per resize. Writes the new
    /// drawable size and clears the flag.
    bool ConsumeResize(int& width, int& height);

    /// A minimised window has a zero-sized drawable; creating a swap chain for
    /// one is invalid, so loops must skip rendering while this is true.
    bool IsMinimized() const;

    bool IsKeyDown(Key key) const;

    void SetTitle(const std::string& title);

    /// Opaque backend state. Declared here rather than in the private section
    /// only so the implementation file's own helpers can name the type; it is
    /// defined in Window.cpp and has no members callers can reach.
    struct Impl;

private:
    Window();

    std::unique_ptr<Impl> impl_;
};

} // namespace Nexus
