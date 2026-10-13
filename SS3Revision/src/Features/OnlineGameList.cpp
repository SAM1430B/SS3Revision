// Online game list
//
// When a multiplayer game is created or its state changes, the game reports it to the former
// online game list at www.suddenstrike.com through asynchronous WinINet requests (InternetOpenW
// at 0x4E1BB6, status callback 0x4E1A30). The response handler (0x4E1840) is not safe for slow
// or redirected replies and can fail inside InternetReadFile.
//
// This feature replaces the host name in both request URLs with a closed local port. Every
// request then fails to connect and is handled by the game's regular error path (0x4E1AA1).

#include "Features.h"
#include "../Core/GameAddresses.h"
#include "../Core/Log.h"
#include "../Core/Memory.h"

namespace OnlineGameList
{
    namespace
    {
        // Both host names have the same length, so the rest of the URL is unchanged.
        constexpr char kOriginalHost[]    = "www.suddenstrike.com";
        constexpr char kReplacementHost[] = "127.0.0.1:1/disabled";
        static_assert(sizeof(kOriginalHost) == sizeof(kReplacementHost), "host names must have the same length");

        constexpr size_t kHostLength = sizeof(kOriginalHost) - 1;
    }

    bool Install()
    {
        const uintptr_t hosts[] = { Game().gameListUrlHostA, Game().gameListUrlHostB };

        for (uintptr_t host : hosts)
        {
            if (!Memory::Matches(host, kOriginalHost, kHostLength))
            {
                Log::Write("OnlineGameList: unexpected bytes at %08X, skipped", static_cast<unsigned>(host));
                return false;
            }
        }

        for (uintptr_t host : hosts)
            Memory::Write(host, kReplacementHost, kHostLength);

        Log::Write("OnlineGameList: suddenstrike.com requests disabled");
        return true;
    }
}
