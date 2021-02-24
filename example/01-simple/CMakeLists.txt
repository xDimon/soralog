#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(xlog_example_simple
    main.cpp
    logging_object.cpp
    )
target_link_libraries(xlog_example_simple
    Boost::Boost.DI
    logger_factory

    )

