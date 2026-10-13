#pragma once

// Queries about the display setup the game runs in.
//
// The game always requests an exclusive fullscreen Direct3D 9 device. A Direct3D 9 wrapper
// can present that device in a window, in which case any size can be used.
// Without a wrapper, only the fullscreen modes reported by the monitor are valid.
namespace DisplayEnvironment
{
    // True when d3d9.dll was loaded from outside the Windows system folders, which indicates a
    // Direct3D 9 wrapper in the game folder. The result is cached once d3d9.dll is loaded.
    bool IsDirect3D9Wrapped();

    // True when the default adapter reports the size as a 32-bit (X8R8G8B8) fullscreen mode.
    // Returns true when the check cannot be performed.
    bool IsFullscreenModeSupported(int width, int height);
}
