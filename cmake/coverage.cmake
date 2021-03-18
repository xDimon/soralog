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
        DEPENDENCIES ${tests}
        EXECUTABLE ${CTEST_BIN}
    )

    setup_target_for_coverage_gcovr_html(
        NAME coverage_html
        DEPENDENCIES ${tests}
        EXECUTABLE ${CTEST_BIN}
    )
endif()
