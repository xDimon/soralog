#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if (DEFINED POLLY_FLAGS_SANITIZE_LEAK_CMAKE_)
    return()
else ()
    set(POLLY_FLAGS_SANITIZE_LEAK_CMAKE_ 1)
endif ()

set(FLAGS
    -fsanitize=leak
    -g
    )

foreach(FLAG IN LISTS FLAGS)
    add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
    add_cache_flag(CMAKE_C_FLAGS ${FLAG})
endforeach()

set(ENV{LSAN_OPTIONS} detect_leaks=1)

list(APPEND HUNTER_TOOLCHAIN_UNDETECTABLE_ID "sanitize-leak")
