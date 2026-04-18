#include <gtest/gtest.h>
#include <hpc/core/dynamic_queue.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, DefaultConstruct)
{
    hpc::core::queue<int> q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);
    EXPECT_EQ(q.capacity(), 0u);
}

TEST(HpcDynamicQueue, ConstructWithCapacity)
{
    hpc::core::queue<int> q(100);
    EXPECT_TRUE(q.empty());
    EXPECT_GE(q.capacity(), 100u);
}

TEST(HpcDynamicQueue, InitializerListConstruct)
{
    hpc::core::queue<int> q{1, 2, 3};
    EXPECT_EQ(q.size(), 3u);
    EXPECT_EQ(q.front(), 1);
    EXPECT_EQ(q.back(), 3);
}

TEST(HpcDynamicQueue, CopyConstruct)
{
    hpc::core::queue<int> a{10, 20, 30};
    hpc::core::queue<int> b(a);
    EXPECT_EQ(a, b);
    a.pop();
    EXPECT_NE(a, b);
}

TEST(HpcDynamicQueue, MoveConstruct)
{
    hpc::core::queue<int> a{10, 20, 30};
    hpc::core::queue<int> b(std::move(a));
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(b.front(), 10);
    EXPECT_TRUE(a.empty());
}

TEST(HpcDynamicQueue, CopyAssign)
{
    hpc::core::queue<int> a{1, 2, 3};
    hpc::core::queue<int> b;
    b = a;
    EXPECT_EQ(a, b);
}

TEST(HpcDynamicQueue, MoveAssign)
{
    hpc::core::queue<int> a{4, 5, 6};
    hpc::core::queue<int> b;
    b = std::move(a);
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(b.front(), 4);
    EXPECT_TRUE(a.empty());
}

TEST(HpcDynamicQueue, InitializerListAssign)
{
    hpc::core::queue<int> q;
    q = {7, 8, 9};
    EXPECT_EQ(q.size(), 3u);
    EXPECT_EQ(q.front(), 7);
    EXPECT_EQ(q.back(), 9);
}

// ---------------------------------------------------------------------------
// Push / Pop / Front / Back (FIFO ordering)
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, PushAndFrontBack)
{
    hpc::core::queue<int> q;
    q.push(10);
    EXPECT_EQ(q.front(), 10);
    EXPECT_EQ(q.back(), 10);
    q.push(20);
    EXPECT_EQ(q.front(), 10);
    EXPECT_EQ(q.back(), 20);
    q.push(30);
    EXPECT_EQ(q.front(), 10);
    EXPECT_EQ(q.back(), 30);
    EXPECT_EQ(q.size(), 3u);
}

TEST(HpcDynamicQueue, PopOrder)
{
    hpc::core::queue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);

    EXPECT_EQ(q.front(), 1); q.pop();
    EXPECT_EQ(q.front(), 2); q.pop();
    EXPECT_EQ(q.front(), 3); q.pop();
    EXPECT_TRUE(q.empty());
}

TEST(HpcDynamicQueue, PopIntoValue)
{
    hpc::core::queue<int> q;
    q.push(42);
    q.push(99);

    int v = 0;
    q.pop(v);
    EXPECT_EQ(v, 42);
    q.pop(v);
    EXPECT_EQ(v, 99);
    EXPECT_TRUE(q.empty());
}

// ---------------------------------------------------------------------------
// Emplace
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, EmplaceWithPair)
{
    hpc::core::queue<std::pair<int, std::string>> q;
    q.emplace(1, "one");
    q.emplace(2, "two");
    EXPECT_EQ(q.front().first, 1);
    EXPECT_EQ(q.front().second, "one");
    EXPECT_EQ(q.back().first, 2);
    EXPECT_EQ(q.back().second, "two");
}

// ---------------------------------------------------------------------------
// Reserve / ShrinkToFit
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, Reserve)
{
    hpc::core::queue<int> q;
    q.reserve(100);
    EXPECT_GE(q.capacity(), 100u);
    EXPECT_EQ(q.size(), 0u);
}

TEST(HpcDynamicQueue, ShrinkToFit)
{
    hpc::core::queue<int> q;
    q.reserve(1024);
    q.push(1);
    q.push(2);
    q.shrink_to_fit();
    EXPECT_GE(q.capacity(), 2u);  // at least fits current size
    EXPECT_LE(q.capacity(), 8u);  // shouldn't be 1024 anymore
    EXPECT_EQ(q.front(), 1);
    EXPECT_EQ(q.back(), 2);
}

TEST(HpcDynamicQueue, ShrinkToFitEmpty)
{
    hpc::core::queue<int> q;
    q.reserve(128);
    q.shrink_to_fit();
    EXPECT_EQ(q.capacity(), 0u);
}

// ---------------------------------------------------------------------------
// Clear / Swap
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, Clear)
{
    hpc::core::queue<int> q{1, 2, 3, 4, 5};
    q.clear();
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);
}

TEST(HpcDynamicQueue, Swap)
{
    hpc::core::queue<int> a{1, 2};
    hpc::core::queue<int> b{10, 20, 30};
    a.swap(b);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a.front(), 10);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b.front(), 1);
}

