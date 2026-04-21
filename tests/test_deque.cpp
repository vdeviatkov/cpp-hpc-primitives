#include <gtest/gtest.h>
#include <hpc/core/deque.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(HpcDeque, DefaultConstruct)
{
    hpc::core::deque<int> d;
    EXPECT_TRUE(d.empty());
    EXPECT_EQ(d.size(), 0u);
}

TEST(HpcDeque, ConstructWithCount)
{
    hpc::core::deque<int> d(5);
    EXPECT_EQ(d.size(), 5u);
    for (auto& e : d) EXPECT_EQ(e, 0);
}

TEST(HpcDeque, ConstructWithCountAndValue)
{
    hpc::core::deque<int> d(4, 42);
    EXPECT_EQ(d.size(), 4u);
    for (auto& e : d) EXPECT_EQ(e, 42);
}

TEST(HpcDeque, InitializerListConstruct)
{
    hpc::core::deque<int> d{1, 2, 3, 4, 5};
    EXPECT_EQ(d.size(), 5u);
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[4], 5);
}

TEST(HpcDeque, IteratorRangeConstruct)
{
    std::vector<int> src{10, 20, 30};
    hpc::core::deque<int> d(src.begin(), src.end());
    EXPECT_EQ(d.size(), 3u);
    EXPECT_EQ(d[0], 10);
    EXPECT_EQ(d[2], 30);
}

TEST(HpcDeque, CopyConstruct)
{
    hpc::core::deque<int> a{1, 2, 3};
    hpc::core::deque<int> b(a);
    EXPECT_EQ(a, b);
    a[0] = 99;
    EXPECT_NE(a, b);
}

TEST(HpcDeque, MoveConstruct)
{
    hpc::core::deque<int> a{1, 2, 3};
    hpc::core::deque<int> b(std::move(a));
    EXPECT_EQ(b.size(), 3u);
    EXPECT_TRUE(a.empty());
}

TEST(HpcDeque, CopyAssign)
{
    hpc::core::deque<int> a{1, 2, 3};
    hpc::core::deque<int> b;
    b = a;
    EXPECT_EQ(a, b);
}

TEST(HpcDeque, MoveAssign)
{
    hpc::core::deque<int> a{4, 5, 6};
    hpc::core::deque<int> b;
    b = std::move(a);
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(b[0], 4);
}

TEST(HpcDeque, InitializerListAssign)
{
    hpc::core::deque<int> d;
    d = {7, 8, 9};
    EXPECT_EQ(d.size(), 3u);
    EXPECT_EQ(d[1], 8);
}

// ---------------------------------------------------------------------------
// Element access
// ---------------------------------------------------------------------------

TEST(HpcDeque, AtBoundsCheck)
{
    hpc::core::deque<int> d{1, 2};
    EXPECT_EQ(d.at(0), 1);
    EXPECT_THROW(d.at(2), std::out_of_range);
}

TEST(HpcDeque, FrontBack)
{
    hpc::core::deque<int> d{10, 20, 30};
    EXPECT_EQ(d.front(), 10);
    EXPECT_EQ(d.back(), 30);
}

TEST(HpcDeque, RandomAccess)
{
    hpc::core::deque<int> d{10, 20, 30, 40};
    EXPECT_EQ(d[0], 10);
    EXPECT_EQ(d[3], 40);
    d[1] = 99;
    EXPECT_EQ(d[1], 99);
}

// ---------------------------------------------------------------------------
// Push / Pop back
// ---------------------------------------------------------------------------

TEST(HpcDeque, PushBackPopBack)
{
    hpc::core::deque<int> d;
    d.push_back(1);
    d.push_back(2);
    d.push_back(3);
    EXPECT_EQ(d.size(), 3u);
    EXPECT_EQ(d.back(), 3);
    d.pop_back();
    EXPECT_EQ(d.back(), 2);
    d.pop_back();
    EXPECT_EQ(d.back(), 1);
    d.pop_back();
    EXPECT_TRUE(d.empty());
}

TEST(HpcDeque, EmplaceBack)
{
    hpc::core::deque<std::pair<int, std::string>> d;
    d.emplace_back(1, "one");
    d.emplace_back(2, "two");
    EXPECT_EQ(d.back().first, 2);
    EXPECT_EQ(d.back().second, "two");
}

// ---------------------------------------------------------------------------
// Push / Pop front
// ---------------------------------------------------------------------------

TEST(HpcDeque, PushFrontPopFront)
{
    hpc::core::deque<int> d;
    d.push_front(1);
    d.push_front(2);
    d.push_front(3);
    EXPECT_EQ(d.size(), 3u);
    EXPECT_EQ(d.front(), 3);
    d.pop_front();
    EXPECT_EQ(d.front(), 2);
    d.pop_front();
    EXPECT_EQ(d.front(), 1);
    d.pop_front();
    EXPECT_TRUE(d.empty());
}

