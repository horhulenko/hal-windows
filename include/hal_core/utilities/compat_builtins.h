#pragma once

// Portable replacements for the GCC/Clang bit-counting builtins used throughout HAL, needed for MSVC.
#ifdef _MSC_VER
#include <intrin.h>

inline int __builtin_clzll(unsigned long long x)
{
    unsigned long index;
    _BitScanReverse64(&index, x);
    return 63 - static_cast<int>(index);
}

inline int __builtin_ctzll(unsigned long long x)
{
    unsigned long index;
    _BitScanForward64(&index, x);
    return static_cast<int>(index);
}

inline int __builtin_ctz(unsigned int x)
{
    unsigned long index;
    _BitScanForward(&index, x);
    return static_cast<int>(index);
}

inline int __builtin_popcountll(unsigned long long x)
{
    return static_cast<int>(__popcnt64(x));
}
#endif
