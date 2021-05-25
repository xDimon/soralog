cmake_minimum_required(VERSION 3.12)

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

set(CMAKE_TOOLCHAIN_FILE
    "${CMAKE_SOURCE_DIR}/cmake/toolchain/cxx17.cmake"
    CACHE FILEPATH "Default toolchain"
    )

include("cmake/Hunter/init.cmake")

cmake_policy(SET CMP0048 NEW)
project(soralog VERSION 0.0.7 LANGUAGES CXX)

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

include(cmake/functions.cmake)
include(cmake/toolchain/compiler.cmake)
include(cmake/toolchain/cxx17.cmake)
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

