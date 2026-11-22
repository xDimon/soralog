/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

// Before soralog headers, on purpose: <windows.h> defines macros ERROR and
// IGNORE; elsewhere they are defined the same way, to check the headers on
// every platform
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#define ERROR 0
#define IGNORE 0
#endif

#include <soralog/impl/fallback_configurator.hpp>
#include <soralog/impl/multisink.hpp>
#include <soralog/impl/sink_to_console.hpp>
#include <soralog/impl/sink_to_file.hpp>
#include <soralog/impl/sink_to_nowhere.hpp>
#include <soralog/logging_system.hpp>
#include <soralog/macro.hpp>
#include <soralog/soralog.hpp>

#include <array>
#include <string>
#include <string_view>

// The macros of the user are restored after soralog headers
static_assert(ERROR == 0);
static_assert(IGNORE == 0);

namespace {
  /// @brief Logger for macros, remembering the last level and message
  struct FakeLogger {
    template <typename Format, typename... Args>
    void log(soralog::Level lvl, const Format &format, Args &&...args) {
      last_level = lvl;
      last_message =
          ::fmt::vformat(soralog::detail::compile_string_to_view(format),
                         ::fmt::make_format_args(args...));
    }

    static soralog::Level level() {
      return soralog::Level::TRACE;
    }

    soralog::Level last_level = soralog::Level::OFF;
    std::string last_message;
  };
}  // namespace

/**
 * @test ErrorMacro
 * @brief Tests SL_ERROR in code where ERROR is a macro.
 *
 * @given A logger, ERROR and IGNORE defined as macros before soralog headers.
 * @when SL_ERROR logs a message.
 * @then The message is logged with the error level.
 */
TEST(WindowsMacrosTest, ErrorMacro) {
  auto logger = std::make_shared<FakeLogger>();
  SL_ERROR(logger, "error #{}", 1);
  EXPECT_EQ(logger->last_level, soralog::detail::level_error);
  EXPECT_STREQ(soralog::levelToStr(logger->last_level), "Error");
  EXPECT_EQ(logger->last_message, "error #1");
}
