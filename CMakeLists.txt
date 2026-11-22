#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.12)

option(BUILD_TESTS  "Build tests"                                 OFF)
option(BUILD_BENCHMARKS "Build benchmarks"                        OFF)
option(BUILD_BENCHMARK_COMPARE "Build comparison with other loggers" OFF)
option(EXAMPLES     "Build examples"                              OFF)
option(CLANG_FORMAT "Enable clang-format target"                  OFF)
option(CLANG_TIDY   "Enable clang-tidy checks during compilation" OFF)
option(COVERAGE     "Enable generation of coverage info"          OFF)

# Sanitizers enables only for this project, and will be disabled for dependencies
option(ASAN         "Enable address sanitizer"                    OFF)
option(LSAN         "Enable leak sanitizer"                       OFF)
option(MSAN         "Enable memory sanitizer"                     OFF)
option(TSAN         "Enable thread sanitizer"                     OFF)
option(UBSAN        "Enable UB sanitizer"                         OFF)

if (COVERAGE)
    set(BUILD_TESTS ON) # tests are needed to generate coverage info
endif ()

if (PACKAGE_MANAGER)
    # 'none': dependencies are found by find_package as they are, e.g. provided
    # by CMAKE_PREFIX_PATH or another package manager
    if(NOT PACKAGE_MANAGER MATCHES "^(hunter|vcpkg|none)$")
        message(FATAL_ERROR "PACKAGE_MANAGER must be set to 'hunter', 'vcpkg', 'none' or isn't set")
    endif ()
else ()
    set(PACKAGE_MANAGER "hunter")
    if (CMAKE_TOOLCHAIN_FILE)
        get_filename_component(ACTUAL_NAME ${CMAKE_TOOLCHAIN_FILE} NAME)
        if(ACTUAL_NAME STREQUAL "vcpkg.cmake")
            message(STATUS "vcpkg will be used because vcpkg.cmake has found")
            set(PACKAGE_MANAGER "vcpkg")
        endif ()
    endif ()
endif ()
message(STATUS "Selected package manager: ${PACKAGE_MANAGER}")

if (PACKAGE_MANAGER STREQUAL "hunter")
    include("cmake/Hunter/init.cmake")
endif ()

if(BUILD_TESTS)
    if (PACKAGE_MANAGER STREQUAL "vcpkg")
        list(APPEND VCPKG_MANIFEST_FEATURES soralog-tests)
    endif()
endif()

if(BUILD_BENCHMARK_COMPARE)
    if (NOT PACKAGE_MANAGER STREQUAL "vcpkg")
        message(FATAL_ERROR "BUILD_BENCHMARK_COMPARE needs vcpkg: other loggers come from its manifest feature")
    endif()
    set(BUILD_BENCHMARKS ON)
    list(APPEND VCPKG_MANIFEST_FEATURES soralog-benchmark-compare)
endif()

cmake_policy(SET CMP0048 NEW)

project(soralog VERSION 0.3.2 LANGUAGES CXX)

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

include(cmake/functions.cmake)

# The syslog sink needs <syslog.h>, which Windows doesn't have
if (WIN32)
    set(SYSLOG_SINK_DEFAULT OFF)
else ()
    set(SYSLOG_SINK_DEFAULT ON)
endif ()
option(SYSLOG_SINK "Build the syslog sink" ${SYSLOG_SINK_DEFAULT})
if (WIN32 AND SYSLOG_SINK)
    fatal_error("SYSLOG_SINK is not supported on Windows")
endif ()

# The Event Log sink is the Windows counterpart of the syslog sink
if (WIN32)
    set(EVENTLOG_SINK_DEFAULT ON)
else ()
    set(EVENTLOG_SINK_DEFAULT OFF)
endif ()
option(EVENTLOG_SINK "Build the Windows Event Log sink" ${EVENTLOG_SINK_DEFAULT})
if (NOT WIN32 AND EVENTLOG_SINK)
    fatal_error("EVENTLOG_SINK is supported on Windows only")
endif ()

if (WIN32)
    # DLLs export all their functions: the sources have no export macros, and
    # public headers access no data of the libraries
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    # Windows looks for DLLs next to the executable: put DLLs, tests and
    # examples into one directory, so that they run from the build tree
    if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    endif ()
endif ()

include(cmake/toolchain/compiler.cmake)

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE OR NOT DEFINED CMAKE_CXX_STANDARD)
    if (MAX_SUPPORTED_CXX_STANDARD GREATER_EQUAL 20)
        set(CXXSTD 20)
    else ()
        set(CXXSTD 17)
    endif ()
    include("${CMAKE_SOURCE_DIR}/cmake/toolchain/cxx${CXXSTD}.cmake")
    print("Using C++${CXXSTD} standard")
    if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/toolchain/cxx${CXXSTD}.cmake")
    endif ()
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# the property is out of "if TESTING" scope due to addtest func is out too
set_property(GLOBAL PROPERTY TEST_TARGETS)

include(cmake/dependencies.cmake)
include(cmake/sanitizers.cmake)

if(CLANG_TIDY)
    # Must be included before creating any target
    include(cmake/clang-tidy.cmake)
endif()

if(CLANG_FORMAT)
    include(cmake/clang-format.cmake)
endif()

add_subdirectory(src)

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(EXAMPLES)
    add_subdirectory(example)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()

if (COVERAGE)
    include(cmake/coverage.cmake)
endif ()

