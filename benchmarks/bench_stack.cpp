#include <benchmark/benchmark.h>

#include <hpc/core/stack.hpp>

#include <cstdint>
#include <stack>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Push N integers
// ---------------------------------------------------------------------------

void BM_StdStack_Push_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        std::stack<int> s;
        for (std::size_t i = 0; i < n; ++i) s.push(static_cast<int>(i));
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

template <std::size_t Cap>
void BM_HpcFixedStack_Push_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        hpc::core::fixed_stack<int, Cap> s;
        for (std::size_t i = 0; i < n; ++i) s.try_push(static_cast<int>(i));
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// Pop N integers (pre-filled)
// ---------------------------------------------------------------------------

void BM_StdStack_Pop_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        std::stack<int> s;
        for (std::size_t i = 0; i < n; ++i) s.push(static_cast<int>(i));
        state.ResumeTiming();
        while (!s.empty()) {
            benchmark::DoNotOptimize(s.top());
            s.pop();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

template <std::size_t Cap>
void BM_HpcFixedStack_Pop_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        hpc::core::fixed_stack<int, Cap> s;
        for (std::size_t i = 0; i < n; ++i) s.try_push(static_cast<int>(i));
        state.ResumeTiming();
        while (!s.empty()) {
            benchmark::DoNotOptimize(s.top());
            s.try_pop();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// Push + Pop round-trip (interleaved)
// ---------------------------------------------------------------------------

void BM_StdStack_PushPop_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        std::stack<int> s;
        for (std::size_t i = 0; i < n; ++i) {
            s.push(static_cast<int>(i));
            benchmark::DoNotOptimize(s.top());
            s.pop();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

template <std::size_t Cap>
void BM_HpcFixedStack_PushPop_Int(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        hpc::core::fixed_stack<int, Cap> s;
        for (std::size_t i = 0; i < n; ++i) {
            s.try_push(static_cast<int>(i));
            benchmark::DoNotOptimize(s.top());
            s.try_pop();
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// ---------------------------------------------------------------------------
// Push N strings (non-trivial type)
// ---------------------------------------------------------------------------

void BM_StdStack_Push_String(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        std::stack<std::string> s;
        for (std::size_t i = 0; i < n; ++i) s.push("benchmark_string");
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

template <std::size_t Cap>
void BM_HpcFixedStack_Push_String(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        hpc::core::fixed_stack<std::string, Cap> s;
        for (std::size_t i = 0; i < n; ++i) s.try_push(std::string("benchmark_string"));
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

// Use a capacity large enough for the biggest range value (1 << 16 = 65536).
constexpr std::size_t kMaxCap = 1u << 16;

} // namespace

// -- Push int --
BENCHMARK(BM_StdStack_Push_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcFixedStack_Push_Int<kMaxCap>)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- Pop int --
BENCHMARK(BM_StdStack_Pop_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcFixedStack_Pop_Int<kMaxCap>)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- Push+Pop round-trip int --
BENCHMARK(BM_StdStack_PushPop_Int)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(BM_HpcFixedStack_PushPop_Int<kMaxCap>)->RangeMultiplier(4)->Range(64, 1 << 16);

// -- Push string --
BENCHMARK(BM_StdStack_Push_String)->RangeMultiplier(4)->Range(64, 1 << 14);
BENCHMARK(BM_HpcFixedStack_Push_String<1u << 14>)->RangeMultiplier(4)->Range(64, 1 << 14);

