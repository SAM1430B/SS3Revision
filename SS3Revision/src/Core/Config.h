#pragma once

#include <string>
#include <vector>

struct Resolution
{
    int width = 0;
    int height = 0;

    // True when both dimensions are between 256 and 8192 pixels.
    bool IsValid() const { return width >= 256 && height >= 256 && width <= 8192 && height <= 8192; }
};

// Settings read from SS3Revision.ini in the folder of the DLL.
struct Config
{
    // [General]
    bool log = false;                       // Log

    // [Features]
    // All features are enabled by default. Their keys are not part of the shipped ini and are
    // read only when added manually.
    bool disableOnlineGameList   = true;    // DisableOnlineGameList
    bool skipSerialPortScan      = true;    // SkipSerialPortScan
    bool allowMultipleInstances  = true;    // AllowMultipleInstances
    bool customResolutions       = true;    // CustomResolutions
    bool keepUnlistedResolution  = true;    // KeepUnlistedResolution
    bool interfaceFitsWidth      = true;    // InterfaceFitsWidth
    bool keepWrapperBorder       = true;    // KeepWrapperBorder
    bool safeMinimize            = true;    // SafeMinimize
    bool keepBorderWhenMaximized = true;    // KeepBorderWhenMaximized
    bool ignoreFullscreenToggle  = true;    // IgnoreFullscreenToggle

    // [Resolution]
    Resolution minimum{ 640, 480 };         // Minimum (not part of the shipped ini)
    std::vector<Resolution> extraModes;     // Modes
    Resolution forced;                      // Force; invalid when not set

    std::wstring folder;                    // Folder of the DLL, including the trailing backslash

    // Reads SS3Revision.ini from the given folder. Missing keys keep their default values.
    static Config Load(const std::wstring& dllFolder);
};

const Config& GetConfig();
void SetConfig(Config config);

// Parses "960x540" (also "960X540"). Returns an invalid Resolution when the text is not a valid size.
Resolution ParseResolution(const std::wstring& text);

std::vector<Resolution> ParseResolutionList(const std::wstring& text);
