// Multiple instances
//
// At startup, IsGameAlreadyRunning (0x463E50) looks for a window of the class
// "Sudden Strike 3 GAME" (string resource 0x66) with FindWindowW. When one exists, it brings
// that window to the front and the new process exits. No other lock is used.
//
// This feature turns the conditional branch after FindWindowW into an unconditional one, so
// the game always continues as if no other game window existed.

#include "Features.h"
#include "../Core/GameAddresses.h"
#include "../Core/Log.h"
#include "../Core/Memory.h"

namespace MultipleInstances
{
    bool Install()
    {
        const uintptr_t branch = Game().alreadyRunningBranch;

        // je +0x1C
        if (!Memory::Matches(branch, { 0x74, 0x1C }))
        {
            Log::Write("MultipleInstances: unexpected bytes, skipped");
            return false;
        }

        // jmp +0x1C
        const unsigned char alwaysContinue = 0xEB;
        Memory::Write(branch, &alwaysContinue, 1);
        Log::Write("MultipleInstances: single-instance check disabled");
        return true;
    }
}
