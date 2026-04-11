add_executable(hpc_tests
    test_ring_buffer_basic.cpp
    test_arena_allocator.cpp
    test_pool_allocator.cpp
    test_ttas_spinlock.cpp
    test_mpmc_ring_buffer.cpp
    test_huge_pages.cpp
    test_vector.cpp
    test_stack.cpp
)

# NUMA tests require libnuma-backed implementation.
if (HPC_HAS_NUMA_H AND HPC_HAS_LIBNUMA)
    target_sources(hpc_tests PRIVATE test_numa_memory.cpp)
else()
    message(STATUS "Skipping test_numa_memory.cpp (NUMA support not available)")
endif()

target_link_libraries(hpc_tests PRIVATE
    hpc_core
    GTest::gtest_main
)

include(GoogleTest)
if (TARGET hpc_tests)
    gtest_discover_tests(hpc_tests)
endif()
