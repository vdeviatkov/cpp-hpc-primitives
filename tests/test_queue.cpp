#include <gtest/gtest.h>
#include <hpc/core/queue.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(HpcFixedQueue, DefaultConstruct)
{
    hpc::core::fixed_queue<int, 16> q;
    EXPECT_TRUE(q.empty());
    EXPECT_FALSE(q.full());
    EXPECT_EQ(q.size(), 0u);
    EXPECT_EQ(q.capacity(), 16u);
}

TEST(HpcFixedQueue, InitializerListConstruct)
{
    hpc::core::fixed_queue<int, 8> q{1, 2, 3};
    EXPECT_EQ(q.size(), 3u);
    EXPECT_EQ(q.front(), 1);
    EXPECT_EQ(q.back(), 3);
}

TEST(HpcFixedQueue, InitializerListOverflow)
{
    EXPECT_THROW(
        (hpc::core::fixed_queue<int, 2>{1, 2, 3}),
        std::overflow_error);
}

TEST(HpcFixedQueue, CopyConstruct)
{
    hpc::core::fixed_queue<int, 8> a{10, 20, 30};
    hpc::core::fixed_queue<int, 8> b(a);
    EXPECT_EQ(a, b);
    a.pop();
    EXPECT_NE(a, b);
}

TEST(HpcFixedQueue, MoveConstruct)
{
    hpc::core::fixed_queue<int, 8> a{10, 20, 30};
    hpc::core::fixed_queue<int, 8> b(std::move(a));
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(b.front(), 10);
    EXPECT_TRUE(a.empty());
}

TEST(HpcFixedQueue, CopyAssign)
{
    hpc::core::fixed_queue<int, 8> a{1, 2, 3};
    hpc::core::fixed_queue<int, 8> b;
    b = a;
    EXPECT_EQ(a, b);
}

TEST(HpcFixedQueue, MoveAssign)
{
    hpc::core::fixed_queue<int, 8> a{4, 5, 6};
    hpc::core::fixed_queue<int, 8> b;
    b = std::move(a);
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(b.front(), 4);
    EXPECT_TRUE(a.empty());
}

// ---------------------------------------------------------------------------
// Push / Pop / Front / Back (FIFO ordering)
// ---------------------------------------------------------------------------

TEST(HpcFixedQueue, PushAndFrontBack)
{
    hpc::core::fixed_queue<int, 8> q;
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

TEST(HpcFixedQueue, PopOrder)
{
    hpc::core::fixed_queue<int, 8> q;
    q.push(1);
    q.push(2);
    q.push(3);

    EXPECT_EQ(q.front(), 1); q.pop();
    EXPECT_EQ(q.front(), 2); q.pop();
    EXPECT_EQ(q.front(), 3); q.pop();
    EXPECT_TRUE(q.empty());
}

TEST(HpcFixedQueue, TryPushOnFull)
{
    hpc::core::fixed_queue<int, 2> q;
    EXPECT_TRUE(q.try_push(1));
    EXPECT_TRUE(q.try_push(2));
    EXPECT_TRUE(q.full());
    EXPECT_FALSE(q.try_push(3));
    EXPECT_EQ(q.size(), 2u);
}

TEST(HpcFixedQueue, PushThrowsOnFull)
{
    hpc::core::fixed_queue<int, 2> q;
    q.push(1);
    q.push(2);
    EXPECT_THROW(q.push(3), std::overflow_error);
}

TEST(HpcFixedQueue, TryPopOnEmpty)
{
    hpc::core::fixed_queue<int, 4> q;
    EXPECT_FALSE(q.try_pop());
}

TEST(HpcFixedQueue, PopThrowsOnEmpty)
{
    hpc::core::fixed_queue<int, 4> q;
    EXPECT_THROW(q.pop(), std::underflow_error);
}

TEST(HpcFixedQueue, TryPopIntoValue)
{
    hpc::core::fixed_queue<int, 4> q;
    q.push(42);
    q.push(99);

    int v = 0;
    EXPECT_TRUE(q.try_pop(v));
    EXPECT_EQ(v, 42);
    EXPECT_TRUE(q.try_pop(v));
    EXPECT_EQ(v, 99);
    EXPECT_FALSE(q.try_pop(v));
}

// ---------------------------------------------------------------------------
// Emplace
// ---------------------------------------------------------------------------

TEST(HpcFixedQueue, EmplaceWithPair)
{
    hpc::core::fixed_queue<std::pair<int, std::string>, 4> q;
    q.emplace(1, "one");
    q.emplace(2, "two");
    EXPECT_EQ(q.front().first, 1);
    EXPECT_EQ(q.front().second, "one");
    EXPECT_EQ(q.back().first, 2);
    EXPECT_EQ(q.back().second, "two");
}

TEST(HpcFixedQueue, TryEmplace)
{
    hpc::core::fixed_queue<std::pair<int, int>, 2> q;
    EXPECT_TRUE(q.try_emplace(1, 2));
    EXPECT_TRUE(q.try_emplace(3, 4));
    EXPECT_FALSE(q.try_emplace(5, 6));
    EXPECT_EQ(q.front().first, 1);
}

TEST(HpcFixedQueue, EmplaceThrowsOnFull)
{
    hpc::core::fixed_queue<int, 1> q;
    q.emplace(42);
    EXPECT_THROW(q.emplace(99), std::overflow_error);
}

// ---------------------------------------------------------------------------
// Clear / Swap
// ---------------------------------------------------------------------------

TEST(HpcFixedQueue, Clear)
{
    hpc::core::fixed_queue<int, 8> q{1, 2, 3, 4, 5};
    q.clear();
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);
}

