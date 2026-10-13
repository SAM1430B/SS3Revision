// Interface scaling
//
// Menus and panels are laid out on a 1024x768 grid, or 1280x1024 when the high-resolution art
// is used (window at least 1280x1024). The game computes the global interface scale at
// 0x440F82 from the window height only:
//     scale = height / 768   (or height / 1024 with the high-resolution art)
// For windows narrower than 4:3 the interface is then wider than the window.
//
// This feature uses the smaller of the two ratios, so the interface fits in both directions:
//     scale = min(height / baseHeight, width / baseWidth)
// For 4:3 and wider windows the result is unchanged.

#include "Features.h"
#include "../Core/GameAddresses.h"
#include "../Core/Log.h"
#include "../Core/Memory.h"

#include <algorithm>
#include <cstdint>

namespace InterfaceScaling
{
    namespace
    {
        // Returned by the renderer method at vtable offset GameOffsets::GetScreenInfoVtable.
        struct ScreenInfo
        {
            int32_t unknown0;
            int32_t unknown1;
            int32_t width;
            int32_t height;
        };
        using GetScreenInfoFn = const ScreenInfo*(__thiscall*)(void* self);

        uintptr_t g_resume         = 0;       // set in Install()
        float*    g_interfaceScale = nullptr; // set in Install()

        void __cdecl StoreInterfaceScale(unsigned char* self, int height, int highResArt)
        {
            const float baseHeight = highResArt ? 1024.0f : 768.0f;
            const float baseWidth  = highResArt ? 1280.0f : 1024.0f;
            float scale = static_cast<float>(height) / baseHeight;

            void* renderer = *reinterpret_cast<void**>(self + GameOffsets::Renderer);
            if (renderer)
            {
                void** vtable = *reinterpret_cast<void***>(renderer);
                const auto getScreenInfo = reinterpret_cast<GetScreenInfoFn>(
                    vtable[GameOffsets::GetScreenInfoVtable / sizeof(void*)]);
                const ScreenInfo* info = getScreenInfo(renderer);
                if (info && info->width > 0)
                    scale = std::min(scale, static_cast<float>(info->width) / baseWidth);
            }

            *g_interfaceScale = scale;
        }

        // Replaces the 16 bytes at interfaceScaleStore (cvtsi2ss / divss / movss [scale]).
        // eax = window height, edi = 1 when the high-resolution art is used, esi = this.
        __declspec(naked) void OnStoreInterfaceScale()
        {
            __asm
            {
                pushad
                push edi
                push eax
                push esi
                call StoreInterfaceScale
                add  esp, 12
                popad
                jmp  dword ptr [g_resume]
            }
        }
    }

    bool Install()
    {
        const GameAddresses& game = Game();

        // cvtsi2ss xmm1,eax ; divss xmm1,xmm0 ; movss [interfaceScale],xmm1
        if (!Memory::MatchesWithValue(game.interfaceScaleStore,
                                      { 0xF3, 0x0F, 0x2A, 0xC8, 0xF3, 0x0F, 0x5E, 0xC8, 0xF3, 0x0F, 0x11, 0x0D },
                                      static_cast<uint32_t>(game.interfaceScale)))
        {
            Log::Write("InterfaceScaling: unexpected bytes, skipped");
            return false;
        }

        g_resume         = game.interfaceScaleResume;
        g_interfaceScale = reinterpret_cast<float*>(game.interfaceScale);
        Memory::WriteJump(game.interfaceScaleStore, &OnStoreInterfaceScale, 16);
        Log::Write("InterfaceScaling: scale = min(height, width) fit");
        return true;
    }
}
