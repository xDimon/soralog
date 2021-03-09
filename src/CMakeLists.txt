#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
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

add_library(sink INTERFACE)
target_link_libraries(sink INTERFACE
    fmt::fmt
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
    pthread
    )

add_library(sink_to_file
    impl/sink_to_file.cpp
    )
target_link_libraries(sink_to_file
    sink
    pthread
    )

add_library(group
    group.cpp
    )
target_link_libraries(group
    sink
    )

add_library(logger
    logger.cpp
    )
target_link_libraries(logger
    sink
    logger_system
    )

add_library(configurator INTERFACE)
target_link_libraries(configurator INTERFACE
    sink_to_nowhere
    sink_to_console
    sink_to_file
    )

add_library(configurator_yaml
    impl/configurator_from_yaml.cpp
    )
target_link_libraries(configurator_yaml
    yaml-cpp::yaml-cpp
    configurator
    )

add_library(fallback_configurator
    impl/fallback_configurator.cpp
    )
target_link_libraries(fallback_configurator
    configurator
    )

add_library(logger_system
    logger_system.cpp
    )
target_link_libraries(logger_system
    group
    logger
    )

add_library(soralog soralog.cpp)
target_link_libraries(soralog
    logger_system
    sink
    group
    logger
    )

add_library(yaml ALIAS configurator_yaml)
add_library(fallback ALIAS fallback_configurator)

add_library(soralog::soralog ALIAS soralog)
add_library(soralog::yaml ALIAS configurator_yaml)
add_library(soralog::fallback ALIAS fallback_configurator)

set(INSTALL_TARGETS
    soralog

    logger_system
    group
    logger

    sink
    sink_to_nowhere
    sink_to_console
    sink_to_file

    configurator
    fallback_configurator
    configurator_yaml
    )

include(GNUInstallDirs)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/soralog
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    TARGETS ${INSTALL_TARGETS}
    EXPORT soralogConfig
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
    EXPORT soralogConfig
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/soralog
    NAMESPACE soralog::
)

export(
    EXPORT soralogConfig
    NAMESPACE soralog::
    FILE ${PROJECT_BINARY_DIR}/soralogConfig.cmake)
