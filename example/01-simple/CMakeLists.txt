#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(example_simple
    main.cpp
    logging_object.cpp
    )
target_include_directories(example_simple
    PRIVATE ${CMAKE_SOURCE_DIR}/include
    )
target_link_libraries(example_simple
    soralog::soralog
    soralog::yaml
    soralog::fallback
    pthread
    )
