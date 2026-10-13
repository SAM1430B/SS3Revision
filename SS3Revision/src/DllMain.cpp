// SS3Revision
//
// Runtime patches for Sudden Strike 3 (SS3Game.exe 1.4) and Sudden Strike 3 Last Stand
// (SSTLSGame.exe 1.4) Steam/GOG versions, loaded by the game as dinput8.dll. On process attach the module reads
// SS3Revision.ini, identifies the game by the PE timestamp of the executable and installs the
// features in src/Features before the game's WinMain runs. An unrecognised executable is left
// unchanged.

#include <windows.h>

#include <string>

#include "Core/Config.h"
#include "Core/GameAddresses.h"
#include "Core/Log.h"
#include "Features/Features.h"

namespace
{
    // Folder of the module, including the trailing backslash.
    std::wstring ModuleFolder(HMODULE module)
    {
        wchar_t path[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameW(module, path, MAX_PATH);
        const std::wstring fullPath(path, length);
        const size_t slash = fullPath.find_last_of(L"\\/");
        return slash == std::wstring::npos ? std::wstring() : fullPath.substr(0, slash + 1);
    }

    // File name of the running executable, without its folder.
    std::string ExecutableName()
    {
        char path[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        const std::string fullPath(path);
        const size_t slash = fullPath.find_last_of("\\/");
        return slash == std::string::npos ? fullPath : fullPath.substr(slash + 1);
    }

    void Install(const char* name, bool enabled, bool (*install)())
    {
        if (enabled)
            install();
        else
            Log::Write("%s: disabled in ini", name);
    }

    void InstallAll(HMODULE self)
    {
        SetConfig(Config::Load(ModuleFolder(self)));
        const Config& config = GetConfig();

        if (config.log)
            Log::Open(config.folder + L"SS3Revision.log");

        Log::Write("SS3Revision loaded into %s", ExecutableName().c_str());

        const GameAddresses* game = DetectGame();
        if (!game)
        {
            Log::Write("Unknown game version (PE timestamp %08X), no changes made",
                       static_cast<unsigned>(RunningExeTimestamp()));
            return;
        }
        SetGame(game);
        Log::Write("Detected %s 1.4 (PE timestamp %08X)", game->name, static_cast<unsigned>(game->peTimestamp));

        Install("OnlineGameList",    config.disableOnlineGameList,   &OnlineGameList::Install);
        Install("SerialPortScan",    config.skipSerialPortScan,      &SerialPortScan::Install);
        Install("MultipleInstances", config.allowMultipleInstances,  &MultipleInstances::Install);
        Install("ResolutionList",    config.customResolutions,       &ResolutionList::Install);
        Install("SavedResolution",   config.keepUnlistedResolution,  &SavedResolution::Install);
        Install("InterfaceScaling",  config.interfaceFitsWidth,      &InterfaceScaling::Install);
        Install("WrapperBorder",     config.keepWrapperBorder,       &WrapperBorder::Install);
        Install("MinimizeRestore",   config.safeMinimize,            &MinimizeRestore::Install);
        Install("MaximizeWindow",    config.keepBorderWhenMaximized, &MaximizeWindow::Install);
        Install("FullscreenToggle",  config.ignoreFullscreenToggle,  &FullscreenToggle::Install);
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(module);
        InstallAll(module);
        break;
    case DLL_PROCESS_DETACH:
        Log::Close();
        break;
    }
    return TRUE;
}
