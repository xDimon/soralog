#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

if (TESTING)
  hunter_add_package(GTest)
  find_package(GTest CONFIG REQUIRED)
  find_package(GMock CONFIG REQUIRED)
endif()

hunter_add_package(yaml-cpp)
find_package(yaml-cpp CONFIG REQUIRED)

hunter_add_package(fmt)
find_package(fmt CONFIG REQUIRED)
