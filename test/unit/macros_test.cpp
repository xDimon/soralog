/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <fmt/format.h>

#include <soralog/macro.hpp>

using namespace soralog;
using namespace testing;

class MacrosTest : public ::testing::Test {
 public:
  struct FakeLogger {
    template <typename Format, typename... Args>
    void log(Level lvl, const Format &format, Args &&...args) {
      last_level = lvl;
      using OutputIt = decltype(message_buf.begin());
      size_t len =
          ::fmt::vformat_to_n<OutputIt>(
              message_buf.begin(), message_buf.size(),
              ::fmt::detail_exported::compile_string_to_view<char>(format),
              ::fmt::make_format_args(args...))
              .size;
      last_message = std::string_view(message_buf.data(),
                                      std::min(len, message_buf.size()));
    }
    static Level level() {
      return Level::TRACE;
    }
    Level last_level = Level::OFF;
    std::array<char, 100> message_buf;
    std::string_view last_message;
  };

  void SetUp() override {
    logger_ = std::make_shared<FakeLogger>();
  }
  void TearDown() override {}

  std::shared_ptr<FakeLogger> logger_;

  std::shared_ptr<FakeLogger> logger() const {
    return logger_;
  }
};

TEST_F(MacrosTest, NoArg) {
  SL_TRACE(logger(), "Trace: no arg");
  EXPECT_TRUE(logger_->last_level == Level::TRACE);
  EXPECT_TRUE(logger_->last_message == "Trace: no arg");

  SL_DEBUG(logger(), "Debug: no arg");
  EXPECT_TRUE(logger_->last_level == Level::DEBUG);
  EXPECT_TRUE(logger_->last_message == "Debug: no arg");

  SL_VERBOSE(logger(), "Verbose: no arg");
  EXPECT_TRUE(logger_->last_level == Level::VERBOSE);
  EXPECT_TRUE(logger_->last_message == "Verbose: no arg");

  SL_INFO(logger(), "Info: no arg");
  EXPECT_TRUE(logger_->last_level == Level::INFO);
  EXPECT_TRUE(logger_->last_message == "Info: no arg");

  SL_WARN(logger(), "Warning: no arg");
  EXPECT_TRUE(logger_->last_level == Level::WARN);
  EXPECT_TRUE(logger_->last_message == "Warning: no arg");

  SL_ERROR(logger(), "Error: no arg");
  EXPECT_TRUE(logger_->last_level == Level::ERROR);
  EXPECT_TRUE(logger_->last_message == "Error: no arg");

  SL_CRITICAL(logger(), "Critical: no arg");
  EXPECT_TRUE(logger_->last_level == Level::CRITICAL);
  EXPECT_TRUE(logger_->last_message == "Critical: no arg");
}

TEST_F(MacrosTest, OneArg) {
  SL_TRACE(logger(), "Trace: one arg: {}", "string");
  EXPECT_TRUE(logger_->last_level == Level::TRACE);
  EXPECT_TRUE(logger_->last_message == "Trace: one arg: string");

  SL_DEBUG(logger(), "Debug: one arg: {}", "string");
  EXPECT_TRUE(logger_->last_level == Level::DEBUG);
  EXPECT_TRUE(logger_->last_message == "Debug: one arg: string");

  SL_VERBOSE(logger(), "Verbose: one arg: {}", "string");
  EXPECT_TRUE(logger_->last_level == Level::VERBOSE);
  EXPECT_TRUE(logger_->last_message == "Verbose: one arg: string");

  SL_INFO(logger(), "Info: one arg: {}", "string");
  EXPECT_TRUE(logger_->last_level == Level::INFO);
  EXPECT_TRUE(logger_->last_message == "Info: one arg: string");

  SL_WARN(logger(), "Warning: one arg: {}", "string");
  EXPECT_TRUE(logger_->last_level == Level::WARN);
  EXPECT_TRUE(logger_->last_message == "Warning: one arg: string");

  SL_ERROR(logger(), "Error: one arg: {}", "string");
  EXPECT_TRUE(logger_->last_level == Level::ERROR);
  EXPECT_TRUE(logger_->last_message == "Error: one arg: string");

  SL_CRITICAL(logger(), "Critical: one arg: {}", "string");
  EXPECT_TRUE(logger_->last_level == Level::CRITICAL);
  EXPECT_TRUE(logger_->last_message == "Critical: one arg: string");
}

TEST_F(MacrosTest, TwoArg) {
  SL_TRACE(logger(), "Trace: two args: {} and {}", 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::TRACE);
  EXPECT_TRUE(logger_->last_message == "Trace: two args: 1 and 2.3");

  SL_DEBUG(logger(), "Debug: two args: {} and {}", 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::DEBUG);
  EXPECT_TRUE(logger_->last_message == "Debug: two args: 1 and 2.3");

  SL_VERBOSE(logger(), "Verbose: two args: {} and {}", 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::VERBOSE);
  EXPECT_TRUE(logger_->last_message == "Verbose: two args: 1 and 2.3");

  SL_INFO(logger(), "Info: two args: {} and {}", 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::INFO);
  EXPECT_TRUE(logger_->last_message == "Info: two args: 1 and 2.3");

  SL_WARN(logger(), "Warning: two args: {} and {}", 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::WARN);
  EXPECT_TRUE(logger_->last_message == "Warning: two args: 1 and 2.3");

  SL_ERROR(logger(), "Error: two args: {} and {}", 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::ERROR);
  EXPECT_TRUE(logger_->last_message == "Error: two args: 1 and 2.3");

  SL_CRITICAL(logger(), "Critical: two args: {} and {}", 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::CRITICAL);
  EXPECT_TRUE(logger_->last_message == "Critical: two args: 1 and 2.3");
}

