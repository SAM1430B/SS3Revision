// dinput8.dll proxy
//
// Both game executables import DINPUT8.dll, so Windows loads this module from the game folder
// before any game code runs. Every export is forwarded to the system dinput8.dll, which is
// loaded on the first call rather than in DllMain (loader lock). Only APIs available on
// Windows XP are used, so the same module runs on Windows XP through Windows 11.

#include <windows.h>
#include <unknwn.h>

namespace
{
    HMODULE volatile g_realDInput8 = nullptr;

    HMODULE LoadRealDInput8()
    {
        wchar_t path[MAX_PATH] = {};
        const UINT length = GetSystemDirectoryW(path, MAX_PATH); // SysWOW64 for a 32-bit process
        if (length == 0 || length >= MAX_PATH - 16)
            return nullptr;
        lstrcatW(path, L"\\dinput8.dll");
        return LoadLibraryW(path);
    }

    FARPROC RealExport(const char* name)
    {
        HMODULE real = g_realDInput8;
        if (!real)
        {
            // When two threads load the module at the same time, the second one releases its reference.
            HMODULE loaded = LoadRealDInput8();
            if (!loaded)
                return nullptr;
            PVOID previous = InterlockedCompareExchangePointer(
                reinterpret_cast<PVOID volatile*>(&g_realDInput8), loaded, nullptr);
            if (previous)
            {
                FreeLibrary(loaded);
                real = static_cast<HMODULE>(previous);
            }
            else
            {
                real = loaded;
            }
        }
        return GetProcAddress(real, name);
    }
}

// Exported under the original names through dinput8.def.
extern "C"
{
    HRESULT WINAPI Proxy_DirectInput8Create(HINSTANCE instance, DWORD version, REFIID riid, LPVOID* out, LPUNKNOWN outer)
    {
        using Fn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
        const auto real = reinterpret_cast<Fn>(RealExport("DirectInput8Create"));
        return real ? real(instance, version, riid, out, outer) : E_FAIL;
    }

    HRESULT WINAPI Proxy_DllCanUnloadNow()
    {
        using Fn = HRESULT(WINAPI*)();
        const auto real = reinterpret_cast<Fn>(RealExport("DllCanUnloadNow"));
        return real ? real() : S_FALSE;
    }

    HRESULT WINAPI Proxy_DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID* out)
    {
        using Fn = HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*);
        const auto real = reinterpret_cast<Fn>(RealExport("DllGetClassObject"));
        return real ? real(clsid, riid, out) : CLASS_E_CLASSNOTAVAILABLE;
    }

    HRESULT WINAPI Proxy_DllRegisterServer()
    {
        using Fn = HRESULT(WINAPI*)();
        const auto real = reinterpret_cast<Fn>(RealExport("DllRegisterServer"));
        return real ? real() : E_FAIL;
    }

    HRESULT WINAPI Proxy_DllUnregisterServer()
    {
        using Fn = HRESULT(WINAPI*)();
        const auto real = reinterpret_cast<Fn>(RealExport("DllUnregisterServer"));
        return real ? real() : E_FAIL;
    }

    // Returns LPCDIDATAFORMAT (c_dfDIJoystick), declared as a plain pointer to avoid including dinput.h.
    const void* WINAPI Proxy_GetdfDIJoystick()
    {
        using Fn = const void*(WINAPI*)();
        const auto real = reinterpret_cast<Fn>(RealExport("GetdfDIJoystick"));
        return real ? real() : nullptr;
    }
}
