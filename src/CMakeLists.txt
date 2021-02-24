#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(logger_factory
    logger_factory.cpp
    )
target_link_libraries(logger_factory
    logger
    logger_system
    )

add_library(logger_system
    logger_system.cpp
    )
target_link_libraries(logger_system
    sink_to_console
    sink_to_file
    )

add_library(logger
    logger.cpp
    )
target_link_libraries(logger
    sink_base
    )

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

