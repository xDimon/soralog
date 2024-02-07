#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if(DEFINED POLLY_COMPILER_CLANG_CMAKE)
    return()
else()
    set(POLLY_COMPILER_CLANG_CMAKE 1)
endif()

if(XCODE_VERSION)
    set(_err "This toolchain is not available for Xcode")
    set(_err "${_err} because Xcode ignores CMAKE_C(XX)_COMPILER variable.")
    set(_err "${_err} Use xcode.cmake toolchain instead.")
    fatal_error(${_err})
endif()

find_program(CMAKE_C_COMPILER
    clang
    clang-17
    clang-16
    clang-15
    clang-14
    clang-13
    clang-12
    clang-11)
find_program(CMAKE_CXX_COMPILER
    clang++
    clang++-17
    clang++-16
    clang++-15
    clang++-14
    clang++-13
    clang++-12
    clang++-11)

if(NOT CMAKE_C_COMPILER)
    fatal_error("clang not found")
endif()

if(NOT CMAKE_CXX_COMPILER)
    fatal_error("clang++ not found")
endif()

set(
    CMAKE_C_COMPILER
    "${CMAKE_C_COMPILER}"
    CACHE
    STRING
    "C compiler"
    FORCE
)

set(
    CMAKE_CXX_COMPILER
    "${CMAKE_CXX_COMPILER}"
    CACHE
    STRING
    "C++ compiler"
    FORCE
)

string(REGEX MATCH "([0-9]+).([0-9]+).([0-9]+)" v ${CMAKE_CXX_COMPILER_VERSION})
if (${CMAKE_MATCH_1} LESS 15)
    print("Requires Clang compiler at least version 15")
endif()

if (${CMAKE_MATCH_1} GREATER_EQUAL 10)
#    print("GNU compiler version ${CMAKE_CXX_COMPILER_VERSION} - C++20 standard is supported")
    set(
        MAX_SUPPORTED_CXX_STANDARD 20
        CACHE STRING "Max supported C++ standard"
        FORCE
    )
endif()
