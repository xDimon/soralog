#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

# CMake 4 refuses projects declaring compatibility with CMake below 3.5, and
# Hunter and some of its packages still do. The environment variable reaches
# every CMake that Hunter runs to build them.
if (CMAKE_VERSION VERSION_GREATER_EQUAL 4.0
    AND "$ENV{CMAKE_POLICY_VERSION_MINIMUM}" STREQUAL "")
    set(ENV{CMAKE_POLICY_VERSION_MINIMUM} 3.5)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/HunterGate.cmake)

HunterGate(
    URL  https://github.com/cpp-pm/hunter/archive/4474ffc59801c8910364a750883868374f901467.tar.gz
    SHA1 12d2565592e2c0bebe02dd469d47ba0354d4647c
    LOCAL
)
