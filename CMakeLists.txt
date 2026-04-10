cmake_minimum_required(VERSION 3.20)
project(cpp-hpc-primitives VERSION 0.1.0 LANGUAGES CXX)

option(HPC_ENABLE_TESTS "Build unit tests" ON)
option(HPC_ENABLE_BENCHMARKS "Build benchmarks" ON)
option(HPC_ENABLE_SANITIZERS "Enable Address/Undefined sanitizers" OFF)
option(HPC_ENABLE_EXAMPLES "Build examples" ON)
option(HPC_ENABLE_NUMA "Enable NUMA (libnuma) support when available" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerOptions.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake)

add_library(hpc_core
    src/arena_allocator.cpp
    src/pool_allocator.cpp
    src/support/clock.cpp
    src/support/cpu_topology.cpp
    src/huge_pages.cpp
)

# POSIX shared-memory IPC is not available on Windows.
if(NOT WIN32)
    target_sources(hpc_core PRIVATE src/ipc/shm_ring_buffer.cpp)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(hpc_core PRIVATE rt)
    endif()
endif()

# Optional NUMA support (Linux libnuma).
if(HPC_ENABLE_NUMA)
    include(CheckIncludeFileCXX)
    include(CheckLibraryExists)

    check_include_file_cxx("numa.h" HPC_HAS_NUMA_H)
    check_library_exists(numa numa_available "" HPC_HAS_LIBNUMA)

    if(HPC_HAS_NUMA_H AND HPC_HAS_LIBNUMA)
        target_sources(hpc_core PRIVATE src/numa_arena.cpp)
        target_link_libraries(hpc_core PRIVATE numa)
        target_compile_definitions(hpc_core PUBLIC HPC_HAS_NUMA=1)
    else()
        message(STATUS "libnuma not found (numa.h and/or -lnuma missing). Building without NUMA support.")
        target_compile_definitions(hpc_core PUBLIC HPC_HAS_NUMA=0)
    endif()
else()
    target_compile_definitions(hpc_core PUBLIC HPC_HAS_NUMA=0)
endif()

hpc_enable_strict_warnings(hpc_core)

target_include_directories(hpc_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(hpc_core PROPERTIES
    OUTPUT_NAME "hpc_core"
    POSITION_INDEPENDENT_CODE ON
)

if(HPC_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(HPC_ENABLE_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(HPC_ENABLE_EXAMPLES AND NOT WIN32)
    add_executable(hpc_shm_publisher examples/shm_publisher.cpp)
    target_link_libraries(hpc_shm_publisher PRIVATE hpc_core)
endif()
