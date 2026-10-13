#include "Config.h"

#include <windows.h>

#include <cwctype>
#include <iterator>
#include <utility>

namespace
{
    Config g_config;

    std::wstring ReadString(const std::wstring& ini, const wchar_t* section, const wchar_t* key, const wchar_t* fallback)
    {
        wchar_t buffer[1024] = {};
        GetPrivateProfileStringW(section, key, fallback, buffer, static_cast<DWORD>(std::size(buffer)), ini.c_str());
        return buffer;
    }
}

Resolution ParseResolution(const std::wstring& text)
{
    Resolution result;
    size_t i = 0;

    auto skipSpaces = [&] {
        while (i < text.size() && std::iswspace(text[i]))
            ++i;
    };
    auto readNumber = [&](int& out) {
        int value = 0;
        bool any = false;
        while (i < text.size() && std::iswdigit(text[i]) && value < 100000)
        {
            value = value * 10 + (text[i] - L'0');
            ++i;
            any = true;
        }
        out = value;
        return any;
    };

    skipSpaces();
    if (!readNumber(result.width))
        return {};
    skipSpaces();
    if (i >= text.size() || (text[i] != L'x' && text[i] != L'X'))
        return {};
    ++i;
    skipSpaces();
    if (!readNumber(result.height))
        return {};
    return result.IsValid() ? result : Resolution{};
}

std::vector<Resolution> ParseResolutionList(const std::wstring& text)
{
    std::vector<Resolution> list;
    std::wstring item;

    auto flush = [&] {
        const Resolution resolution = ParseResolution(item);
        if (resolution.IsValid())
            list.push_back(resolution);
        item.clear();
    };

    for (wchar_t ch : text)
    {
        if (ch == L',' || ch == L';' || ch == L' ' || ch == L'\t')
            flush();
        else
            item.push_back(ch);
    }
    flush();
    return list;
}

Config Config::Load(const std::wstring& dllFolder)
{
    Config c;
    c.folder = dllFolder;
    const std::wstring ini = dllFolder + L"SS3Revision.ini";
    const wchar_t* path = ini.c_str();

    // On/off settings: 0 = off, any other number = on. A missing key keeps the default.
    c.log = GetPrivateProfileIntW(L"General", L"Log", c.log, path) != 0;

    c.disableOnlineGameList   = GetPrivateProfileIntW(L"Features", L"DisableOnlineGameList",   c.disableOnlineGameList,   path) != 0;
    c.skipSerialPortScan      = GetPrivateProfileIntW(L"Features", L"SkipSerialPortScan",      c.skipSerialPortScan,      path) != 0;
    c.allowMultipleInstances  = GetPrivateProfileIntW(L"Features", L"AllowMultipleInstances",  c.allowMultipleInstances,  path) != 0;
    c.customResolutions       = GetPrivateProfileIntW(L"Features", L"CustomResolutions",       c.customResolutions,       path) != 0;
    c.keepUnlistedResolution  = GetPrivateProfileIntW(L"Features", L"KeepUnlistedResolution",  c.keepUnlistedResolution,  path) != 0;
    c.interfaceFitsWidth      = GetPrivateProfileIntW(L"Features", L"InterfaceFitsWidth",      c.interfaceFitsWidth,      path) != 0;
    c.keepWrapperBorder       = GetPrivateProfileIntW(L"Features", L"KeepWrapperBorder",       c.keepWrapperBorder,       path) != 0;
    c.safeMinimize            = GetPrivateProfileIntW(L"Features", L"SafeMinimize",            c.safeMinimize,            path) != 0;
    c.keepBorderWhenMaximized = GetPrivateProfileIntW(L"Features", L"KeepBorderWhenMaximized", c.keepBorderWhenMaximized, path) != 0;
    c.ignoreFullscreenToggle  = GetPrivateProfileIntW(L"Features", L"IgnoreFullscreenToggle",  c.ignoreFullscreenToggle,  path) != 0;

    const Resolution minimum = ParseResolution(ReadString(ini, L"Resolution", L"Minimum", L"640x480"));
    if (minimum.IsValid())
        c.minimum = minimum;
    c.extraModes = ParseResolutionList(ReadString(ini, L"Resolution", L"Modes", L""));
    c.forced     = ParseResolution(ReadString(ini, L"Resolution", L"Force", L""));
    return c;
}

const Config& GetConfig()
{
    return g_config;
}

void SetConfig(Config config)
{
    g_config = std::move(config);
}
