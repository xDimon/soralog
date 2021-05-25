#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(libs4test INTERFACE EXCLUDE_FROM_ALL)
target_link_libraries(libs4test INTERFACE
    logging_system
    sink_to_nowhere
    group
    logger
    )

addtest(group_test
    group_test.cpp
    )
target_link_libraries(group_test
    libs4test
    )

addtest(logger_test
    logger_test.cpp
    )
target_link_libraries(logger_test
    libs4test
    )

addtest(logging_system_test
    logging_system_test.cpp
    )
target_link_libraries(logging_system_test
    libs4test
    )

addtest(sink_to_console_test
    sink_to_console_test.cpp
    )
target_link_libraries(sink_to_console_test
    sink_to_console
    )

addtest(sink_to_file_test
    sink_to_file_test.cpp
    )
target_link_libraries(sink_to_file_test
    sink_to_file
    )

addtest(macros_test
    macros_test.cpp
    )
target_link_libraries(macros_test
    fmt::fmt
    )
