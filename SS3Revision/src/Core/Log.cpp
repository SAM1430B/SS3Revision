#include "Log.h"

#include <windows.h>

#include <cstdarg>
#include <cstdio>

namespace
{
    HANDLE           g_file = INVALID_HANDLE_VALUE;
    CRITICAL_SECTION g_lock;
    bool             g_lockReady = false;
}

namespace Log
{
    void Open(const std::wstring& path)
    {
        if (!g_lockReady)
        {
            InitializeCriticalSection(&g_lock);
            g_lockReady = true;
        }
        g_file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
    }

    void Write(const char* format, ...)
    {
        if (g_file == INVALID_HANDLE_VALUE)
            return;

        char message[1024];
        SYSTEMTIME time;
        GetLocalTime(&time);
        int prefix = std::snprintf(message, sizeof(message), "%02u:%02u:%02u.%03u  ",
                                   time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
        if (prefix < 0)
            prefix = 0;

        va_list args;
        va_start(args, format);
        int body = std::vsnprintf(message + prefix, sizeof(message) - prefix - 2, format, args);
        va_end(args);
        if (body < 0)
            body = 0;

        size_t length = static_cast<size_t>(prefix) + static_cast<size_t>(body);
        if (length > sizeof(message) - 2)
            length = sizeof(message) - 2;
        message[length++] = '\r';
        message[length++] = '\n';

        EnterCriticalSection(&g_lock);
        DWORD written = 0;
        WriteFile(g_file, message, static_cast<DWORD>(length), &written, nullptr);
        LeaveCriticalSection(&g_lock);
    }

    void Close()
    {
        if (g_file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(g_file);
            g_file = INVALID_HANDLE_VALUE;
        }
    }
}
