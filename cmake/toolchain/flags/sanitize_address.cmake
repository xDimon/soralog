#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if (DEFINED POLLY_FLAGS_SANITIZE_ADDRESS_CMAKE_)
    return()
else ()
    set(POLLY_FLAGS_SANITIZE_ADDRESS_CMAKE_ 1)
endif ()

set(FLAGS
    -fsanitize=address
    -fsanitize-address-use-after-scope
    -g
    -O1
    -DNDEBUG
    )

foreach(FLAG IN LISTS FLAGS)
    add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
    add_cache_flag(CMAKE_C_FLAGS ${FLAG})
endforeach()

add_cache_flag(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")
add_cache_flag(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=address")

set(ENV{ASAN_OPTIONS} verbosity=1:debug=1:detect_leaks=1:check_initialization_order=1:alloc_dealloc_mismatch=true:use_odr_indicator=true)
