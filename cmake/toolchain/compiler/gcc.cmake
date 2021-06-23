if(DEFINED POLLY_COMPILER_GCC_CMAKE_)
    return()
else()
    set(POLLY_COMPILER_GCC_CMAKE_ 1)
endif()

find_program(CMAKE_C_COMPILER gcc gcc-10 gcc-9 gcc-8)
find_program(CMAKE_CXX_COMPILER g++ g++-10 g++-9 g++-8)

if(NOT CMAKE_C_COMPILER)
    fatal_error("gcc not found")
endif()

if(NOT CMAKE_CXX_COMPILER)
    fatal_error("g++ not found")
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
if (${CMAKE_MATCH_1} LESS 8)
    message(FATAL "Requires GNU compiler at least version 8")
endif()
if ((${CMAKE_MATCH_1} LESS 9) OR ((${CMAKE_MATCH_1} EQUAL 9) AND (${CMAKE_MATCH_2} LESS 2)))
    add_cache_flag(CMAKE_EXE_LINKER_FLAGS "-lstdc++fs")
    add_cache_flag(CMAKE_SHARED_LINKER_FLAGS "-lstdc++fs")
endif()

