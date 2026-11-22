#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

## Template for add custom hunter config
#hunter_config(
#    package-name
#    VERSION 0.0.0-package-version
#    CMAKE_ARGS "CMAKE_VARIABLE=value"
#)

# fmt 10.1.1, the newest one in this Hunter release, does not compile with
# AppleClang 21 (format strings checked by consteval); take the release that
# vcpkg provides.
# No module library: fmt builds it by itself for C++20 with MSVC on Visual
# Studio, and then the Debug build of the package, installed after the Release
# one, replaced its export files: Release builds got fmtd.lib
# Position-independent code: the static libraries of the dependencies are
# linked into soralog's shared ones (BUILD_SHARED_LIBS=ON), which ld refuses
# on Linux otherwise
hunter_config(
    fmt
    URL  https://github.com/fmtlib/fmt/archive/refs/tags/12.2.1.tar.gz
    SHA1 6f55e53645fb4619c8e8d2b9fda533b4cca9e46e
    CMAKE_ARGS FMT_MODULE=OFF CMAKE_POSITION_INDEPENDENT_CODE=TRUE
)

hunter_config(
    yaml-cpp
    VERSION ${HUNTER_yaml-cpp_VERSION}
    CMAKE_ARGS CMAKE_POSITION_INDEPENDENT_CODE=TRUE
)
