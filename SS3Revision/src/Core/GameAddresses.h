#pragma once

// Code and data locations in the supported game executables.
//
// Sudden Strike 3 (SS3Game.exe) and Sudden Strike 3 Last Stand (SSTLSGame.exe) are builds of
// the same version 1.4 code base, so every location exists in both at slightly different
// addresses. The running executable is identified by the TimeDateStamp in its PE header.
// Neither executable supports relocation, so both always load at 0x400000.
//
// Addresses mentioned in the comments of the feature files refer to SS3Game.exe.
// Every feature verifies the original bytes at its addresses before changing them.

#include <cstdint>

struct GameAddresses
{
    const char* name;
    uint32_t    peTimestamp;

    // Online game list
    uintptr_t gameListUrlHostA;                 // host part of the game report URL (with "&s=")
    uintptr_t gameListUrlHostB;                 // host part of the game report URL (without "&s=")

    // DirectPlay service provider lookup
    uintptr_t findTcpIpServiceProvider;         // NetPeer::FindTcpIpServiceProvider (thiscall, no arguments)
    uint32_t  findTcpIpServiceProviderSehScope; // operand of "push imm32" in its prologue

    // Single instance check
    uintptr_t alreadyRunningBranch;             // "je" after FindWindowW(L"Sudden Strike 3 GAME")

    // Resolution list
    uintptr_t minimumModeHeightPush;            // push 600
    uintptr_t minimumModeWidthPush;             // push 800
    uintptr_t afterModeListBuiltCall;           // call DisplayMode::DisplayMode() after the list is built
    uintptr_t displayModeDefaultConstructor;
    uintptr_t displayModeListPushBack;          // std::vector<DisplayMode>::push_back (thiscall)

    // Saved resolution at startup
    uintptr_t savedResolutionNotListed;         // start of the "reset to 1024x768" branch
    uintptr_t applySavedResolution;             // cvttss2si eax,[ebp+0x324] ; cvttss2si ecx,[ebp+0x320]
    uintptr_t applySavedResolutionResume;
    uintptr_t floatConstant1024;                // 1024.0f, read by the reset branch

    // Interface scale
    uintptr_t interfaceScaleStore;              // cvtsi2ss / divss / movss [interfaceScale]
    uintptr_t interfaceScaleResume;
    uintptr_t interfaceScale;                   // float

    // Window style of the fullscreen window
    uintptr_t fullscreenWindowStyleCall;        // push 0x90080000 ; push GWL_STYLE ; push eax ; call edi

    // Window resize (WM_SIZE)
    uintptr_t windowSizedCall;                  // call windowSizedHandler, in the window procedure
    uintptr_t windowSizedHandler;               // resizes the back buffer to the client area

    // Fullscreen toggle (Alt+Enter)
    uintptr_t toggleFullscreenCall;             // call toggleFullscreen, for game command 103
    uintptr_t toggleFullscreen;                 // the game's fullscreen/windowed switch (thiscall)
};

// Structure offsets, identical in both games.
namespace GameOffsets
{
    constexpr uint32_t NetPeerServiceProviderGuid = 0x7C;  // GUID stored by FindTcpIpServiceProvider
    constexpr uint32_t ModeListOnStack            = 0x4C;  // mode list, relative to esp at afterModeListBuiltCall
    constexpr uint32_t SettingsResolutionWidth    = 0x320; // float in the settings object
    constexpr uint32_t SettingsResolutionHeight   = 0x324; // float in the settings object
    constexpr uint32_t Renderer                   = 0x170; // renderer object in the application object
    constexpr uint32_t GetScreenInfoVtable        = 0x44;  // renderer method returning the screen size
    constexpr uint32_t AppWindow                  = 0x2AC; // HWND in the application object
}

// Identifies the running executable by its PE timestamp. Returns nullptr for an unknown executable.
const GameAddresses* DetectGame();

// The detected game. SetGame is called once by DllMain before any feature is installed.
const GameAddresses& Game();
void SetGame(const GameAddresses* game);

// TimeDateStamp from the PE header of the running executable.
uint32_t RunningExeTimestamp();