TEST(HpcFixedQueue, Swap)
{
    hpc::core::fixed_queue<int, 8> a{1, 2};
    hpc::core::fixed_queue<int, 8> b{10, 20, 30};
    a.swap(b);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a.front(), 10);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b.front(), 1);
}

// ---------------------------------------------------------------------------
// Wrap-around: push/pop past internal buffer boundary
// ---------------------------------------------------------------------------

TEST(HpcFixedQueue, WrapAround)
{
    // Capacity 4 → internal buffer is 8 slots (next power of two >= 5).
    hpc::core::fixed_queue<int, 4> q;

    // Fill and drain a few times to force head_ past index 0.
    for (int round = 0; round < 5; ++round) {
        for (int i = 0; i < 4; ++i) q.push(round * 10 + i);
        EXPECT_TRUE(q.full());
        for (int i = 0; i < 4; ++i) {
            EXPECT_EQ(q.front(), round * 10 + i);
            q.pop();
        }
        EXPECT_TRUE(q.empty());
    }
}

// ---------------------------------------------------------------------------
// Non-trivial types (std::string)
// ---------------------------------------------------------------------------

TEST(HpcFixedQueue, StringPushPop)
{
    hpc::core::fixed_queue<std::string, 4> q;
    q.push("hello");
    q.push("world");
    EXPECT_EQ(q.front(), "hello");
    q.pop();
    EXPECT_EQ(q.front(), "world");
}

TEST(HpcFixedQueue, StringMoveSemantics)
{
    hpc::core::fixed_queue<std::string, 4> q;
    std::string val = "moved_value";
    q.push(std::move(val));
    EXPECT_EQ(q.front(), "moved_value");
}

// ---------------------------------------------------------------------------
// Move-only types
// ---------------------------------------------------------------------------

TEST(HpcFixedQueue, MoveOnlyType)
{
    hpc::core::fixed_queue<std::unique_ptr<int>, 4> q;
    q.push(std::make_unique<int>(42));
    q.push(std::make_unique<int>(99));

    EXPECT_EQ(*q.front(), 42);

    std::unique_ptr<int> out;
    EXPECT_TRUE(q.try_pop(out));
    EXPECT_EQ(*out, 42);
    EXPECT_TRUE(q.try_pop(out));
    EXPECT_EQ(*out, 99);
    EXPECT_TRUE(q.empty());
}

// ---------------------------------------------------------------------------
// Over-aligned types
// ---------------------------------------------------------------------------

struct alignas(128) QueueOverAligned {
    std::int64_t value;
};

TEST(HpcFixedQueue, OverAlignedType)
{
    hpc::core::fixed_queue<QueueOverAligned, 4> q;
    q.push(QueueOverAligned{100});
    q.push(QueueOverAligned{200});

    EXPECT_EQ(q.front().value, 100);
    EXPECT_EQ(q.back().value, 200);

    auto addr = reinterpret_cast<std::uintptr_t>(&q.front());
    EXPECT_EQ(addr % alignof(QueueOverAligned), 0u);
}

// ---------------------------------------------------------------------------
// Equality
// ---------------------------------------------------------------------------

TEST(HpcFixedQueue, Equality)
{
    hpc::core::fixed_queue<int, 8> a{1, 2, 3};
    hpc::core::fixed_queue<int, 8> b{1, 2, 3};
    hpc::core::fixed_queue<int, 8> c{1, 2, 4};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

// ---------------------------------------------------------------------------
// FIFO stress
// ---------------------------------------------------------------------------

TEST(HpcFixedQueue, FIFOStress)
{
    constexpr std::size_t N = 256;
    hpc::core::fixed_queue<int, N> q;

    for (int i = 0; i < static_cast<int>(N); ++i)
        q.push(i);

    EXPECT_TRUE(q.full());

    for (int i = 0; i < static_cast<int>(N); ++i) {
        EXPECT_EQ(q.front(), i);
        q.pop();
    }

    EXPECT_TRUE(q.empty());
}

// ---------------------------------------------------------------------------
// Const access
// ---------------------------------------------------------------------------

TEST(HpcFixedQueue, ConstAccess)
{
    hpc::core::fixed_queue<int, 4> q;
    q.push(42);
    q.push(99);
    const auto& cq = q;
    EXPECT_EQ(cq.front(), 42);
    EXPECT_EQ(cq.back(), 99);
    EXPECT_EQ(cq.size(), 2u);
    EXPECT_FALSE(cq.empty());
    EXPECT_FALSE(cq.full());
}

