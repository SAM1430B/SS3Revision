#include "Memory.h"

#include <windows.h>

#include <cstring>
#include <vector>

namespace Memory
{
    namespace
    {
        constexpr uint8_t kCall = 0xE8;
        constexpr uint8_t kJump = 0xE9;
        constexpr uint8_t kNop  = 0x90;

        // True when [address, address + size) is committed, accessible memory.
        bool IsReadable(uintptr_t address, size_t size)
        {
            const uintptr_t end = address + size;
            while (address < end)
            {
                MEMORY_BASIC_INFORMATION info{};
                if (VirtualQuery(reinterpret_cast<const void*>(address), &info, sizeof(info)) == 0)
                    return false;
                if (info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
                    return false;

                const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
                if (regionEnd <= address)
                    return false;
                address = regionEnd;
            }
            return true;
        }

        // Writes "opcode rel32" followed by NOPs up to 'length' bytes.
        bool WriteBranch(uint8_t opcode, uintptr_t address, uintptr_t target, size_t length)
        {
            if (length < 5)
                return false;

            std::vector<uint8_t> code(length, kNop);
            const int32_t rel = static_cast<int32_t>(target - (address + 5));
            code[0] = opcode;
            std::memcpy(&code[1], &rel, sizeof(rel));
            return Write(address, code.data(), code.size());
        }
    }

    bool Matches(uintptr_t address, const void* expected, size_t size)
    {
        return IsReadable(address, size) &&
               std::memcmp(reinterpret_cast<const void*>(address), expected, size) == 0;
    }

    bool Matches(uintptr_t address, std::initializer_list<uint8_t> expected)
    {
        const std::vector<uint8_t> bytes(expected);
        return Matches(address, bytes.data(), bytes.size());
    }

    bool MatchesWithValue(uintptr_t address, std::initializer_list<uint8_t> prefix, uint32_t value,
                          std::initializer_list<uint8_t> suffix)
    {
        std::vector<uint8_t> bytes(prefix);
        for (int shift = 0; shift < 32; shift += 8)
            bytes.push_back(static_cast<uint8_t>(value >> shift));
        bytes.insert(bytes.end(), suffix.begin(), suffix.end());
        return Matches(address, bytes.data(), bytes.size());
    }

    bool Write(uintptr_t address, const void* data, size_t size)
    {
        void* target = reinterpret_cast<void*>(address);
        DWORD oldProtect = 0;
        if (!VirtualProtect(target, size, PAGE_EXECUTE_READWRITE, &oldProtect))
            return false;

        std::memcpy(target, data, size);

        DWORD ignored = 0;
        VirtualProtect(target, size, oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), target, size);
        return true;
    }

    bool WriteJump(uintptr_t address, uintptr_t target, size_t length)
    {
        return WriteBranch(kJump, address, target, length);
    }

    bool WriteCall(uintptr_t address, uintptr_t target, size_t length)
    {
        return WriteBranch(kCall, address, target, length);
    }

    bool RedirectCall(uintptr_t address, uintptr_t target)
    {
        if (!Matches(address, { kCall }))
            return false;

        const int32_t rel = static_cast<int32_t>(target - (address + 5));
        return Write(address + 1, &rel, sizeof(rel));
    }

    uintptr_t RelativeTarget(uintptr_t address)
    {
        int32_t rel = 0;
        std::memcpy(&rel, reinterpret_cast<const void*>(address + 1), sizeof(rel));
        return address + 5 + rel;
    }
}
