#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if (CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang)$")
    print("Using Clang")
    include(${CMAKE_CURRENT_LIST_DIR}/compiler/clang.cmake)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    print("Using GCC")
    include(${CMAKE_CURRENT_LIST_DIR}/compiler/gcc.cmake)
else()
    fatal_error("Compiler '${CMAKE_CXX_COMPILER_ID}' is not supported")
endif()
