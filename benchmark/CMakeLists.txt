#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

include_directories(SYSTEM
    ${CMAKE_SOURCE_DIR}/include
    )

add_executable(soralog_benchmark
    soralog_benchmark.cpp
    )
target_link_libraries(soralog_benchmark
    sink_to_file
    )

if(BUILD_BENCHMARK_COMPARE)
    add_subdirectory(compare)
endif()
