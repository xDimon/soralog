#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

function(reg_dependency name)
    if (${HUNTER_ENABLED})
        hunter_add_package(${name})
    endif ()
    find_package(${name} CONFIG REQUIRED)
endfunction()


reg_dependency(yaml-cpp)
if (NOT TARGET yaml-cpp::yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
endif()

reg_dependency(fmt)

if (TESTING OR COVERAGE)
    reg_dependency(GTest)
endif()
