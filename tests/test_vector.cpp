#include <gtest/gtest.h>
#include <hpc/core/vector.hpp>
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <utility>
TEST(HpcVector, DefaultConstruct)
{
    hpc::core::vector<int> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0u);
    EXPECT_EQ(v.capacity(), 0u);
    EXPECT_EQ(v.data(), nullptr);
}
TEST(HpcVector, ConstructWithCount)
{
    hpc::core::vector<int> v(5);
    EXPECT_EQ(v.size(), 5u);
    for (auto& e : v) EXPECT_EQ(e, 0);
}
TEST(HpcVector, ConstructWithCountAndValue)
{
    hpc::core::vector<int> v(4, 42);
    EXPECT_EQ(v.size(), 4u);
    for (auto& e : v) EXPECT_EQ(e, 42);
}
TEST(HpcVector, InitializerListConstruct)
{
    hpc::core::vector<int> v{1, 2, 3, 4, 5};
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[4], 5);
}
TEST(HpcVector, IteratorRangeConstruct)
{
    std::vector<int> src{10, 20, 30};
    hpc::core::vector<int> v(src.begin(), src.end());
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[2], 30);
}
TEST(HpcVector, CopyConstruct)
{
    hpc::core::vector<int> a{1, 2, 3};
    hpc::core::vector<int> b(a);
    EXPECT_EQ(a, b);
    a[0] = 99;
    EXPECT_NE(a, b);
}
TEST(HpcVector, MoveConstruct)
{
    hpc::core::vector<int> a{1, 2, 3};
    hpc::core::vector<int> b(std::move(a));
    EXPECT_EQ(b.size(), 3u);
    EXPECT_TRUE(a.empty());
}
TEST(HpcVector, CopyAssign)
{
    hpc::core::vector<int> a{1, 2, 3};
    hpc::core::vector<int> b;
    b = a;
    EXPECT_EQ(a, b);
}
TEST(HpcVector, MoveAssign)
{
    hpc::core::vector<int> a{4, 5, 6};
    hpc::core::vector<int> b;
    b = std::move(a);
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(b[0], 4);
}
TEST(HpcVector, InitializerListAssign)
{
    hpc::core::vector<int> v;
    v = {7, 8, 9};
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[1], 8);
}
TEST(HpcVector, AtBoundsCheck)
{
    hpc::core::vector<int> v{1, 2};
    EXPECT_EQ(v.at(0), 1);
    EXPECT_THROW(v.at(2), std::out_of_range);
}
TEST(HpcVector, FrontBack)
{
    hpc::core::vector<int> v{10, 20, 30};
    EXPECT_EQ(v.front(), 10);
    EXPECT_EQ(v.back(), 30);
}
TEST(HpcVector, DataPointer)
{
    hpc::core::vector<int> v{1, 2, 3};
    int* p = v.data();
    EXPECT_EQ(p[0], 1);
    EXPECT_EQ(p[2], 3);
}
TEST(HpcVector, ForwardIteration)
{
    hpc::core::vector<int> v{1, 2, 3};
    int sum = 0;
    for (auto it = v.begin(); it != v.end(); ++it) sum += *it;
    EXPECT_EQ(sum, 6);
}
TEST(HpcVector, ReverseIteration)
{
    hpc::core::vector<int> v{1, 2, 3};
    std::vector<int> rev(v.rbegin(), v.rend());
    EXPECT_EQ(rev[0], 3);
    EXPECT_EQ(rev[2], 1);
}
TEST(HpcVector, ConstIterators)
{
    const hpc::core::vector<int> v{4, 5, 6};
    int sum = 0;
    for (auto it = v.cbegin(); it != v.cend(); ++it) sum += *it;
    EXPECT_EQ(sum, 15);
}
TEST(HpcVector, RangeForLoop)
{
    hpc::core::vector<int> v{10, 20, 30};
    int sum = 0;
    for (auto x : v) sum += x;
    EXPECT_EQ(sum, 60);
}
TEST(HpcVector, ReserveIncreasesCapacity)
{
    hpc::core::vector<int> v;
    v.reserve(100);
    EXPECT_GE(v.capacity(), 100u);
    EXPECT_EQ(v.size(), 0u);
}
TEST(HpcVector, ShrinkToFit)
{
    hpc::core::vector<int> v;
    v.reserve(128);
    v.push_back(1);
    v.push_back(2);
    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), v.size());
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
}
TEST(HpcVector, PushBackCopy)
{
    hpc::core::vector<int> v;
    for (int i = 0; i < 100; ++i) v.push_back(i);
    EXPECT_EQ(v.size(), 100u);
    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[99], 99);
}
TEST(HpcVector, PushBackMove)
{
    hpc::core::vector<std::string> v;
    std::string s = "hello";
    v.push_back(std::move(s));
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], "hello");
}
TEST(HpcVector, EmplaceBack)
{
    hpc::core::vector<std::pair<int, std::string>> v;
    v.emplace_back(1, "one");
    v.emplace_back(2, "two");
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0].first, 1);
    EXPECT_EQ(v[1].second, "two");
}
TEST(HpcVector, PopBack)
{
    hpc::core::vector<int> v{1, 2, 3};
    v.pop_back();
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v.back(), 2);
}
TEST(HpcVector, InsertAtBeginning)
{
    hpc::core::vector<int> v{2, 3, 4};
    v.insert(v.begin(), 1);
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
}
TEST(HpcVector, InsertInMiddle)
{
    hpc::core::vector<int> v{1, 2, 4, 5};
    v.insert(v.begin() + 2, 3);
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v[3], 4);
}
TEST(HpcVector, InsertAtEnd)
{
    hpc::core::vector<int> v{1, 2};
    v.insert(v.end(), 3);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v.back(), 3);
}
TEST(HpcVector, EmplaceInMiddle)
{
    hpc::core::vector<std::pair<int, int>> v;
    v.emplace_back(1, 10);
    v.emplace_back(3, 30);
    v.emplace(v.begin() + 1, 2, 20);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[1].first, 2);
}
TEST(HpcVector, EraseSingle)
{
    hpc::core::vector<int> v{1, 2, 3, 4};
    auto it = v.erase(v.begin() + 1);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(*it, 3);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 4);
}
TEST(HpcVector, EraseRange)
{
    hpc::core::vector<int> v{1, 2, 3, 4, 5};
    v.erase(v.begin() + 1, v.begin() + 3);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 4);
    EXPECT_EQ(v[2], 5);
}
TEST(HpcVector, Clear)
{
    hpc::core::vector<int> v{1, 2, 3};
    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_GE(v.capacity(), 3u);
}
TEST(HpcVector, ResizeGrow)
{
    hpc::core::vector<int> v{1, 2};
    v.resize(5);
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[4], 0);
}
TEST(HpcVector, ResizeShrink)
{
    hpc::core::vector<int> v{1, 2, 3, 4, 5};
    v.resize(2);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[1], 2);
}
TEST(HpcVector, ResizeWithValue)
{
    hpc::core::vector<int> v{1};
    v.resize(4, 99);
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[3], 99);
}
TEST(HpcVector, Swap)
{
    hpc::core::vector<int> a{1, 2};
    hpc::core::vector<int> b{3, 4, 5};
    a.swap(b);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(a[0], 3);
    EXPECT_EQ(b[0], 1);
}
TEST(HpcVector, AssignCountValue)
{
    hpc::core::vector<int> v{1, 2, 3};
    v.assign(5, 7);
    EXPECT_EQ(v.size(), 5u);
    for (auto& e : v) EXPECT_EQ(e, 7);
}
TEST(HpcVector, AssignIteratorRange)
{
    std::vector<int> src{10, 20, 30};
    hpc::core::vector<int> v;
    v.assign(src.begin(), src.end());
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[1], 20);
}
TEST(HpcVector, AssignInitializerList)
{
    hpc::core::vector<int> v;
    v.assign({100, 200});
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 100);
}
TEST(HpcVector, Equality)
{
    hpc::core::vector<int> a{1, 2, 3};
    hpc::core::vector<int> b{1, 2, 3};
    hpc::core::vector<int> c{1, 2, 4};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}