TEST(HpcDeque, EmplaceFront)
{
    hpc::core::deque<std::pair<int, std::string>> d;
    d.emplace_front(1, "one");
    d.emplace_front(2, "two");
    EXPECT_EQ(d.front().first, 2);
    EXPECT_EQ(d.front().second, "two");
}

// ---------------------------------------------------------------------------
// Mixed front/back operations
// ---------------------------------------------------------------------------

TEST(HpcDeque, MixedPushFrontBack)
{
    hpc::core::deque<int> d;
    d.push_back(3);
    d.push_front(2);
    d.push_back(4);
    d.push_front(1);
    // Expected order: 1, 2, 3, 4
    EXPECT_EQ(d.size(), 4u);
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[1], 2);
    EXPECT_EQ(d[2], 3);
    EXPECT_EQ(d[3], 4);
}

// ---------------------------------------------------------------------------
// Iterators
// ---------------------------------------------------------------------------

TEST(HpcDeque, ForwardIteration)
{
    hpc::core::deque<int> d{1, 2, 3};
    int sum = 0;
    for (auto it = d.begin(); it != d.end(); ++it) sum += *it;
    EXPECT_EQ(sum, 6);
}

TEST(HpcDeque, ReverseIteration)
{
    hpc::core::deque<int> d{1, 2, 3};
    std::vector<int> rev(d.rbegin(), d.rend());
    EXPECT_EQ(rev[0], 3);
    EXPECT_EQ(rev[2], 1);
}

TEST(HpcDeque, ConstIterators)
{
    const hpc::core::deque<int> d{4, 5, 6};
    int sum = 0;
    for (auto it = d.cbegin(); it != d.cend(); ++it) sum += *it;
    EXPECT_EQ(sum, 15);
}

TEST(HpcDeque, RangeFor)
{
    hpc::core::deque<int> d{10, 20, 30};
    int sum = 0;
    for (auto x : d) sum += x;
    EXPECT_EQ(sum, 60);
}

TEST(HpcDeque, IteratorRandomAccess)
{
    hpc::core::deque<int> d{10, 20, 30, 40, 50};
    auto it = d.begin();
    EXPECT_EQ(it[2], 30);
    EXPECT_EQ(*(it + 4), 50);
    EXPECT_EQ(d.end() - d.begin(), 5);
}

TEST(HpcDeque, StdAlgorithmSort)
{
    hpc::core::deque<int> d{5, 3, 1, 4, 2};
    std::sort(d.begin(), d.end());
    for (std::size_t i = 0; i < d.size(); ++i)
        EXPECT_EQ(d[i], static_cast<int>(i + 1));
}

// ---------------------------------------------------------------------------
// Reference stability (std::deque guarantee)
// ---------------------------------------------------------------------------

TEST(HpcDeque, ReferenceStabilityPushBack)
{
    hpc::core::deque<int> d;
    d.push_back(42);
    int& ref = d.front();
    // Push enough to trigger chunk allocation / map reallocation.
    for (int i = 0; i < 10000; ++i) d.push_back(i);
    // Reference to first element must still be valid.
    EXPECT_EQ(ref, 42);
}

TEST(HpcDeque, ReferenceStabilityPushFront)
{
    hpc::core::deque<int> d;
    d.push_back(99);
    int& ref = d.back();
    for (int i = 0; i < 10000; ++i) d.push_front(i);
    EXPECT_EQ(ref, 99);
}

// ---------------------------------------------------------------------------
// Resize
// ---------------------------------------------------------------------------

TEST(HpcDeque, ResizeGrow)
{
    hpc::core::deque<int> d{1, 2, 3};
    d.resize(5);
    EXPECT_EQ(d.size(), 5u);
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[3], 0);
    EXPECT_EQ(d[4], 0);
}

TEST(HpcDeque, ResizeShrink)
{
    hpc::core::deque<int> d{1, 2, 3, 4, 5};
    d.resize(2);
    EXPECT_EQ(d.size(), 2u);
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[1], 2);
}

TEST(HpcDeque, ResizeWithValue)
{
    hpc::core::deque<int> d{1};
    d.resize(4, 42);
    EXPECT_EQ(d.size(), 4u);
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[1], 42);
    EXPECT_EQ(d[3], 42);
}

// ---------------------------------------------------------------------------
// Clear / Swap
// ---------------------------------------------------------------------------

