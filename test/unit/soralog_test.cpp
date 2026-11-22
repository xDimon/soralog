/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <soralog/soralog.hpp>

// Links to the soralog library itself: on Windows it checks that soralog.dll
// exports a symbol
TEST(SoralogTest, LibraryName) {
  EXPECT_STREQ(soralog::libraryName(), "soralog");
}
