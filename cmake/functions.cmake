function(print)
    message(STATUS "[${CMAKE_PROJECT_NAME}] ${ARGV}")
endfunction()

function(fatal_error)
    message(FATAL_ERROOR "[${CMAKE_PROJECT_NAME}] ${ARGV}")
endfunction()

function(add_cache_flag var_name flag)
    set(spaced_string " ${${var_name}} ")
    string(FIND "${spaced_string}" " ${flag} " flag_index)
    if(NOT flag_index EQUAL -1)
        return()
    endif()
    string(COMPARE EQUAL "" "${${var_name}}" is_empty)
    if(is_empty)
        # beautify: avoid extra space at the end if var_name is empty
        set("${var_name}" "${flag}" CACHE STRING "" FORCE)
    else()
        set("${var_name}" "${flag} ${${var_name}}" CACHE STRING "" FORCE)
    endif()
endfunction()

function(disable_clang_tidy target)
    set_target_properties(${target} PROPERTIES
        C_CLANG_TIDY ""
        CXX_CLANG_TIDY ""
        )
endfunction()

function(addtest test_name)
    add_executable(${test_name} ${ARGN})
    addtest_part(${test_name} ${ARGN})
    target_link_libraries(${test_name}
        GTest::main
        GMock::main
    )
    add_test(
        NAME ${test_name}
        COMMAND $<TARGET_FILE:${test_name}>
    )
    set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_bin
        ARCHIVE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/test_lib
        LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/test_lib
    )
    disable_clang_tidy(${test_name})
    set_property(GLOBAL APPEND PROPERTY TEST_TARGETS ${test_name})
endfunction()

function(addtest_part test_name)
    if (POLICY CMP0076)
        cmake_policy(SET CMP0076 NEW)
    endif ()
    target_sources(${test_name}
        PUBLIC ${ARGN}
    )
    target_link_libraries(${test_name}
        GTest::gtest
    )
endfunction()
