#include <gtest/gtest.h>
#include <hpc/core/stack.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(HpcFixedStack, DefaultConstruct)
{
    hpc::core::fixed_stack<int, 16> s;
    EXPECT_TRUE(s.empty());
    EXPECT_FALSE(s.full());
    EXPECT_EQ(s.size(), 0u);
    EXPECT_EQ(s.capacity(), 16u);
}

TEST(HpcFixedStack, InitializerListConstruct)
{
    hpc::core::fixed_stack<int, 8> s{1, 2, 3};
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s.top(), 3);
}

TEST(HpcFixedStack, InitializerListOverflow)
{
    EXPECT_THROW(
        (hpc::core::fixed_stack<int, 2>{1, 2, 3}),
        std::overflow_error);
}

TEST(HpcFixedStack, CopyConstruct)
{
    hpc::core::fixed_stack<int, 8> a{10, 20, 30};
    hpc::core::fixed_stack<int, 8> b(a);
    EXPECT_EQ(a, b);
    a.pop();
    EXPECT_NE(a, b);
}

TEST(HpcFixedStack, MoveConstruct)
{
    hpc::core::fixed_stack<int, 8> a{10, 20, 30};
    hpc::core::fixed_stack<int, 8> b(std::move(a));
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(b.top(), 30);
    EXPECT_TRUE(a.empty());
}

TEST(HpcFixedStack, CopyAssign)
{
    hpc::core::fixed_stack<int, 8> a{1, 2, 3};
    hpc::core::fixed_stack<int, 8> b;
    b = a;
    EXPECT_EQ(a, b);
}

TEST(HpcFixedStack, MoveAssign)
{
    hpc::core::fixed_stack<int, 8> a{4, 5, 6};
    hpc::core::fixed_stack<int, 8> b;
    b = std::move(a);
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(b.top(), 6);
    EXPECT_TRUE(a.empty());
}

// ---------------------------------------------------------------------------
// Push / Pop / Top (LIFO ordering)
// ---------------------------------------------------------------------------

TEST(HpcFixedStack, PushAndTop)
{
    hpc::core::fixed_stack<int, 8> s;
    s.push(10);
    EXPECT_EQ(s.top(), 10);
    s.push(20);
    EXPECT_EQ(s.top(), 20);
    s.push(30);
    EXPECT_EQ(s.top(), 30);
    EXPECT_EQ(s.size(), 3u);
}

TEST(HpcFixedStack, PopOrder)
{
    hpc::core::fixed_stack<int, 8> s;
    s.push(1);
    s.push(2);
    s.push(3);

    EXPECT_EQ(s.top(), 3); s.pop();
    EXPECT_EQ(s.top(), 2); s.pop();
    EXPECT_EQ(s.top(), 1); s.pop();
    EXPECT_TRUE(s.empty());
}

TEST(HpcFixedStack, TryPushOnFull)
{
    hpc::core::fixed_stack<int, 2> s;
    EXPECT_TRUE(s.try_push(1));
    EXPECT_TRUE(s.try_push(2));
    EXPECT_TRUE(s.full());
    EXPECT_FALSE(s.try_push(3));
    EXPECT_EQ(s.size(), 2u);
}

TEST(HpcFixedStack, PushThrowsOnFull)
{
    hpc::core::fixed_stack<int, 2> s;
    s.push(1);
    s.push(2);
    EXPECT_THROW(s.push(3), std::overflow_error);
}

TEST(HpcFixedStack, TryPopOnEmpty)
{
    hpc::core::fixed_stack<int, 4> s;
    EXPECT_FALSE(s.try_pop());
}

TEST(HpcFixedStack, PopThrowsOnEmpty)
{
    hpc::core::fixed_stack<int, 4> s;
    EXPECT_THROW(s.pop(), std::underflow_error);
}

TEST(HpcFixedStack, TryPopIntoValue)
{
    hpc::core::fixed_stack<int, 4> s;
    s.push(42);
    s.push(99);

    int v = 0;
    EXPECT_TRUE(s.try_pop(v));
    EXPECT_EQ(v, 99);
    EXPECT_TRUE(s.try_pop(v));
    EXPECT_EQ(v, 42);
    EXPECT_FALSE(s.try_pop(v));
}

// ---------------------------------------------------------------------------
// Emplace
// ---------------------------------------------------------------------------

TEST(HpcFixedStack, EmplaceWithPair)
{
    hpc::core::fixed_stack<std::pair<int, std::string>, 4> s;
    s.emplace(1, "one");
    s.emplace(2, "two");
    EXPECT_EQ(s.top().first, 2);
    EXPECT_EQ(s.top().second, "two");
    s.pop();
    EXPECT_EQ(s.top().first, 1);
    EXPECT_EQ(s.top().second, "one");
}

