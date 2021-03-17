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

setup_target_for_coverage_gcovr_xml(
    NAME ctest_coverage
    DEPENDENCIES ${tests}
    EXECUTABLE ctest
)

setup_target_for_coverage_gcovr_html(
    NAME ctest_coverage_html
    DEPENDENCIES ${tests}
    EXECUTABLE ctest
)
