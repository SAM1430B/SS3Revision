// Fullscreen toggle
//
// Alt+Enter triggers game command 103 (dispatcher 0x447650, case 0x44766E), which calls the
// game's fullscreen/windowed switch (0x441120 -> renderer 0x7A1FD0 -> swap chain 0x7AF430).
// The switch restores the window style and position saved at startup and resets the device
// as a windowed device. When a Direct3D 9 wrapper already presents the game in a window, the
// back buffer and the window no longer match, and the interface is drawn at a reduced size.
//
// When a Direct3D 9 wrapper is loaded, this feature ignores the command. Without a wrapper
// Alt+Enter keeps its original behaviour.

#include "Features.h"
#include "../Core/DisplayEnvironment.h"
#include "../Core/GameAddresses.h"
#include "../Core/Log.h"
#include "../Core/Memory.h"

namespace FullscreenToggle
{
    namespace
    {
        using ToggleFullscreenFn = void(__thiscall*)(void* app);

        uintptr_t g_toggleFullscreen = 0; // set in Install()

        // Replaces "call ToggleFullscreen" for game command 103.
        void __fastcall OnToggleFullscreen(void* app)
        {
            if (DisplayEnvironment::IsDirect3D9Wrapped())
            {
                static bool logged = false;
                if (!logged)
                {
                    Log::Write("FullscreenToggle: Alt+Enter ignored (Direct3D 9 wrapper loaded)");
                    logged = true;
                }
                return;
            }
            reinterpret_cast<ToggleFullscreenFn>(g_toggleFullscreen)(app);
        }
    }

    bool Install()
    {
        const GameAddresses& game = Game();

        // mov al,[ecx+0x270] ; test al,al ; jne <default> ; call ToggleFullscreen
        const bool toggleOk =
            Memory::Matches(game.toggleFullscreenCall - 10, { 0x8A, 0x81, 0x70, 0x02, 0x00, 0x00, 0x84, 0xC0, 0x75 }) &&
            Memory::Matches(game.toggleFullscreenCall, { 0xE8 }) &&
            Memory::RelativeTarget(game.toggleFullscreenCall) == game.toggleFullscreen &&
            Memory::Matches(game.toggleFullscreen, { 0x56, 0x8B, 0xF1, 0xC6, 0x86, 0x72, 0x02, 0x00, 0x00, 0x00 });
        if (!toggleOk)
        {
            Log::Write("FullscreenToggle: unexpected bytes, skipped");
            return false;
        }

        g_toggleFullscreen = game.toggleFullscreen;
        Memory::RedirectCall(game.toggleFullscreenCall, &OnToggleFullscreen);
        Log::Write("FullscreenToggle: Alt+Enter is ignored when a Direct3D 9 wrapper is loaded");
        return true;
    }
}
