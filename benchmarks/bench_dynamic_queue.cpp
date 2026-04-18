#include <benchmark/benchmark.h>

#include <hpc/core/dynamic_queue.hpp>

#include <cstdint>
#include <queue>
#include <string>

namespace {

// ---------------------------------------------------------------------------
// Push N integers
// ---------------------------------------------------------------------------

void BM_StdQueue_Push_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::queue<int> q;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) q.push(static_cast<int>(i));
        benchmark::DoNotOptimize(&q);
        state.PauseTiming();
        while (!q.empty()) q.pop();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcQueue_Push_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::queue<int> q;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) q.push(static_cast<int>(i));
        benchmark::DoNotOptimize(&q);
        state.PauseTiming();
        q.clear();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// Push N integers (pre-reserved)
// ---------------------------------------------------------------------------

void BM_HpcQueue_PushReserved_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::queue<int> q;
    q.reserve(n);
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) q.push(static_cast<int>(i));
        benchmark::DoNotOptimize(&q);
        state.PauseTiming();
        q.clear();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// Pop N integers (pre-filled)
// ---------------------------------------------------------------------------

void BM_StdQueue_Pop_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::queue<int> q;
    for (auto _ : state) {
        state.PauseTiming();
        for (std::size_t i = 0; i < n; ++i) q.push(static_cast<int>(i));
        state.ResumeTiming();
        while (!q.empty()) {
            benchmark::DoNotOptimize(q.front());
            q.pop();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcQueue_Pop_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::queue<int> q;
    for (auto _ : state) {
        state.PauseTiming();
        for (std::size_t i = 0; i < n; ++i) q.push(static_cast<int>(i));
        state.ResumeTiming();
        while (!q.empty()) {
            benchmark::DoNotOptimize(q.front());
            q.pop();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// Push + Pop round-trip (interleaved, depth-1)
// ---------------------------------------------------------------------------

void BM_StdQueue_PushPop_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::queue<int> q;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) {
            q.push(static_cast<int>(i));
            benchmark::DoNotOptimize(q.front());
            q.pop();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcQueue_PushPop_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::queue<int> q;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) {
            q.push(static_cast<int>(i));
            benchmark::DoNotOptimize(q.front());
            q.pop();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// Push N strings (non-trivial type)
// ---------------------------------------------------------------------------

void BM_StdQueue_Push_String(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::queue<std::string> q;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) q.push("benchmark_string");
        benchmark::DoNotOptimize(&q);
        state.PauseTiming();
        while (!q.empty()) q.pop();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

void BM_HpcQueue_Push_String(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    hpc::core::queue<std::string> q;
    for (auto _ : state) {
        for (std::size_t i = 0; i < n; ++i) q.push("benchmark_string");
        benchmark::DoNotOptimize(&q);
        state.PauseTiming();
        q.clear();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

} // namespace

// -- Push int --
BENCHMARK(BM_StdQueue_Push_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcQueue_Push_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcQueue_PushReserved_Int)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- Pop int --
BENCHMARK(BM_StdQueue_Pop_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcQueue_Pop_Int)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- Push+Pop round-trip int --
BENCHMARK(BM_StdQueue_PushPop_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcQueue_PushPop_Int)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- Push string --
BENCHMARK(BM_StdQueue_Push_String)->RangeMultiplier(4)->Range(64, 1 << 14);
BENCHMARK(BM_HpcQueue_Push_String)->RangeMultiplier(4)->Range(64, 1 << 14);