TEST(HpcVector, LessThan)
{
    hpc::core::vector<int> a{1, 2, 3};
    hpc::core::vector<int> b{1, 2, 4};
    EXPECT_LT(a, b);
    EXPECT_GT(b, a);
    EXPECT_LE(a, a);
    EXPECT_GE(b, b);
}
TEST(HpcVector, GrowthStress)
{
    hpc::core::vector<int> v;
    constexpr int N = 10000;
    for (int i = 0; i < N; ++i) v.push_back(i);
    EXPECT_EQ(v.size(), static_cast<std::size_t>(N));
    for (int i = 0; i < N; ++i) EXPECT_EQ(v[i], i);
}
TEST(HpcVector, StringType)
{
    hpc::core::vector<std::string> v;
    v.push_back("hello");
    v.push_back("world");
    v.emplace_back("!!!");
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], "hello");
    v.erase(v.begin());
    EXPECT_EQ(v[0], "world");
}
TEST(HpcVector, StdSort)
{
    hpc::core::vector<int> v{5, 3, 1, 4, 2};
    std::sort(v.begin(), v.end());
    for (std::size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(v[i], static_cast<int>(i + 1));
    }
}
TEST(HpcVector, StdAccumulate)
{
    hpc::core::vector<int> v{1, 2, 3, 4, 5};
    int sum = std::accumulate(v.begin(), v.end(), 0);
    EXPECT_EQ(sum, 15);
}
TEST(HpcVector, StdFind)
{
    hpc::core::vector<int> v{10, 20, 30, 40};
    auto it = std::find(v.begin(), v.end(), 30);
    EXPECT_NE(it, v.end());
    EXPECT_EQ(*it, 30);
}
TEST(HpcVector, MoveOnlyType)
{
    hpc::core::vector<std::unique_ptr<int>> v;
    v.push_back(std::make_unique<int>(1));
    v.push_back(std::make_unique<int>(2));
    v.emplace_back(std::make_unique<int>(3));
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(*v[0], 1);
    EXPECT_EQ(*v[2], 3);
}

TEST(HpcVector, OverAlignedType)
{
    struct alignas(64) cache_line_aligned {
        std::uint64_t data[8]{};
        bool operator==(const cache_line_aligned& o) const { return data[0] == o.data[0]; }
    };

    hpc::core::vector<cache_line_aligned> v;
    for (int i = 0; i < 16; ++i) {
        cache_line_aligned obj{};
        obj.data[0] = static_cast<std::uint64_t>(i);
        v.push_back(obj);
    }

    EXPECT_EQ(v.size(), 16u);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(v[i].data[0], static_cast<std::uint64_t>(i));
        // Verify each element is actually aligned to 64 bytes.
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&v[i]) % 64, 0u);
    }
}

