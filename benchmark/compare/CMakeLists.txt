#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

# Comparison of soralog with other loggers: one executable per logger,
# same load generator and options; see docs/benchmark.

find_package(spdlog CONFIG REQUIRED)
find_package(quill CONFIG REQUIRED)

add_executable(compare_soralog compare_soralog.cpp)
target_link_libraries(compare_soralog soralog)
target_compile_definitions(compare_soralog PRIVATE
    SORALOG_VERSION="${PROJECT_VERSION}")

add_executable(compare_spdlog compare_spdlog.cpp)
target_link_libraries(compare_spdlog spdlog::spdlog)

add_executable(compare_quill compare_quill.cpp)
target_link_libraries(compare_quill quill::quill)
