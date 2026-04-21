#include <benchmark/benchmark.h>

#include <hpc/core/deque.hpp>

#include <cstdint>
#include <deque>
#include <numeric>
#include <string>

namespace {

// ---------------------------------------------------------------------------
// push_back N integers
// ---------------------------------------------------------------------------

void BM_StdDeque_PushBack_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::deque<int> d;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) d.push_back(static_cast<int>(i));
        benchmark::DoNotOptimize(&d);
        state.PauseTiming();
        d.clear();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcDeque_PushBack_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::deque<int> d;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) d.push_back(static_cast<int>(i));
        benchmark::DoNotOptimize(&d);
        state.PauseTiming();
        d.clear();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}


// ---------------------------------------------------------------------------
// push_front N integers
// ---------------------------------------------------------------------------

void BM_StdDeque_PushFront_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::deque<int> d;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) d.push_front(static_cast<int>(i));
        benchmark::DoNotOptimize(&d);
        state.PauseTiming();
        d.clear();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcDeque_PushFront_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::deque<int> d;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) d.push_front(static_cast<int>(i));
        benchmark::DoNotOptimize(&d);
        state.PauseTiming();
        d.clear();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// pop_back N integers (pre-filled)
// ---------------------------------------------------------------------------

void BM_StdDeque_PopBack_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::deque<int> d;
    for (auto _ : state) {
        state.PauseTiming();
        d.assign(n, 42);
        state.ResumeTiming();
        while (!d.empty()) {
            benchmark::DoNotOptimize(d.back());
            d.pop_back();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcDeque_PopBack_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::deque<int> d;
    for (auto _ : state) {
        state.PauseTiming();
        for (std::size_t i = 0; i < n; ++i) d.push_back(42);
        state.ResumeTiming();
        while (!d.empty()) {
            benchmark::DoNotOptimize(d.back());
            d.pop_back();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// pop_front N integers (pre-filled)
// ---------------------------------------------------------------------------

void BM_StdDeque_PopFront_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::deque<int> d;
    for (auto _ : state) {
        state.PauseTiming();
        d.assign(n, 42);
        state.ResumeTiming();
        while (!d.empty()) {
            benchmark::DoNotOptimize(d.front());
            d.pop_front();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcDeque_PopFront_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::deque<int> d;
    for (auto _ : state) {
        state.PauseTiming();
        for (std::size_t i = 0; i < n; ++i) d.push_back(42);
        state.ResumeTiming();
        while (!d.empty()) {
            benchmark::DoNotOptimize(d.front());
            d.pop_front();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// Random access (iteration via operator[])
// ---------------------------------------------------------------------------

void BM_StdDeque_RandomAccess(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::deque<int> d(n);
    std::iota(d.begin(), d.end(), 0);
    for (auto _ : state) {
        std::int64_t sum = 0;
        for (std::size_t i = 0; i < n; ++i) sum += d[i];
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcDeque_RandomAccess(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::deque<int> d;
    for (std::size_t i = 0; i < n; ++i) d.push_back(static_cast<int>(i));
    for (auto _ : state) {
        std::int64_t sum = 0;
        for (std::size_t i = 0; i < n; ++i) sum += d[i];
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// Iterator traversal
// ---------------------------------------------------------------------------

void BM_StdDeque_Iterate(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::deque<int> d(n, 1);
    for (auto _ : state) {
        std::int64_t sum = 0;
        for (auto x : d) sum += x;
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcDeque_Iterate(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::deque<int> d(n, 1);
    for (auto _ : state) {
        std::int64_t sum = 0;
        for (auto x : d) sum += x;
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// push_back + pop_front round-trip (queue-style, depth-1)
// ---------------------------------------------------------------------------

void BM_StdDeque_PushBackPopFront_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::deque<int> d;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) {
            d.push_back(static_cast<int>(i));
            benchmark::DoNotOptimize(d.front());
            d.pop_front();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcDeque_PushBackPopFront_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::deque<int> d;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) {
            d.push_back(static_cast<int>(i));
            benchmark::DoNotOptimize(d.front());
            d.pop_front();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// push_back N strings (non-trivial type)
// ---------------------------------------------------------------------------

void BM_StdDeque_PushBack_String(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::deque<std::string> d;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) d.push_back("benchmark_string");
        benchmark::DoNotOptimize(&d);
        state.PauseTiming();
        d.clear();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcDeque_PushBack_String(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::deque<std::string> d;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) d.push_back("benchmark_string");
        benchmark::DoNotOptimize(&d);
        state.PauseTiming();
        d.clear();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

} // namespace

// -- push_back int --
BENCHMARK(BM_StdDeque_PushBack_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcDeque_PushBack_Int)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- push_front int --
BENCHMARK(BM_StdDeque_PushFront_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcDeque_PushFront_Int)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- pop_back int --
BENCHMARK(BM_StdDeque_PopBack_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcDeque_PopBack_Int)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- pop_front int --
BENCHMARK(BM_StdDeque_PopFront_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcDeque_PopFront_Int)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- random access --
BENCHMARK(BM_StdDeque_RandomAccess)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcDeque_RandomAccess)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- iterate --
BENCHMARK(BM_StdDeque_Iterate)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcDeque_Iterate)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- push_back + pop_front round-trip --
BENCHMARK(BM_StdDeque_PushBackPopFront_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcDeque_PushBackPopFront_Int)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- push_back string --
BENCHMARK(BM_StdDeque_PushBack_String)->RangeMultiplier(4)->Range(64, 1 << 14);
BENCHMARK(BM_HpcDeque_PushBack_String)->RangeMultiplier(4)->Range(64, 1 << 14);


