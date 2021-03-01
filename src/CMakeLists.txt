#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(sink_base
    sink.hpp
    )
target_link_libraries(sink_base
    fmt::fmt
    )

add_library(sink_to_console
    sink/sink_to_console.cpp
    )
target_link_libraries(sink_to_console
    sink_base
    pthread
    )

add_library(sink_to_file
    sink/sink_to_file.cpp
    )
target_link_libraries(sink_to_file
    sink_base
    pthread
    )

add_library(group
    group.cpp
    )
target_link_libraries(group
    sink_base
    )

add_library(logger
    logger.cpp
    )
target_link_libraries(logger
    sink_base
    )

add_library(configurator
    impl/configurator_from_yaml.cpp
    )
target_link_libraries(configurator
    yaml-cpp::yaml-cpp
    sink_to_console
    sink_to_file
    )

add_library(logger_system
    logger_system.cpp
    )
target_link_libraries(logger_system
    group
    logger
    )