TEST(HpcFixedStack, TryEmplace)
{
    hpc::core::fixed_stack<std::pair<int, int>, 2> s;
    EXPECT_TRUE(s.try_emplace(1, 2));
    EXPECT_TRUE(s.try_emplace(3, 4));
    EXPECT_FALSE(s.try_emplace(5, 6));
    EXPECT_EQ(s.top().first, 3);
}

TEST(HpcFixedStack, EmplaceThrowsOnFull)
{
    hpc::core::fixed_stack<int, 1> s;
    s.emplace(42);
    EXPECT_THROW(s.emplace(99), std::overflow_error);
}

// ---------------------------------------------------------------------------
// Clear / Swap
// ---------------------------------------------------------------------------

TEST(HpcFixedStack, Clear)
{
    hpc::core::fixed_stack<int, 8> s{1, 2, 3, 4, 5};
    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0u);
}

TEST(HpcFixedStack, Swap)
{
    hpc::core::fixed_stack<int, 8> a{1, 2};
    hpc::core::fixed_stack<int, 8> b{10, 20, 30};
    a.swap(b);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a.top(), 30);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b.top(), 2);
}

// ---------------------------------------------------------------------------
// Non-trivial types (std::string)
// ---------------------------------------------------------------------------

TEST(HpcFixedStack, StringPushPop)
{
    hpc::core::fixed_stack<std::string, 4> s;
    s.push("hello");
    s.push("world");
    EXPECT_EQ(s.top(), "world");
    s.pop();
    EXPECT_EQ(s.top(), "hello");
}

TEST(HpcFixedStack, StringMoveSemantics)
{
    hpc::core::fixed_stack<std::string, 4> s;
    std::string val = "moved_value";
    s.push(std::move(val));
    EXPECT_EQ(s.top(), "moved_value");
    // val is in a valid but unspecified state after move
}

// ---------------------------------------------------------------------------
// Move-only types
// ---------------------------------------------------------------------------

TEST(HpcFixedStack, MoveOnlyType)
{
    hpc::core::fixed_stack<std::unique_ptr<int>, 4> s;
    s.push(std::make_unique<int>(42));
    s.push(std::make_unique<int>(99));

    EXPECT_EQ(*s.top(), 99);

    std::unique_ptr<int> out;
    EXPECT_TRUE(s.try_pop(out));
    EXPECT_EQ(*out, 99);
    EXPECT_TRUE(s.try_pop(out));
    EXPECT_EQ(*out, 42);
    EXPECT_TRUE(s.empty());
}

// ---------------------------------------------------------------------------
// Over-aligned types
// ---------------------------------------------------------------------------

struct alignas(128) OverAligned {
    std::int64_t value;
};

TEST(HpcFixedStack, OverAlignedType)
{
    hpc::core::fixed_stack<OverAligned, 4> s;
    s.push(OverAligned{100});
    s.push(OverAligned{200});

    EXPECT_EQ(s.top().value, 200);
    s.pop();
    EXPECT_EQ(s.top().value, 100);

    // Verify alignment of the storage
    auto addr = reinterpret_cast<std::uintptr_t>(&s.top());
    EXPECT_EQ(addr % alignof(OverAligned), 0u);
}

// ---------------------------------------------------------------------------
// Equality
// ---------------------------------------------------------------------------

TEST(HpcFixedStack, Equality)
{
    hpc::core::fixed_stack<int, 8> a{1, 2, 3};
    hpc::core::fixed_stack<int, 8> b{1, 2, 3};
    hpc::core::fixed_stack<int, 8> c{1, 2, 4};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

// ---------------------------------------------------------------------------
// LIFO stress: push N, pop N, verify ordering
// ---------------------------------------------------------------------------

TEST(HpcFixedStack, LIFOStress)
{
    constexpr std::size_t N = 256;
    hpc::core::fixed_stack<int, N> s;

    for (int i = 0; i < static_cast<int>(N); ++i)
        s.push(i);

    EXPECT_TRUE(s.full());

    for (int i = static_cast<int>(N) - 1; i >= 0; --i) {
        EXPECT_EQ(s.top(), i);
        s.pop();
    }

    EXPECT_TRUE(s.empty());
}

// ---------------------------------------------------------------------------
// Const access
// ---------------------------------------------------------------------------

TEST(HpcFixedStack, ConstTop)
{
    hpc::core::fixed_stack<int, 4> s;
    s.push(42);
    const auto& cs = s;
    EXPECT_EQ(cs.top(), 42);
    EXPECT_EQ(cs.size(), 1u);
    EXPECT_FALSE(cs.empty());
    EXPECT_FALSE(cs.full());
}

