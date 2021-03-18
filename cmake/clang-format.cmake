if (CLANG_FORMAT)

    # Adding clang-format target if executable is found
    if(NOT CLANG_FORMAT_BIN)
        find_program(CLANG_FORMAT_BIN NAMES clang-format clang-format-9 clang-format-8 clang-format-7)
        if(CLANG_FORMAT_BIN)
            message(STATUS "Binary clang-format has been found: ${CLANG_FORMAT_BIN}")
        endif()
    endif()

    if(CLANG_FORMAT_BIN)
        message(STATUS "Target clang-format enabled")

        file(GLOB_RECURSE
            ALL_CXX_SOURCE_FILES
            include/*.[ch]pp
            src/*.[ch]pp
            test/*.[ch]
            )

        add_custom_target(
            clang-format
            COMMAND "${CLANG_FORMAT_BIN}" -i ${ALL_CXX_SOURCE_FILES}
        )
    endif()

endif()
