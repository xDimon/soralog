#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(group_test
    group_test.cpp
    )
target_link_libraries(group_test
    group
    logging_system
    )

addtest(logger_test
    logger_test.cpp
    )
target_link_libraries(logger_test
    logger
    )

addtest(logging_system_test
    logging_system_test.cpp
    )
target_link_libraries(logging_system_test
    logging_system
    )

addtest(macros_test
    macros_test.cpp
    )
target_link_libraries(macros_test
    fmt::fmt
    )
