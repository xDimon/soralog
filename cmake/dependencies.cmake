#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

function(reg_dependency name)
    if (PACKAGE_MANAGER STREQUAL "hunter")
        hunter_add_package(${name})
    endif ()
    find_package(${name} CONFIG REQUIRED)
endfunction()


reg_dependency(yaml-cpp)

reg_dependency(fmt)

if (BUILD_TESTS)
    reg_dependency(GTest)
endif()
