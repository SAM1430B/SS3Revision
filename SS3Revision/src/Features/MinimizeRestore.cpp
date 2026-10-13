// Minimize and restore
//
// The window procedure (0x445830) handles WM_SIZE by calling 0x440CE0, which resizes the
// Direct3D back buffer to the client area and resets the device. Windows sends WM_SIZE with
// a 0x0 client area when the window is minimized, before the game is deactivated, so the game
// attempts a device reset with a 0x0 back buffer. The reset fails and the window cannot be
// restored.
//
// This feature skips the resize while the window is minimized or has an empty client area.
// The back buffer keeps its size, and the game pauses and resumes through its own activation
// handling.

#include "Features.h"
#include "../Core/GameAddresses.h"
#include "../Core/Log.h"
#include "../Core/Memory.h"

#include <windows.h>

namespace MinimizeRestore
{
    namespace
    {
        using WindowSizedFn = void(__thiscall*)(void* app);

        uintptr_t g_next = 0; // previous target of the WM_SIZE call, set in Install()

        bool HasEmptyClientArea(HWND window)
        {
            if (IsIconic(window))
                return true;

            RECT client{};
            if (!GetClientRect(window, &client))
                return false;
            return client.right - client.left <= 0 || client.bottom - client.top <= 0;
        }

        // Replaces the thiscall WM_SIZE handler call. 'app' is passed in ecx and there are no
        // stack arguments, which matches __fastcall with a single parameter.
        void __fastcall OnWindowSized(unsigned char* app)
        {
            const HWND window = *reinterpret_cast<HWND*>(app + GameOffsets::AppWindow);
            if (window && HasEmptyClientArea(window))
            {
                static bool logged = false;
                if (!logged)
                {
                    Log::Write("MinimizeRestore: window minimized, back buffer size kept");
                    logged = true;
                }
                return;
            }
            reinterpret_cast<WindowSizedFn>(g_next)(app);
        }
    }

    bool Install()
    {
        const GameAddresses& game = Game();

        // The call may already be redirected by MaximizeWindow; its current target is chained.
        const bool callOk    = Memory::Matches(game.windowSizedCall, { 0xE8 });
        const bool handlerOk = Memory::Matches(game.windowSizedHandler, { 0x56, 0x8B, 0xF1, 0x8A, 0x86, 0x72, 0x02, 0x00, 0x00 });
        if (!callOk || !handlerOk)
        {
            Log::Write("MinimizeRestore: unexpected bytes, skipped");
            return false;
        }

        g_next = Memory::RelativeTarget(game.windowSizedCall);
        Memory::RedirectCall(game.windowSizedCall, &OnWindowSized);
        Log::Write("MinimizeRestore: back buffer is not resized while the window is minimized");
        return true;
    }
}
