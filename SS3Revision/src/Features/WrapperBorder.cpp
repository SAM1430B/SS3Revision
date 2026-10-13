// Wrapper border
//
// The game treats its window as a fullscreen window. After the Direct3D device has been
// created, 0x78A060 sets the window style to WS_POPUP | WS_VISIBLE | WS_SYSMENU (0x78A08A),
// which removes any frame that a Direct3D 9 wrapper window mode applied
// while the device was created. The window keeps its framed size, so the image is stretched
// and the cursor position no longer matches.
//
// When a Direct3D 9 wrapper is loaded, this feature applies the game's style together with
// the frame bits already present on the window. The wrapper therefore decides whether the
// window has a frame; this feature does not add one. Without a wrapper the style is unchanged.

#include "Features.h"
#include "../Core/DisplayEnvironment.h"
#include "../Core/GameAddresses.h"
#include "../Core/Log.h"
#include "../Core/Memory.h"

#include <windows.h>

namespace WrapperBorder
{
    namespace
    {
        constexpr LONG kGameStyle = static_cast<LONG>(WS_POPUP | WS_VISIBLE | WS_SYSMENU); // 0x90080000
        constexpr LONG kFrameBits = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

        // Replaces "push 0x90080000 ; push GWL_STYLE ; push eax ; call edi (SetWindowLongW)".
        // Called through "push eax ; call" with eax = game window. The game reloads eax, ecx and
        // edx after the original call, so they need not be preserved.
        void __stdcall ApplyGameWindowStyle(HWND window)
        {
            LONG style = kGameStyle;
            if (DisplayEnvironment::IsDirect3D9Wrapped())
            {
                const LONG frame = GetWindowLongW(window, GWL_STYLE) & kFrameBits;
                style |= frame;

                static bool logged = false;
                if (!logged)
                {
                    Log::Write(frame ? "WrapperBorder: window frame of the Direct3D 9 wrapper kept (style %08X)"
                                     : "WrapperBorder: Direct3D 9 wrapper window has no frame (style %08X)",
                               static_cast<unsigned>(style));
                    logged = true;
                }
            }
            SetWindowLongW(window, GWL_STYLE, style);
        }
    }

    bool Install()
    {
        const uintptr_t site = Game().fullscreenWindowStyleCall;

        // push 0x90080000 ; push -16 (GWL_STYLE) ; push eax ; call edi
        if (!Memory::Matches(site, { 0x68, 0x00, 0x00, 0x08, 0x90, 0x6A, 0xF0, 0x50, 0xFF, 0xD7 }))
        {
            Log::Write("WrapperBorder: unexpected bytes, skipped");
            return false;
        }

        // push eax ; call ApplyGameWindowStyle ; nop x4
        const unsigned char pushWindow = 0x50;
        Memory::Write(site, &pushWindow, 1);
        Memory::WriteCall(site + 1, &ApplyGameWindowStyle, 9);
        Log::Write("WrapperBorder: the window frame is left to the Direct3D 9 wrapper");
        return true;
    }
}
