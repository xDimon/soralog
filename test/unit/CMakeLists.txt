#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(libs4test INTERFACE)
target_link_libraries(libs4test INTERFACE
    logging_system
    sink_to_nowhere
    group
    logger
    )

addtest(circular_buffer_test
    circular_buffer_test.cpp
    )
target_link_libraries(circular_buffer_test
    libs4test
    )

addtest(file_writer_test
    file_writer_test.cpp
    )
target_link_libraries(file_writer_test
    sink_to_file
    )

addtest(group_test
    group_test.cpp
    )
target_link_libraries(group_test
    libs4test
    )

addtest(configurator_from_yaml_test
    configurator_from_yaml_test.cpp
    )
target_link_libraries(configurator_from_yaml_test
    configurator_yaml
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

if (SYSLOG_SINK)
    addtest(sink_to_syslog_test
        sink_to_syslog_test.cpp
        )
    target_link_libraries(sink_to_syslog_test
        sink_to_syslog
        )
endif ()

if (EVENTLOG_SINK)
    addtest(sink_to_eventlog_test
        sink_to_eventlog_test.cpp
        )
    target_link_libraries(sink_to_eventlog_test
        sink_to_eventlog
        configurator_yaml
        )
endif ()

addtest(macros_test
    macros_test.cpp
    )
target_link_libraries(macros_test
    fmt::fmt
    )

addtest(windows_macros_test
    windows_macros_test.cpp
    )
target_link_libraries(windows_macros_test
    soralog
    fallback_configurator
    )
