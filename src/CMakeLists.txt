#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

include_directories(SYSTEM
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    )

set(INSTALL_TARGETS)
function (add_for_install target)
    list(APPEND INSTALL_TARGETS ${target})
    set(INSTALL_TARGETS ${INSTALL_TARGETS} PARENT_SCOPE)
endfunction()

# Not header-only: thread utilities keep per-process state, which must be in
# one place, also when the libraries are DLLs
add_library(sink
    util.cpp
    )
target_link_libraries(sink
    PUBLIC
        fmt::fmt
    PRIVATE
        Threads::Threads
    )

add_library(sink_to_nowhere
    impl/sink_to_nowhere.cpp
    )
target_link_libraries(sink_to_nowhere
    sink
    )

add_library(sink_to_console
    impl/sink_to_console.cpp
    )
target_link_libraries(sink_to_console
    sink
    Threads::Threads
    )

add_library(sink_to_file
    impl/file_writer.cpp
    impl/sink_to_file.cpp
    )
target_link_libraries(sink_to_file
    sink
    Threads::Threads
    )

if (SYSLOG_SINK)
    add_library(sink_to_syslog
        impl/sink_to_syslog.cpp
        )
    target_link_libraries(sink_to_syslog
        sink
        Threads::Threads
        )
endif ()

add_library(multisink
    impl/multisink.cpp
    )
target_link_libraries(multisink
    sink
    )

# Groups, loggers and the logging system refer to each other, so they are
# built as one library; the targets are kept for those who link to them by name
add_library(group INTERFACE)
target_link_libraries(group INTERFACE
    logging_system
    )

add_library(logger INTERFACE)
target_link_libraries(logger INTERFACE
    logging_system
    )

add_library(configurator INTERFACE)
target_link_libraries(configurator INTERFACE
    sink_to_nowhere
    sink_to_console
    sink_to_file
    multisink
    )
if (SYSLOG_SINK)
    target_link_libraries(configurator INTERFACE
        sink_to_syslog
        )
endif ()

add_library(configurator_yaml
    impl/configurator_from_yaml.cpp
    )
target_link_libraries(configurator_yaml
    ${SORALOG_YAML_CPP_TARGET}
    configurator
    logging_system
    )
if (SYSLOG_SINK)
    target_compile_definitions(configurator_yaml PRIVATE
        SORALOG_SYSLOG_SINK
        )
endif ()

add_library(fallback_configurator
    impl/fallback_configurator.cpp
    )
target_link_libraries(fallback_configurator
    configurator
    logging_system
    )

add_library(logging_system
    logging_system.cpp
    group.cpp
    logger.cpp
    )
target_link_libraries(logging_system
    sink
    sink_to_nowhere
    )

add_library(soralog soralog.cpp)
target_link_libraries(soralog
    logging_system
    sink
    group
    configurator
    logger
    )

add_library(yaml ALIAS configurator_yaml)
add_library(fallback ALIAS fallback_configurator)

add_library(soralog::soralog ALIAS soralog)
add_library(soralog::yaml ALIAS configurator_yaml)
add_library(soralog::fallback ALIAS fallback_configurator)

set(INSTALL_TARGETS
    sink
    sink_to_nowhere
    sink_to_console
    sink_to_file
    multisink

    group

    configurator
    fallback_configurator
    configurator_yaml

    logger
    logging_system

    soralog
    )
if (SYSLOG_SINK)
    list(APPEND INSTALL_TARGETS sink_to_syslog)
    set(SORALOG_HEADERS_EXCLUDE)
else ()
    set(SORALOG_HEADERS_EXCLUDE PATTERN sink_to_syslog.hpp EXCLUDE)
endif ()

include(GNUInstallDirs)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/soralog
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    ${SORALOG_HEADERS_EXCLUDE}
)

install(
    TARGETS ${INSTALL_TARGETS}
    EXPORT soralogTargets
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FRAMEWORK     DESTINATION ${CMAKE_INSTALL_PREFIX}
)

set(SORALOG_CONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/soralog)

install(
    EXPORT soralogTargets
    DESTINATION ${SORALOG_CONFIG_DIR}
    NAMESPACE soralog::
)

export(
    EXPORT soralogTargets
    FILE ${PROJECT_BINARY_DIR}/soralogTargets.cmake
    NAMESPACE soralog::
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/soralogConfig.cmake.in
    ${PROJECT_BINARY_DIR}/soralogConfig.cmake
    INSTALL_DESTINATION ${SORALOG_CONFIG_DIR}
)

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/soralogConfigVersion.cmake
    COMPATIBILITY SameMinorVersion
)

install(
    FILES
        ${PROJECT_BINARY_DIR}/soralogConfig.cmake
        ${PROJECT_BINARY_DIR}/soralogConfigVersion.cmake
    DESTINATION ${SORALOG_CONFIG_DIR}
)