TEST_F(MacrosTest, TwentyArg) {
  // clang-format off
  SL_TRACE(logger(), "Trace: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::TRACE);
  EXPECT_TRUE(logger_->last_message == "Trace: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");

  SL_DEBUG(logger(), "Debug: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::DEBUG);
  EXPECT_TRUE(logger_->last_message == "Debug: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");

  SL_VERBOSE(logger(), "Verbose: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
             1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::VERBOSE);
  EXPECT_TRUE(logger_->last_message == "Verbose: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");

  SL_INFO(logger(), "Info: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
          1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::INFO);
  EXPECT_TRUE(logger_->last_message == "Info: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");

  SL_WARN(logger(),"Warning: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
          1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::WARN);
  EXPECT_TRUE(logger_->last_message == "Warning: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");

  SL_ERROR(logger(), "Error: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::ERROR);
  EXPECT_TRUE(logger_->last_message == "Error: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");

  SL_CRITICAL(logger(), "Critical: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
              1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::CRITICAL);
  EXPECT_TRUE(logger_->last_message == "Critical: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");
  // clang-format on
}

TEST_F(MacrosTest, CustomLevel) {
  auto calculatedLevel = [](Level level) { return level; };

  SL_LOG(logger(), Level::TRACE, "Custom: trace");
  EXPECT_TRUE(logger_->last_level == Level::TRACE);
  EXPECT_TRUE(logger_->last_message == "Custom: trace");

  SL_LOG(logger(), Level::DEBUG, "Custom: {}", "debug");
  EXPECT_TRUE(logger_->last_level == Level::DEBUG);
  EXPECT_TRUE(logger_->last_message == "Custom: debug");

  SL_LOG(logger(), Level::VERBOSE, "Custom: {} is {}", "level", "verbose");
  EXPECT_TRUE(logger_->last_level == Level::VERBOSE);
  EXPECT_TRUE(logger_->last_message == "Custom: level is verbose");

  SL_LOG(logger(), calculatedLevel(Level::INFO), "Custom: {}", "info");
  EXPECT_TRUE(logger_->last_level == Level::INFO);
  EXPECT_TRUE(logger_->last_message == "Custom: info");

  SL_LOG(logger(), calculatedLevel(Level::WARN), "Custom: {}", "warning");
  EXPECT_TRUE(logger_->last_level == Level::WARN);
  EXPECT_TRUE(logger_->last_message == "Custom: warning");

  SL_LOG(logger(), calculatedLevel(Level::ERROR), "Custom: {}", "error");
  EXPECT_TRUE(logger_->last_level == Level::ERROR);
  EXPECT_TRUE(logger_->last_message == "Custom: error");

  // clang-format off
  SL_LOG(logger(), calculatedLevel(Level::CRITICAL),
         "Custom: critical; {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
         1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::CRITICAL);
  EXPECT_TRUE(logger_->last_message == "Custom: critical; 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");
  // clang-format on
}

TEST_F(MacrosTest, CalculatedFormat) {
  // String literal
  SL_DEBUG(logger(), "ping => {}", "pong");
  EXPECT_TRUE(logger_->last_message == "ping => pong");

  // C-string
  static const char *c_string = "ping => {}";
  SL_DEBUG_DF(logger(), c_string, "pong");
  EXPECT_TRUE(logger_->last_message == "ping => pong");

  std::string_view string_view("ping => {}");
  SL_DEBUG_DF(logger(), string_view, "pong");
  EXPECT_TRUE(logger_->last_message == "ping => pong");

  std::string string("ping => {}");
  SL_DEBUG_DF(logger(), string, "pong");
  EXPECT_TRUE(logger_->last_message == "ping => pong");
}

TEST_F(MacrosTest, CalculatedArgs) {
  // clang-format off
  auto number = [](int i) { return "#" + std::to_string(i); };
  auto length = [](std::string s) { return std::to_string(s.length()); };

  SL_DEBUG(logger(), "Numbers: {}, {}, {}", number(1), number(2), number(3));
  EXPECT_TRUE(logger_->last_message == "Numbers: #1, #2, #3");

  SL_DEBUG(logger(), "Lengths: {}, {}, {}", length("*"), length("**"), length("***"));
  EXPECT_TRUE(logger_->last_message == "Lengths: 1, 2, 3");
  // clang-format on
}

TEST_F(MacrosTest, StructuredBinding) {
  struct {
    int x = 1;
  } a;
  auto &[x] = a;

  SL_DEBUG(logger(), "x: {}", x);
  EXPECT_TRUE(logger_->last_message == "x: 1");

  static const char *df = "x: {}";
  SL_DEBUG_DF(logger(), df, x);
  EXPECT_TRUE(logger_->last_message == "x: 1");
}
