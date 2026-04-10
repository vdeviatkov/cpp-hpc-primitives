#include <benchmark/benchmark.h>

#include <hpc/core/vector.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace {

void BM_StdVector_PushBack_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        std::vector<int> v;
        for (std::size_t i = 0; i < n; ++i) v.push_back(static_cast<int>(i));
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcVector_PushBack_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        hpc::core::vector<int> v;
        for (std::size_t i = 0; i < n; ++i) v.push_back(static_cast<int>(i));
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_StdVector_PushBackReserved_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        std::vector<int> v;
        v.reserve(n);
        for (std::size_t i = 0; i < n; ++i) v.push_back(static_cast<int>(i));
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcVector_PushBackReserved_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        hpc::core::vector<int> v;
        v.reserve(n);
        for (std::size_t i = 0; i < n; ++i) v.push_back(static_cast<int>(i));
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_StdVector_EmplaceBack_String(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        std::vector<std::string> v;
        for (std::size_t i = 0; i < n; ++i) v.emplace_back("benchmark_string");
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcVector_EmplaceBack_String(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        hpc::core::vector<std::string> v;
        for (std::size_t i = 0; i < n; ++i) v.emplace_back("benchmark_string");
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_StdVector_RandomAccess(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::vector<int> v(n);
    for (std::size_t i = 0; i < n; ++i) v[i] = static_cast<int>(i);

    for (auto _ : state) {
        std::int64_t sum = 0;
        for (std::size_t i = 0; i < n; ++i) sum += v[i];
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcVector_RandomAccess(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::vector<int> v(n);
    for (std::size_t i = 0; i < n; ++i) v[i] = static_cast<int>(i);

    for (auto _ : state) {
        std::int64_t sum = 0;
        for (std::size_t i = 0; i < n; ++i) sum += v[i];
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_StdVector_Iterate(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::vector<int> v(n, 1);

    for (auto _ : state) {
        std::int64_t sum = 0;
        for (auto x : v) sum += x;
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcVector_Iterate(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::vector<int> v(n);
    for (std::size_t i = 0; i < n; ++i) v[i] = 1;

    for (auto _ : state) {
        std::int64_t sum = 0;
        for (auto x : v) sum += x;
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_StdVector_InsertFront(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        std::vector<int> v;
        for (std::size_t i = 0; i < n; ++i) v.insert(v.begin(), static_cast<int>(i));
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcVector_InsertFront(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        hpc::core::vector<int> v;
        for (std::size_t i = 0; i < n; ++i) v.insert(v.begin(), static_cast<int>(i));
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_StdVector_PopBack(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> v(n, 42);
        state.ResumeTiming();
        while (!v.empty()) {
            v.pop_back();
        }
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcVector_PopBack(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        hpc::core::vector<int> v(n);
        for (std::size_t i = 0; i < n; ++i) v[i] = 42;
        state.ResumeTiming();
        while (!v.empty()) {
            v.pop_back();
        }
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

} // namespace

BENCHMARK(BM_StdVector_PushBack_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcVector_PushBack_Int)->RangeMultiplier(4)->Range(64, 1 << 16);

BENCHMARK(BM_StdVector_PushBackReserved_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcVector_PushBackReserved_Int)->RangeMultiplier(4)->Range(64, 1 << 16);

BENCHMARK(BM_StdVector_EmplaceBack_String)->RangeMultiplier(4)->Range(64, 1 << 14);
BENCHMARK(BM_HpcVector_EmplaceBack_String)->RangeMultiplier(4)->Range(64, 1 << 14);

BENCHMARK(BM_StdVector_RandomAccess)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcVector_RandomAccess)->RangeMultiplier(4)->Range(64, 1 << 16);

BENCHMARK(BM_StdVector_Iterate)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcVector_Iterate)->RangeMultiplier(4)->Range(64, 1 << 16);

BENCHMARK(BM_StdVector_InsertFront)->RangeMultiplier(4)->Range(64, 1 << 12);
BENCHMARK(BM_HpcVector_InsertFront)->RangeMultiplier(4)->Range(64, 1 << 12);

BENCHMARK(BM_StdVector_PopBack)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcVector_PopBack)->RangeMultiplier(4)->Range(64, 1 << 16);

