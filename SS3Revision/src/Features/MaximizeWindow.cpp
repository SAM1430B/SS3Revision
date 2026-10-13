// Maximize window
//
// When the window is maximized, the game resizes its back buffer to the new client area and
// resets the device (WM_SIZE handler 0x440CE0). Because the game treats its window as
// fullscreen, a Direct3D 9 wrapper fits the window to the back buffer during
// the reset. A screen-wide back buffer with a frame does not fit on the screen, so the wrapper
// removes the frame and moves the window. The resize also replaces the window size that Windows
// restores later.
//
// When a Direct3D 9 wrapper is loaded, this feature:
//   * after the reset of a maximized window, restores the previous frame and places the window
//     so that its client area matches the back buffer, and
//   * on restore, returns the window to its placement from before maximizing before the game
//     reads the new size, so the game returns to its previous resolution.
// Without a wrapper the game's behaviour is unchanged.

#include "Features.h"
#include "../Core/DisplayEnvironment.h"
#include "../Core/GameAddresses.h"
#include "../Core/Log.h"
#include "../Core/Memory.h"

#include <windows.h>

namespace MaximizeWindow
{
    namespace
    {
        constexpr LONG kFrameBits = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

        using WindowSizedFn = void(__thiscall*)(void* app);

        uintptr_t g_next = 0;     // previous target of the WM_SIZE call, set in Install()
        bool g_adjusting = false; // set while this feature moves the window

        bool g_restorePending = false;
        WINDOWPLACEMENT g_normalPlacement{};

        RECT ClientRectOnScreen(HWND window)
        {
            RECT rect{};
            GetClientRect(window, &rect);
            MapWindowPoints(window, nullptr, reinterpret_cast<POINT*>(&rect), 2);
            return rect;
        }

        // Window restored: returns it to the placement saved when it was maximized.
        void RestoreNormalPlacement(HWND window)
        {
            g_restorePending = false;

            WINDOWPLACEMENT placement = g_normalPlacement;
            placement.length  = sizeof(placement);
            placement.flags   = 0;
            placement.showCmd = SW_SHOWNORMAL;

            g_adjusting = true;
            SetWindowPlacement(window, &placement);
            g_adjusting = false;

            const RECT& bounds = placement.rcNormalPosition;
            Log::Write("MaximizeWindow: restored to %dx%d (window size including frame)",
                       static_cast<int>(bounds.right - bounds.left), static_cast<int>(bounds.bottom - bounds.top));
        }

        // Window maximized: resizes the back buffer, then repairs the frame and position changed
        // by the wrapper during the device reset.
        void ResizeMaximized(unsigned char* app, HWND window, WindowSizedFn next)
        {
            if (!g_restorePending)
            {
                WINDOWPLACEMENT placement{};
                placement.length = sizeof(placement);
                if (GetWindowPlacement(window, &placement))
                {
                    g_normalPlacement = placement;
                    g_restorePending  = true;
                }
            }

            const LONG styleBefore  = GetWindowLongW(window, GWL_STYLE);
            const RECT clientBefore = ClientRectOnScreen(window);

            next(app);

            const LONG style       = GetWindowLongW(window, GWL_STYLE);
            const RECT clientAfter = ClientRectOnScreen(window);
            const LONG frame       = styleBefore & kFrameBits;
            if ((style & kFrameBits) == frame && EqualRect(&clientAfter, &clientBefore))
                return;

            const LONG fixedStyle = (style & ~(kFrameBits | WS_MAXIMIZE)) | frame | (styleBefore & WS_MAXIMIZE);
            RECT bounds = clientBefore;
            AdjustWindowRectEx(&bounds, fixedStyle, FALSE, GetWindowLongW(window, GWL_EXSTYLE));

            g_adjusting = true;
            SetWindowLongW(window, GWL_STYLE, fixedStyle);
            SetWindowPos(window, nullptr, bounds.left, bounds.top,
                         bounds.right - bounds.left, bounds.bottom - bounds.top,
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            g_adjusting = false;

            Log::Write("MaximizeWindow: maximized to %dx%d, window frame restored",
                       static_cast<int>(clientBefore.right - clientBefore.left),
                       static_cast<int>(clientBefore.bottom - clientBefore.top));
        }

        // Replaces the thiscall WM_SIZE handler call. 'app' is passed in ecx and there are no
        // stack arguments, which matches __fastcall with a single parameter.
        void __fastcall OnWindowSized(unsigned char* app)
        {
            // WM_SIZE sent by this feature's own window changes; the back buffer already matches.
            if (g_adjusting)
                return;

            const auto next = reinterpret_cast<WindowSizedFn>(g_next);
            const HWND window = *reinterpret_cast<HWND*>(app + GameOffsets::AppWindow);
            if (!window || !DisplayEnvironment::IsDirect3D9Wrapped())
            {
                next(app);
                return;
            }

            if (IsZoomed(window))
            {
                ResizeMaximized(app, window, next);
                return;
            }

            if (g_restorePending && !IsIconic(window))
                RestoreNormalPlacement(window);
            next(app);
        }
    }

    bool Install()
    {
        const GameAddresses& game = Game();

        // The call may already be redirected by MinimizeRestore; its current target is chained.
        const bool callOk    = Memory::Matches(game.windowSizedCall, { 0xE8 });
        const bool handlerOk = Memory::Matches(game.windowSizedHandler, { 0x56, 0x8B, 0xF1, 0x8A, 0x86, 0x72, 0x02, 0x00, 0x00 });
        if (!callOk || !handlerOk)
        {
            Log::Write("MaximizeWindow: unexpected bytes, skipped");
            return false;
        }

        g_next = Memory::RelativeTarget(game.windowSizedCall);
        Memory::RedirectCall(game.windowSizedCall, &OnWindowSized);
        Log::Write("MaximizeWindow: maximize and restore handled for Direct3D 9 wrappers");
        return true;
    }
}
