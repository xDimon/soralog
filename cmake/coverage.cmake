#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

set(CMAKE_BUILD_TYPE Debug)

include(cmake/CodeCoverage/CodeCoverage.cmake)

append_coverage_compiler_flags()

set(COVERAGE_EXCLUDES
    '${CMAKE_SOURCE_DIR}/build/*'
    '${CMAKE_SOURCE_DIR}/cmake-build-*/*'
    '${CMAKE_SOURCE_DIR}/example/*'
    '${CMAKE_SOURCE_DIR}/test/*'
    )

get_property(tests GLOBAL PROPERTY TEST_TARGETS)

if(NOT CTEST_BIN)
    find_program(CTEST_BIN NAMES ctest)
    if(CTEST_BIN)
        message(STATUS "Binary ctest has been found: ${CTEST_BIN}")
    endif()
endif()

if(CTEST_BIN)
    setup_target_for_coverage_gcovr_xml(
        NAME coverage
        EXECUTABLE ${CTEST_BIN} -j ${PROCESSOR_COUNT}
        DEPENDENCIES all_tests
        EXCLUDE ${COVERAGE_EXCLUDES}
    )

    setup_target_for_coverage_gcovr_html(
        NAME coverage_html
        EXECUTABLE ${CTEST_BIN} -j ${PROCESSOR_COUNT}
        DEPENDENCIES all_tests
        EXCLUDE ${COVERAGE_EXCLUDES}
    )
endif()
