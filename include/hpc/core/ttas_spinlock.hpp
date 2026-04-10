#pragma once

#include <atomic>
#include <thread>
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

namespace hpc::core {

// Test-Test-And-Set spinlock with simple exponential backoff.
//
// Design notes:
//  - First performs a relaxed load to avoid unnecessary cache line invalidation
//    when the lock is contended.
//  - Only when the lock appears free does it attempt an exchange with
//    acquire semantics.
//  - Release semantics on unlock publish all prior writes to other threads.
class ttas_spinlock {
public:
    ttas_spinlock() noexcept = default;

    void lock() noexcept
    {
        std::size_t backoff = 1;
        for (;;) {
            // Test phase: spin while lock appears held using relaxed loads.
            while (flag_.load(std::memory_order_relaxed)) {
                pause(backoff);
                backoff = std::min<std::size_t>(backoff * 2, max_backoff_);
            }

            // Test-and-set phase: attempt to acquire.
            bool expected = false;
            if (flag_.compare_exchange_strong(expected, true, std::memory_order_acquire, std::memory_order_relaxed)) {
                return;
            }
        }
    }

    bool try_lock() noexcept
    {
        bool expected = false;
        return flag_.compare_exchange_strong(expected, true, std::memory_order_acquire, std::memory_order_relaxed);
    }

    void unlock() noexcept
    {
        flag_.store(false, std::memory_order_release);
    }

private:
    static constexpr std::size_t max_backoff_ = 1u << 16;

    static void pause(std::size_t iterations) noexcept
    {
        for (std::size_t i = 0; i < iterations; ++i) {
        #if defined(__x86_64__) || defined(_M_X64)
            _mm_pause();
        #else
            std::this_thread::yield();
        #endif
        }
    }

    std::atomic<bool> flag_{false};
};

} // namespace hpc::core

