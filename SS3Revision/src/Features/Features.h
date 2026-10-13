#pragma once

// Each feature is implemented in its own file. Install() verifies the original bytes at every
// location it changes and returns false, without changing anything, if they do not match.
namespace OnlineGameList     { bool Install(); }  // disables reports to the former online game list
namespace SerialPortScan     { bool Install(); }  // selects the TCP/IP provider without enumerating COM ports
namespace MultipleInstances  { bool Install(); }  // allows more than one game window
namespace ResolutionList     { bool Install(); }  // lower minimum and additional modes in the options menu
namespace SavedResolution    { bool Install(); }  // keeps unlisted or forced resolutions at startup
namespace InterfaceScaling   { bool Install(); }  // fits the interface to both window width and height
namespace WrapperBorder      { bool Install(); }  // keeps the window frame set by a Direct3D 9 wrapper
namespace MinimizeRestore    { bool Install(); }  // keeps the back buffer while the window is minimized
namespace MaximizeWindow     { bool Install(); }  // maximize and restore with a Direct3D 9 wrapper
namespace FullscreenToggle   { bool Install(); }  // ignores Alt+Enter with a Direct3D 9 wrapper
