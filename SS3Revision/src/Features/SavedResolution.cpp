// Saved resolution
//
// The resolution chosen in the options menu is stored in the player profile. At startup
// (0x44434C) the game compares it with the display modes reported by EnumDisplaySettingsW and,
// when it is not listed, resets it to 1024x768 (0x4443CF). Custom sizes are never listed.
//
// This feature:
//   * skips the reset, so an unlisted resolution is kept,
//   * applies [Resolution] Force, when set, regardless of the profile, and
//   * without a Direct3D 9 wrapper, falls back to 1024x768 when the size is not a fullscreen
//     mode of the monitor, because the exclusive fullscreen device cannot be created otherwise.
//
// The size is adjusted immediately before the game reads it to create the device (0x44443F).

#include "Features.h"
#include "../Core/Config.h"
#include "../Core/DisplayEnvironment.h"
#include "../Core/GameAddresses.h"
#include "../Core/Log.h"
#include "../Core/Memory.h"

namespace SavedResolution
{
    namespace
    {
        constexpr int kFallbackWidth  = 1024; // the game's default resolution
        constexpr int kFallbackHeight = 768;

        uintptr_t g_resume = 0; // set in Install()

        void __cdecl ApplyStartupResolution(unsigned char* settings)
        {
            float& width  = *reinterpret_cast<float*>(settings + GameOffsets::SettingsResolutionWidth);
            float& height = *reinterpret_cast<float*>(settings + GameOffsets::SettingsResolutionHeight);

            const Resolution& forced = GetConfig().forced;
            if (forced.IsValid())
            {
                width  = static_cast<float>(forced.width);
                height = static_cast<float>(forced.height);
            }

            const int w = static_cast<int>(width);
            const int h = static_cast<int>(height);
            if (DisplayEnvironment::IsDirect3D9Wrapped())
            {
                Log::Write("SavedResolution: starting at %dx%d (Direct3D 9 wrapper loaded, any size allowed)", w, h);
                return;
            }

            if (DisplayEnvironment::IsFullscreenModeSupported(w, h))
            {
                Log::Write("SavedResolution: starting at %dx%d (fullscreen)", w, h);
                return;
            }

            Log::Write("SavedResolution: %dx%d is not a fullscreen mode of this monitor and no Direct3D 9 "
                       "wrapper is loaded, using %dx%d", w, h, kFallbackWidth, kFallbackHeight);
            width  = static_cast<float>(kFallbackWidth);
            height = static_cast<float>(kFallbackHeight);
        }

        // Replaces the 16 bytes at applySavedResolution. ebp points to the settings object.
        __declspec(naked) void BeforeApplySavedResolution()
        {
            __asm
            {
                pushad
                push ebp
                call ApplyStartupResolution
                add  esp, 4
                popad
                // Original instructions
                cvttss2si eax, dword ptr [ebp + 0x324]
                cvttss2si ecx, dword ptr [ebp + 0x320]
                jmp  dword ptr [g_resume]
            }
        }
        static_assert(GameOffsets::SettingsResolutionWidth == 0x320 && GameOffsets::SettingsResolutionHeight == 0x324,
                      "BeforeApplySavedResolution uses the offsets directly");
    }

    bool Install()
    {
        const Config& config = GetConfig();
        const GameAddresses& game = Game();

        // movss xmm0, [1024.0f] (start of the reset branch)
        const bool resetOk = Memory::MatchesWithValue(game.savedResolutionNotListed, { 0xF3, 0x0F, 0x10, 0x05 },
                                                      static_cast<uint32_t>(game.floatConstant1024));
        // cvttss2si eax,[ebp+0x324] ; cvttss2si ecx,[ebp+0x320]
        const bool applyOk = Memory::Matches(game.applySavedResolution,
                                             { 0xF3, 0x0F, 0x2C, 0x85, 0x24, 0x03, 0x00, 0x00,
                                               0xF3, 0x0F, 0x2C, 0x8D, 0x20, 0x03, 0x00, 0x00 });
        // The short jump written below must land on applySavedResolution.
        const bool distanceOk = game.savedResolutionNotListed + 2 + 0x6E == game.applySavedResolution;
        if (!resetOk || !applyOk || !distanceOk)
        {
            Log::Write("SavedResolution: unexpected bytes, skipped");
            return false;
        }

        // jmp short applySavedResolution
        const unsigned char keepSaved[] = { 0xEB, 0x6E };
        Memory::Write(game.savedResolutionNotListed, keepSaved, sizeof(keepSaved));

        g_resume = game.applySavedResolutionResume;
        Memory::WriteJump(game.applySavedResolution, &BeforeApplySavedResolution, 16);

        if (config.forced.IsValid())
            Log::Write("SavedResolution: unlisted resolutions kept, forcing %dx%d", config.forced.width, config.forced.height);
        else
            Log::Write("SavedResolution: unlisted resolutions kept");
        return true;
    }
}
