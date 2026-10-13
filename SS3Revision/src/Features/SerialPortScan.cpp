// Serial port scan
//
// Each time the network layer is initialised (at startup and when the host leaves the
// multiplayer lobby), NetPeer::FindTcpIpServiceProvider (0x4E6C20) calls
// IDirectPlay8Peer::EnumServiceProviders. The enumeration includes the serial provider, which
// opens every COM port; Bluetooth and virtual COM ports can block for about a minute.
//
// The function only stores the GUID of the TCP/IP service provider at this+0x7C, and its
// return value is not used. This feature replaces it with a function that stores
// CLSID_DP8SP_TCPIP directly.

#include "Features.h"
#include "../Core/GameAddresses.h"
#include "../Core/Log.h"
#include "../Core/Memory.h"

#include <windows.h>

namespace SerialPortScan
{
    namespace
    {
        // CLSID_DP8SP_TCPIP {EBFE7BA0-628D-11D2-AE0F-006097B01411}
        constexpr GUID kTcpIpServiceProvider =
            { 0xEBFE7BA0, 0x628D, 0x11D2, { 0xAE, 0x0F, 0x00, 0x60, 0x97, 0xB0, 0x14, 0x11 } };

        // Replacement for the thiscall member function. 'self' is passed in ecx and there are
        // no stack arguments, which matches __fastcall with a single parameter.
        void __fastcall SelectTcpIpServiceProvider(void* self)
        {
            auto* guid = reinterpret_cast<GUID*>(static_cast<unsigned char*>(self) +
                                                 GameOffsets::NetPeerServiceProviderGuid);
            *guid = kTcpIpServiceProvider;
        }
    }

    bool Install()
    {
        const GameAddresses& game = Game();

        // push -1 ; push <SEH scope table>
        if (!Memory::MatchesWithValue(game.findTcpIpServiceProvider, { 0x6A, 0xFF, 0x68 },
                                      game.findTcpIpServiceProviderSehScope))
        {
            Log::Write("SerialPortScan: unexpected bytes, skipped");
            return false;
        }

        Memory::WriteJump(game.findTcpIpServiceProvider, &SelectTcpIpServiceProvider, 7);
        Log::Write("SerialPortScan: DirectPlay provider lookup replaced (no COM port probing)");
        return true;
    }
}
