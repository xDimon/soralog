#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if(DEFINED POLLY_COMPILER_MSVC_CMAKE_)
    return()
else()
    set(POLLY_COMPILER_MSVC_CMAKE_ 1)
endif()

# std::ranges, which soralog uses, has come with Visual Studio 2019 16.10
if (MSVC_VERSION LESS 1929)
    print("Requires MSVC compiler at least version 19.29")
endif()

# Without it __cplusplus stays 199711L whatever the standard is
add_compile_options(/Zc:__cplusplus)

set(
    MAX_SUPPORTED_CXX_STANDARD 20
    CACHE STRING "Max supported C++ standard"
    FORCE
)
