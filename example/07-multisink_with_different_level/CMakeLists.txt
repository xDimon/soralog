#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_executable(example_07
    main.cpp
)
target_include_directories(example_07
    PRIVATE ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(example_07
    soralog::soralog
    soralog::yaml
)
