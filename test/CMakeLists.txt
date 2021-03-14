#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

include_directories(SYSTEM
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/test>
    )

add_subdirectory(unit)
