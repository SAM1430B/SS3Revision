#include "GameAddresses.h"

#include <windows.h>

namespace
{
    // Sudden Strike 3 - SS3Game.exe 1.4, MD5 a3765f4c5f563574f811ae1dd4066296
    constexpr GameAddresses kSuddenStrike3 =
    {
        "Sudden Strike 3",
        0x48FDC221,

        0x86811F,   // gameListUrlHostA
        0x8685D7,   // gameListUrlHostB

        0x4E6C20,   // findTcpIpServiceProvider
        0x803466,   // findTcpIpServiceProviderSehScope

        0x463E84,   // alreadyRunningBranch

        0x43B181,   // minimumModeHeightPush
        0x43B186,   // minimumModeWidthPush
        0x43B1AC,   // afterModeListBuiltCall
        0x787630,   // displayModeDefaultConstructor
        0x788720,   // displayModeListPushBack

        0x4443CF,   // savedResolutionNotListed
        0x44443F,   // applySavedResolution
        0x44444F,   // applySavedResolutionResume
        0x85FAFC,   // floatConstant1024

        0x440F82,   // interfaceScaleStore
        0x440F92,   // interfaceScaleResume
        0xA59858,   // interfaceScale

        0x78A08A,   // fullscreenWindowStyleCall

        0x445C9A,   // windowSizedCall
        0x440CE0,   // windowSizedHandler

        0x447678,   // toggleFullscreenCall
        0x441120,   // toggleFullscreen
    };

    // Sudden Strike 3 Last Stand - SSTLSGame.exe 1.4, MD5 a10cee54392f6a9199ad78e312dc753b
    constexpr GameAddresses kSuddenStrike3LastStand =
    {
        "Sudden Strike 3 Last Stand",
        0x48FDC2E8,

        0x868177,   // gameListUrlHostA
        0x86862F,   // gameListUrlHostB

        0x4E69A0,   // findTcpIpServiceProvider
        0x803566,   // findTcpIpServiceProviderSehScope

        0x463A44,   // alreadyRunningBranch

        0x43AFA1,   // minimumModeHeightPush
        0x43AFA6,   // minimumModeWidthPush
        0x43AFCC,   // afterModeListBuiltCall
        0x787570,   // displayModeDefaultConstructor
        0x788600,   // displayModeListPushBack

        0x44421F,   // savedResolutionNotListed
        0x44428F,   // applySavedResolution
        0x44429F,   // applySavedResolutionResume
        0x85FB04,   // floatConstant1024

        0x440DD2,   // interfaceScaleStore
        0x440DE2,   // interfaceScaleResume
        0xA59858,   // interfaceScale

        0x789F6A,   // fullscreenWindowStyleCall

        0x445AEA,   // windowSizedCall
        0x440B30,   // windowSizedHandler

        0x4474C8,   // toggleFullscreenCall
        0x440F70,   // toggleFullscreen
    };

    constexpr const GameAddresses* kKnownGames[] = { &kSuddenStrike3, &kSuddenStrike3LastStand };

    const GameAddresses* g_game = &kSuddenStrike3;
}

uint32_t RunningExeTimestamp()
{
    const auto* image = reinterpret_cast<const unsigned char*>(GetModuleHandleW(nullptr));
    if (!image)
        return 0;

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    return nt->FileHeader.TimeDateStamp;
}

const GameAddresses* DetectGame()
{
    const uint32_t timestamp = RunningExeTimestamp();
    for (const GameAddresses* game : kKnownGames)
    {
        if (game->peTimestamp == timestamp)
            return game;
    }
    return nullptr;
}

const GameAddresses& Game()
{
    return *g_game;
}

void SetGame(const GameAddresses* game)
{
    if (game)
        g_game = game;
}
