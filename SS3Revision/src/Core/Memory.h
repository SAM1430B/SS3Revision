#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>

// Helpers for inspecting and patching the game executable in memory.
namespace Memory
{
    // True when the bytes at the address are readable and equal the expected bytes.
    bool Matches(uintptr_t address, std::initializer_list<uint8_t> expected);
    bool Matches(uintptr_t address, const void* expected, size_t size);

    // True when the bytes at the address are 'prefix', then 'value' in little-endian order, then
    // 'suffix'. Used for instructions that contain an absolute address.
    bool MatchesWithValue(uintptr_t address, std::initializer_list<uint8_t> prefix, uint32_t value,
                          std::initializer_list<uint8_t> suffix = {});

    // Writes raw bytes. The page protection is changed for the duration of the write.
    bool Write(uintptr_t address, const void* data, size_t size);

    template <typename T>
    bool WriteValue(uintptr_t address, const T& value)
    {
        return Write(address, &value, sizeof(T));
    }

    // Replaces 'length' bytes (at least 5) with "jmp target" followed by NOPs.
    bool WriteJump(uintptr_t address, uintptr_t target, size_t length = 5);

    // Replaces 'length' bytes (at least 5) with "call target" followed by NOPs.
    bool WriteCall(uintptr_t address, uintptr_t target, size_t length = 5);

    // Changes the destination of the existing "call rel32" (E8) instruction at the address.
    bool RedirectCall(uintptr_t address, uintptr_t target);

    // Destination of the "call rel32" or "jmp rel32" instruction at the address.
    uintptr_t RelativeTarget(uintptr_t address);

    template <typename Function>
    bool WriteJump(uintptr_t address, Function* target, size_t length = 5)
    {
        return WriteJump(address, reinterpret_cast<uintptr_t>(target), length);
    }

    template <typename Function>
    bool WriteCall(uintptr_t address, Function* target, size_t length = 5)
    {
        return WriteCall(address, reinterpret_cast<uintptr_t>(target), length);
    }

    template <typename Function>
    bool RedirectCall(uintptr_t address, Function* target)
    {
        return RedirectCall(address, reinterpret_cast<uintptr_t>(target));
    }
}
