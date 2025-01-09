#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.12)

if (PACKAGE_MANAGER)
    if(PACKAGE_MANAGER NOT MATCHES "^(hunter|vcpkg)$")
        message(FATAL_ERROR "PACKAGE_MANAGER must be set to 'hunter', 'vcpkg' or isn't set")
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

cmake_policy(SET CMP0048 NEW)
project(soralog VERSION 0.2.5 LANGUAGES CXX)

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

include(cmake/functions.cmake)

include(cmake/toolchain/compiler.cmake)

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if (MAX_SUPPORTED_CXX_STANDARD GREATER_EQUAL 20)
        set(CMAKE_TOOLCHAIN_FILE
            "${CMAKE_SOURCE_DIR}/cmake/toolchain/cxx20.cmake"
            CACHE FILEPATH "Default toolchain"
        )
    else ()
        set(CMAKE_TOOLCHAIN_FILE
            "${CMAKE_SOURCE_DIR}/cmake/toolchain/cxx17.cmake"
            CACHE FILEPATH "Default toolchain"
        )
    endif ()
    print("Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
    include(${CMAKE_TOOLCHAIN_FILE})
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TESTING      "Build tests"                                 ON)
option(EXAMPLES     "Build examples"                              ON)
option(CLANG_FORMAT "Enable clang-format target"                  OFF)
option(CLANG_TIDY   "Enable clang-tidy checks during compilation" OFF)
option(COVERAGE     "Enable generation of coverage info"          OFF)

# Sanitizers enables only for this project, and will be disabled for dependencies
option(ASAN         "Enable address sanitizer"                    OFF)
option(LSAN         "Enable leak sanitizer"                       OFF)
option(MSAN         "Enable memory sanitizer"                     OFF)
option(TSAN         "Enable thread sanitizer"                     OFF)
option(UBSAN        "Enable UB sanitizer"                         OFF)

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

if(TESTING OR COVERAGE)
    enable_testing()
    add_subdirectory(test)
endif()

if(EXAMPLES)
    add_subdirectory(example)
endif()

if (COVERAGE)
    include(cmake/coverage.cmake)
endif ()

