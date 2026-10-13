#include "DisplayEnvironment.h"

#include <windows.h>
#include <d3d9.h>

#include <cwchar>
#include <string_view>

namespace DisplayEnvironment
{
    namespace
    {
        volatile LONG g_wrapped = -1;

        std::wstring_view FolderOf(const wchar_t* path)
        {
            const wchar_t* slash = std::wcsrchr(path, L'\\');
            return slash ? std::wstring_view(path, static_cast<size_t>(slash - path)) : std::wstring_view();
        }

        bool SameFolder(std::wstring_view folder, const wchar_t* other)
        {
            const size_t length = std::wcslen(other);
            return folder.size() == length && _wcsnicmp(folder.data(), other, length) == 0;
        }

        bool IsSystemFolder(std::wstring_view folder)
        {
            // A 32-bit process loads the system d3d9.dll from SysWOW64 on 64-bit Windows and
            // from System32 on 32-bit Windows.
            wchar_t systemFolder[MAX_PATH] = {};
            if (GetSystemDirectoryW(systemFolder, MAX_PATH) && SameFolder(folder, systemFolder))
                return true;
            wchar_t wow64Folder[MAX_PATH] = {};
            if (GetSystemWow64DirectoryW(wow64Folder, MAX_PATH) && SameFolder(folder, wow64Folder))
                return true;
            return false;
        }
    }

    bool IsDirect3D9Wrapped()
    {
        if (g_wrapped >= 0)
            return g_wrapped == 1;

        const HMODULE d3d9 = GetModuleHandleW(L"d3d9.dll");
        if (!d3d9)
            return false;

        wchar_t modulePath[MAX_PATH] = {};
        if (GetModuleFileNameW(d3d9, modulePath, MAX_PATH) == 0)
            return false;

        const bool wrapped = !IsSystemFolder(FolderOf(modulePath));
        g_wrapped = wrapped ? 1 : 0;
        return wrapped;
    }

    bool IsFullscreenModeSupported(int width, int height)
    {
        HMODULE d3d9 = GetModuleHandleW(L"d3d9.dll");
        if (!d3d9)
            d3d9 = LoadLibraryW(L"d3d9.dll");
        if (!d3d9)
            return true;

        using Direct3DCreate9Fn = IDirect3D9*(WINAPI*)(UINT);
        const auto create = reinterpret_cast<Direct3DCreate9Fn>(GetProcAddress(d3d9, "Direct3DCreate9"));
        IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
        if (!d3d)
            return true;

        bool supported = false;
        const UINT count = d3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
        for (UINT i = 0; i < count && !supported; ++i)
        {
            D3DDISPLAYMODE mode{};
            if (SUCCEEDED(d3d->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &mode)))
                supported = static_cast<int>(mode.Width) == width && static_cast<int>(mode.Height) == height;
        }
        d3d->Release();
        return supported;
    }
}
