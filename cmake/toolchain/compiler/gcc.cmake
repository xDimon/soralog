#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if(DEFINED POLLY_COMPILER_GCC_CMAKE_)
    return()
else()
    set(POLLY_COMPILER_GCC_CMAKE_ 1)
endif()

find_program(CMAKE_C_COMPILER
    gcc
    gcc-13
    gcc-12
    gcc-11
    gcc-10
    gcc-9)
find_program(CMAKE_CXX_COMPILER
    g++
    g++-13
    g++-12
    g++-11
    g++-10
    g++-9)

if(NOT CMAKE_C_COMPILER)
    fatal_error("gcc not found")
endif()

if(NOT CMAKE_CXX_COMPILER)
    fatal_error("g++ not found")
endif()

set(
    CMAKE_C_COMPILER
    "${CMAKE_C_COMPILER}"
    CACHE STRING "C compiler"
    FORCE
)

set(
    CMAKE_CXX_COMPILER
    "${CMAKE_CXX_COMPILER}"
    CACHE STRING "C++ compiler"
    FORCE
)

string(REGEX MATCH "([0-9]+).([0-9]+).([0-9]+)" v ${CMAKE_CXX_COMPILER_VERSION})
if (${CMAKE_MATCH_1} LESS 9)
    fatal_error("Requires GNU compiler at least version 8")
endif()

if (${CMAKE_MATCH_1} GREATER_EQUAL 10)
#    print("GNU compiler version ${CMAKE_CXX_COMPILER_VERSION} - C++20 standard is supported")
    set(
        MAX_SUPPORTED_CXX_STANDARD 20
        CACHE STRING "Max supported C++ standard"
        FORCE
    )
endif()

if ((${CMAKE_MATCH_1} EQUAL 9) AND (${CMAKE_MATCH_2} LESS 2))
    add_cache_flag(CMAKE_EXE_LINKER_FLAGS "-lstdc++fs")
    add_cache_flag(CMAKE_SHARED_LINKER_FLAGS "-lstdc++fs")
endif()

