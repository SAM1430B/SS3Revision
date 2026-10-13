#pragma once

#include <string>

// Plain-text log (SS3Revision.log in the folder of the DLL), written when [General] Log is enabled.
// Each line starts with the local time. Writing is safe from any thread.
namespace Log
{
    // Creates or truncates the log file. Until this is called, Write does nothing.
    void Open(const std::wstring& path);

    // Writes one line using printf-style formatting. Lines longer than 1 KB are truncated.
    void Write(const char* format, ...);

    void Close();
}
