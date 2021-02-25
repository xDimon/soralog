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
project(soralog VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TESTING "Build tests" ON)
option(EXAMPLES "Build examples" ON)


include(cmake/dependencies.cmake)


include_directories(
    SYSTEM
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:soralog>
)

add_subdirectory(src)

if(TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

if(EXAMPLES)
    add_subdirectory(example)
endif()
