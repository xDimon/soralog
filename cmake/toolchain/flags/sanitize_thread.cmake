#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if (DEFINED POLLY_FLAGS_SANITIZE_THREAD_CMAKE_)
    return()
else ()
    set(POLLY_FLAGS_SANITIZE_THREAD_CMAKE_ 1)
endif ()

set(FLAGS
    -fsanitize=thread
    -g
    )

foreach(FLAG IN LISTS FLAGS)
    add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
    add_cache_flag(CMAKE_C_FLAGS ${FLAG})
endforeach()

add_cache_flag(CMAKE_EXE_LINKER_FLAGS "-fsanitize=thread")
add_cache_flag(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=thread")
