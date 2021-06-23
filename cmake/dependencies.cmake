#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

if (TESTING OR COVERAGE)
  hunter_add_package(GTest)
  find_package(GTest CONFIG REQUIRED)
  find_package(GMock CONFIG REQUIRED)
endif()

hunter_add_package(yaml-cpp)
find_package(yaml-cpp CONFIG REQUIRED)
if (NOT TARGET yaml-cpp::yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
endif()

hunter_add_package(fmt)
find_package(fmt CONFIG REQUIRED)