// ---------------------------------------------------------------------------
// Growth: push beyond initial capacity triggers reallocation
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, GrowthOnPush)
{
    hpc::core::queue<int> q;
    for (int i = 0; i < 1000; ++i) q.push(i);
    EXPECT_EQ(q.size(), 1000u);
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(q.front(), i);
        q.pop();
    }
    EXPECT_TRUE(q.empty());
}

// ---------------------------------------------------------------------------
// Wrap-around + growth: grow while head > 0 (wrapped state)
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, GrowWhileWrapped)
{
    hpc::core::queue<int> q;
    // Push some, pop some, so head_ advances past 0.
    for (int i = 0; i < 6; ++i) q.push(i);
    for (int i = 0; i < 4; ++i) q.pop();
    // Now head_ > 0. Push enough to trigger growth.
    for (int i = 100; i < 200; ++i) q.push(i);

    // Verify FIFO order: remaining original + new.
    EXPECT_EQ(q.front(), 4);
    q.pop();
    EXPECT_EQ(q.front(), 5);
    q.pop();
    for (int i = 100; i < 200; ++i) {
        EXPECT_EQ(q.front(), i);
        q.pop();
    }
    EXPECT_TRUE(q.empty());
}

// ---------------------------------------------------------------------------
// Non-trivial types (std::string)
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, StringPushPop)
{
    hpc::core::queue<std::string> q;
    q.push("hello");
    q.push("world");
    EXPECT_EQ(q.front(), "hello");
    q.pop();
    EXPECT_EQ(q.front(), "world");
}

TEST(HpcDynamicQueue, StringMoveSemantics)
{
    hpc::core::queue<std::string> q;
    std::string val = "moved_value";
    q.push(std::move(val));
    EXPECT_EQ(q.front(), "moved_value");
}

// ---------------------------------------------------------------------------
// Move-only types
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, MoveOnlyType)
{
    hpc::core::queue<std::unique_ptr<int>> q;
    q.push(std::make_unique<int>(42));
    q.push(std::make_unique<int>(99));

    EXPECT_EQ(*q.front(), 42);
    q.pop();
    EXPECT_EQ(*q.front(), 99);
    q.pop();
    EXPECT_TRUE(q.empty());
}

// ---------------------------------------------------------------------------
// Over-aligned types
// ---------------------------------------------------------------------------

struct alignas(128) DynQueueOverAligned {
    std::int64_t value;
};

TEST(HpcDynamicQueue, OverAlignedType)
{
    hpc::core::queue<DynQueueOverAligned> q;
    q.push(DynQueueOverAligned{100});
    q.push(DynQueueOverAligned{200});

    EXPECT_EQ(q.front().value, 100);
    EXPECT_EQ(q.back().value, 200);

    auto addr = reinterpret_cast<std::uintptr_t>(&q.front());
    EXPECT_EQ(addr % alignof(DynQueueOverAligned), 0u);
}

// ---------------------------------------------------------------------------
// Equality
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, Equality)
{
    hpc::core::queue<int> a{1, 2, 3};
    hpc::core::queue<int> b{1, 2, 3};
    hpc::core::queue<int> c{1, 2, 4};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(HpcDynamicQueue, EqualityAfterWrap)
{
    // Build two queues with the same logical content but different internal layouts.
    hpc::core::queue<int> a;
    for (int i = 0; i < 10; ++i) a.push(i);
    for (int i = 0; i < 7; ++i) a.pop();  // head_ advanced

    hpc::core::queue<int> b;
    for (int i = 7; i < 10; ++i) b.push(i);

    EXPECT_EQ(a, b);
}

// ---------------------------------------------------------------------------
// FIFO stress
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, FIFOStress)
{
    constexpr int N = 10000;
    hpc::core::queue<int> q;

    for (int i = 0; i < N; ++i) q.push(i);
    EXPECT_EQ(q.size(), static_cast<std::size_t>(N));

    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(q.front(), i);
        q.pop();
    }
    EXPECT_TRUE(q.empty());
}

// ---------------------------------------------------------------------------
// Interleaved push/pop stress (exercises wrap-around heavily)
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, InterleavedStress)
{
    hpc::core::queue<int> q;
    int push_val = 0, pop_val = 0;
    for (int round = 0; round < 100; ++round) {
        for (int i = 0; i < 50; ++i) q.push(push_val++);
        for (int i = 0; i < 30; ++i) {
            EXPECT_EQ(q.front(), pop_val++);
            q.pop();
        }
    }
    while (!q.empty()) {
        EXPECT_EQ(q.front(), pop_val++);
        q.pop();
    }
    EXPECT_EQ(push_val, pop_val);
}

// ---------------------------------------------------------------------------
// Const access
// ---------------------------------------------------------------------------

TEST(HpcDynamicQueue, ConstAccess)
{
    hpc::core::queue<int> q;
    q.push(42);
    q.push(99);
    const auto& cq = q;
    EXPECT_EQ(cq.front(), 42);
    EXPECT_EQ(cq.back(), 99);
    EXPECT_EQ(cq.size(), 2u);
    EXPECT_FALSE(cq.empty());
}

