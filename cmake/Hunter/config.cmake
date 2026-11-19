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
# vcpkg provides
hunter_config(
    fmt
    URL  https://github.com/fmtlib/fmt/archive/refs/tags/12.2.1.tar.gz
    SHA1 6f55e53645fb4619c8e8d2b9fda533b4cca9e46e
)
