// Resolution list
//
// The options menu builds its resolution list at 0x43B17E. A reference mode of 800x600
// (format 3, X8R8G8B8) is constructed, and 0x788D50 fills a std::vector<DisplayMode> from
// IDirect3D9::EnumAdapterModes, keeping only modes that are at least as large as the reference.
// Direct3D 9 does not report custom resolutions created in the graphics driver.
//
// This feature lowers the reference mode to [Resolution] Minimum and appends the entries of
// [Resolution] Modes to the list. The entries are added through the call to the DisplayMode
// default constructor that follows the list construction (0x43B1AC). They are only added when
// a Direct3D 9 wrapper is loaded, because an exclusive fullscreen device accepts only the modes
// reported by the monitor.

#include "Features.h"
#include "../Core/Config.h"
#include "../Core/DisplayEnvironment.h"
#include "../Core/GameAddresses.h"
#include "../Core/Log.h"
#include "../Core/Memory.h"

#include <cstdint>

namespace ResolutionList
{
    namespace
    {
        // Display mode entry used by the game (20 bytes).
        struct DisplayMode
        {
            int32_t width;
            int32_t height;
            int32_t format;      // 3 = X8R8G8B8
            int32_t reserved;
            int32_t refreshRate; // 0 = any
        };
        static_assert(sizeof(DisplayMode) == 20, "DisplayMode must be 20 bytes");

        using PushBackFn = void(__thiscall*)(void* list, const DisplayMode* mode);

        uintptr_t g_displayModeDefaultConstructor = 0; // set in Install()
        uintptr_t g_displayModeListPushBack       = 0; // set in Install()

        void __cdecl AddExtraModes(void* modeList)
        {
            if (GetConfig().extraModes.empty())
                return;

            if (!DisplayEnvironment::IsDirect3D9Wrapped())
            {
                static bool logged = false;
                if (!logged)
                {
                    Log::Write("ResolutionList: no Direct3D 9 wrapper loaded, additional modes not listed");
                    logged = true;
                }
                return;
            }

            const auto pushBack = reinterpret_cast<PushBackFn>(g_displayModeListPushBack);
            for (const Resolution& resolution : GetConfig().extraModes)
            {
                const DisplayMode mode{ resolution.width, resolution.height, 3, 0, 0 };
                pushBack(modeList, &mode);
            }
        }

        // Replaces the target of "call DisplayMode::DisplayMode" at afterModeListBuilt.
        // On entry, [esp] holds the return address, ecx the DisplayMode being constructed, and
        // the mode list is located at [esp + GameOffsets::ModeListOnStack].
        __declspec(naked) void AfterModeListBuilt()
        {
            __asm
            {
                pushad
                lea  eax, [esp + 0x20 + 0x4C]   // 0x20 = size of the pushad frame
                push eax
                call AddExtraModes
                add  esp, 4
                popad
                jmp  dword ptr [g_displayModeDefaultConstructor]
            }
        }
        static_assert(GameOffsets::ModeListOnStack == 0x4C, "AfterModeListBuilt uses the offset directly");
    }

    bool Install()
    {
        const Config& config = GetConfig();
        const GameAddresses& game = Game();

        // push 600 ; push 800
        const bool minimumOk =
            Memory::Matches(game.minimumModeHeightPush, { 0x68, 0x58, 0x02, 0x00, 0x00 }) &&
            Memory::Matches(game.minimumModeWidthPush,  { 0x68, 0x20, 0x03, 0x00, 0x00 });
        // call DisplayMode::DisplayMode
        const bool callOk =
            Memory::Matches(game.afterModeListBuiltCall, { 0xE8 }) &&
            Memory::RelativeTarget(game.afterModeListBuiltCall) == game.displayModeDefaultConstructor;

        if (!minimumOk || !callOk)
        {
            Log::Write("ResolutionList: unexpected bytes, skipped");
            return false;
        }

        const int32_t minHeight = config.minimum.height;
        const int32_t minWidth  = config.minimum.width;
        Memory::WriteValue(game.minimumModeHeightPush + 1, minHeight);
        Memory::WriteValue(game.minimumModeWidthPush + 1, minWidth);

        g_displayModeDefaultConstructor = game.displayModeDefaultConstructor;
        g_displayModeListPushBack       = game.displayModeListPushBack;
        Memory::RedirectCall(game.afterModeListBuiltCall, &AfterModeListBuilt);

        Log::Write("ResolutionList: minimum %dx%d, %u additional mode(s)", minWidth, minHeight,
                   static_cast<unsigned>(config.extraModes.size()));
        for (const Resolution& resolution : config.extraModes)
            Log::Write("ResolutionList:   + %dx%d", resolution.width, resolution.height);
        return true;
    }
}
