#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if(NOT CLANG_TIDY_BIN)
    find_program(CLANG_TIDY_BIN
        NAMES clang-tidy
              clang-tidy-12
              clang-tidy-11
              clang-tidy-10
              clang-tidy-9
              clang-tidy-8
              clang-tidy-7
        DOC "Path to clang-tidy executable"
    )
endif()

if(NOT CLANG_TIDY_BIN)
    message(FATAL_ERROR "Binary clang-tidy is not installed. Aborting...")
else()
    message(STATUS "Binary clang-tidy has been found: ${CLANG_TIDY_BIN}")
endif()

set(CMAKE_C_CLANG_TIDY   ${CLANG_TIDY_BIN})
set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_BIN})
