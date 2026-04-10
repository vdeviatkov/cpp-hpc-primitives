#pragma once

#include <cstddef>
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <immintrin.h>
#endif

namespace hpc::support {

// Conservative default cache line size; can be specialized per-platform.
inline constexpr std::size_t cache_line_size = 64;

// Prefetch a cache line for read.
inline void prefetch_for_read(const void* ptr) noexcept
{
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(ptr, 0 /* read */, 3 /* high temporal locality */);
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0);
#else
    (void)ptr;
#endif
}

// Prefetch a cache line for write.
inline void prefetch_for_write(const void* ptr) noexcept
{
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(ptr, 1 /* write */, 3 /* high temporal locality */);
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0);
#else
    (void)ptr;
#endif
}

} // namespace hpc::support

