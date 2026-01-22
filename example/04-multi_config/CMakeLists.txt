#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_executable(example_04
    main.cpp
)
target_include_directories(example_04
    PRIVATE ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(example_04
    soralog::soralog
    soralog::yaml
)