TEST(HpcDeque, Clear)
{
    hpc::core::deque<int> d{1, 2, 3};
    d.clear();
    EXPECT_TRUE(d.empty());
}

TEST(HpcDeque, Swap)
{
    hpc::core::deque<int> a{1, 2};
    hpc::core::deque<int> b{10, 20, 30};
    a.swap(b);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a.front(), 10);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b.front(), 1);
}

// ---------------------------------------------------------------------------
// Growth under wrap-around
// ---------------------------------------------------------------------------

TEST(HpcDeque, GrowWhileWrapped)
{
    hpc::core::deque<int> d;
    for (int i = 0; i < 6; ++i) d.push_back(i);
    for (int i = 0; i < 4; ++i) d.pop_front();
    // head_ > 0 now; push enough to trigger growth.
    for (int i = 100; i < 200; ++i) d.push_back(i);
    EXPECT_EQ(d.front(), 4);
    d.pop_front(); EXPECT_EQ(d.front(), 5);
    d.pop_front();
    for (int i = 100; i < 200; ++i) {
        EXPECT_EQ(d.front(), i);
        d.pop_front();
    }
    EXPECT_TRUE(d.empty());
}

TEST(HpcDeque, GrowViaPushFront)
{
    hpc::core::deque<int> d;
    for (int i = 0; i < 100; ++i) d.push_front(i);
    EXPECT_EQ(d.size(), 100u);
    // push_front inserts in reverse order at the front.
    for (int i = 0; i < 100; ++i)
        EXPECT_EQ(d[i], 99 - i);
}

// ---------------------------------------------------------------------------
// Non-trivial types (std::string)
// ---------------------------------------------------------------------------

TEST(HpcDeque, StringPushPop)
{
    hpc::core::deque<std::string> d;
    d.push_back("hello");
    d.push_front("world");
    EXPECT_EQ(d.front(), "world");
    EXPECT_EQ(d.back(), "hello");
}

TEST(HpcDeque, StringMoveSemantics)
{
    hpc::core::deque<std::string> d;
    std::string v = "moved";
    d.push_back(std::move(v));
    EXPECT_EQ(d.back(), "moved");
}

// ---------------------------------------------------------------------------
// Move-only types
// ---------------------------------------------------------------------------

TEST(HpcDeque, MoveOnlyType)
{
    hpc::core::deque<std::unique_ptr<int>> d;
    d.push_back(std::make_unique<int>(42));
    d.push_front(std::make_unique<int>(99));
    EXPECT_EQ(*d.front(), 99);
    EXPECT_EQ(*d.back(), 42);
    d.pop_front();
    EXPECT_EQ(*d.front(), 42);
}

// ---------------------------------------------------------------------------
// Over-aligned types
// ---------------------------------------------------------------------------

struct alignas(128) DequeOverAligned {
    std::int64_t value;
};

TEST(HpcDeque, OverAlignedType)
{
    hpc::core::deque<DequeOverAligned> d;
    d.push_back(DequeOverAligned{100});
    d.push_back(DequeOverAligned{200});
    EXPECT_EQ(d.front().value, 100);
    EXPECT_EQ(d.back().value, 200);
    auto addr = reinterpret_cast<std::uintptr_t>(&d.front());
    EXPECT_EQ(addr % alignof(DequeOverAligned), 0u);
}

// ---------------------------------------------------------------------------
// Equality
// ---------------------------------------------------------------------------

TEST(HpcDeque, Equality)
{
    hpc::core::deque<int> a{1, 2, 3};
    hpc::core::deque<int> b{1, 2, 3};
    hpc::core::deque<int> c{1, 2, 4};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

// ---------------------------------------------------------------------------
// Stress
// ---------------------------------------------------------------------------

TEST(HpcDeque, Stress)
{
    constexpr int N = 5000;
    hpc::core::deque<int> d;
    for (int i = 0; i < N; ++i) d.push_back(i);
    for (int i = 0; i < N; ++i) d.push_front(-i - 1);
    // Elements: [-5000, ..., -1, 0, 1, ..., 4999]
    EXPECT_EQ(d.size(), static_cast<std::size_t>(2 * N));
    for (int i = 0; i < 2 * N; ++i)
        EXPECT_EQ(d[i], i - N);
}

// ---------------------------------------------------------------------------
// Const access
// ---------------------------------------------------------------------------

TEST(HpcDeque, ConstAccess)
{
    hpc::core::deque<int> d{1, 2, 3};
    const auto& cd = d;
    EXPECT_EQ(cd.front(), 1);
    EXPECT_EQ(cd.back(), 3);
    EXPECT_EQ(cd[1], 2);
    EXPECT_EQ(cd.size(), 3u);
    EXPECT_FALSE(cd.empty());
}

