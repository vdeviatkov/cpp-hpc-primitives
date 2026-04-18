add_executable(hpc_benchmarks
    bench_queue.cpp
    bench_allocator.cpp
    bench_spinlock.cpp
    bench_mpmc_ring_buffer.cpp
    bench_vector.cpp
    bench_stack.cpp
    bench_fixed_queue.cpp
    bench_dynamic_queue.cpp
)

# NUMA-specific benchmarks only make sense when NUMA support is enabled.
# On non-Linux platforms (or when libnuma isn't available), hpc_core is built
# without numa_arena/numa_pool implementations.
if (HPC_HAS_NUMA_H AND HPC_HAS_LIBNUMA)
    target_sources(hpc_benchmarks PRIVATE bench_numa_hugepages.cpp)
else()
    message(STATUS "Skipping bench_numa_hugepages.cpp (NUMA support not available)")
endif()

target_link_libraries(hpc_benchmarks PRIVATE
    hpc_core
    benchmark::benchmark_main
)

# Favor performance for benchmarks
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hpc_benchmarks PRIVATE -O3 -DNDEBUG -march=native)
endif()
