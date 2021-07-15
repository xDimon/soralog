/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <soralog/macro.hpp>

using namespace soralog;
using namespace testing;

class MacrosTest : public ::testing::Test {
 public:
  struct FakeLogger {
    template <typename... Args>
    void log(Level lvl, std::string_view format, Args &&... args) {
      last_level = lvl;
      last_message = fmt::format(format, args...);
    }
    static Level level() {
      return Level::TRACE;
    }
    Level last_level = Level::OFF;
    std::string last_message{};
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
  std::string fmt = "Trace: no arg";
  SL_TRACE(logger(), fmt);
  EXPECT_TRUE(logger_->last_level == Level::TRACE);
  EXPECT_TRUE(logger_->last_message == fmt);

  fmt = "Debug: no arg";
  SL_DEBUG(logger(), fmt);
  EXPECT_TRUE(logger_->last_level == Level::DEBUG);
  EXPECT_TRUE(logger_->last_message == fmt);

  fmt = "Verbose: no arg";
  SL_VERBOSE(logger(), fmt);
  EXPECT_TRUE(logger_->last_level == Level::VERBOSE);
  EXPECT_TRUE(logger_->last_message == fmt);

  fmt = "Info: no arg";
  SL_INFO(logger(), fmt);
  EXPECT_TRUE(logger_->last_level == Level::INFO);
  EXPECT_TRUE(logger_->last_message == fmt);

  fmt = "Warning: no arg";
  SL_WARN(logger(), fmt);
  EXPECT_TRUE(logger_->last_level == Level::WARN);
  EXPECT_TRUE(logger_->last_message == fmt);

  fmt = "Error: no arg";
  SL_ERROR(logger(), fmt);
  EXPECT_TRUE(logger_->last_level == Level::ERROR);
  EXPECT_TRUE(logger_->last_message == fmt);

  fmt = "Critical: no arg";
  SL_CRITICAL(logger(), fmt);
  EXPECT_TRUE(logger_->last_level == Level::CRITICAL);
  EXPECT_TRUE(logger_->last_message == fmt);
}

TEST_F(MacrosTest, OneArg) {
  std::string fmt = "Trace: one arg: {}";
  SL_TRACE(logger(), fmt, "string");
  EXPECT_TRUE(logger_->last_level == Level::TRACE);
  EXPECT_TRUE(logger_->last_message == "Trace: one arg: string");

  fmt = "Debug: one arg: {}";
  SL_DEBUG(logger(), fmt, "string");
  EXPECT_TRUE(logger_->last_level == Level::DEBUG);
  EXPECT_TRUE(logger_->last_message == "Debug: one arg: string");

  fmt = "Verbose: one arg: {}";
  SL_VERBOSE(logger(), fmt, "string");
  EXPECT_TRUE(logger_->last_level == Level::VERBOSE);
  EXPECT_TRUE(logger_->last_message == "Verbose: one arg: string");

  fmt = "Info: one arg: {}";
  SL_INFO(logger(), fmt, "string");
  EXPECT_TRUE(logger_->last_level == Level::INFO);
  EXPECT_TRUE(logger_->last_message == "Info: one arg: string");

  fmt = "Warning: one arg: {}";
  SL_WARN(logger(), fmt, "string");
  EXPECT_TRUE(logger_->last_level == Level::WARN);
  EXPECT_TRUE(logger_->last_message == "Warning: one arg: string");

  fmt = "Error: one arg: {}";
  SL_ERROR(logger(), fmt, "string");
  EXPECT_TRUE(logger_->last_level == Level::ERROR);
  EXPECT_TRUE(logger_->last_message == "Error: one arg: string");

  fmt = "Critical: one arg: {}";
  SL_CRITICAL(logger(), fmt, "string");
  EXPECT_TRUE(logger_->last_level == Level::CRITICAL);
  EXPECT_TRUE(logger_->last_message == "Critical: one arg: string");
}

TEST_F(MacrosTest, TwoArg) {
  std::string fmt = "Trace: two args: {} and {}";
  SL_TRACE(logger(), fmt, 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::TRACE);
  EXPECT_TRUE(logger_->last_message == "Trace: two args: 1 and 2.3");

  fmt = "Debug: two args: {} and {}";
  SL_DEBUG(logger(), fmt, 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::DEBUG);
  EXPECT_TRUE(logger_->last_message == "Debug: two args: 1 and 2.3");

  fmt = "Verbose: two args: {} and {}";
  SL_VERBOSE(logger(), fmt, 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::VERBOSE);
  EXPECT_TRUE(logger_->last_message == "Verbose: two args: 1 and 2.3");

  fmt = "Info: two args: {} and {}";
  SL_INFO(logger(), fmt, 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::INFO);
  EXPECT_TRUE(logger_->last_message == "Info: two args: 1 and 2.3");

  fmt = "Warning: two args: {} and {}";
  SL_WARN(logger(), fmt, 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::WARN);
  EXPECT_TRUE(logger_->last_message == "Warning: two args: 1 and 2.3");

  fmt = "Error: two args: {} and {}";
  SL_ERROR(logger(), fmt, 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::ERROR);
  EXPECT_TRUE(logger_->last_message == "Error: two args: 1 and 2.3");

  fmt = "Critical: two args: {} and {}";
  SL_CRITICAL(logger(), fmt, 1, 2.3);
  EXPECT_TRUE(logger_->last_level == Level::CRITICAL);
  EXPECT_TRUE(logger_->last_message == "Critical: two args: 1 and 2.3");
}

TEST_F(MacrosTest, TwentyArg) {
  // clang-format off
  std::string fmt = "Trace: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}";
  SL_TRACE(logger(), fmt, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::TRACE);
  EXPECT_TRUE(logger_->last_message == "Trace: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");

  fmt = "Debug: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}";
  SL_DEBUG(logger(), fmt, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::DEBUG);
  EXPECT_TRUE(logger_->last_message == "Debug: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");

  fmt = "Verbose: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}";
  SL_VERBOSE(logger(), fmt, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::VERBOSE);
  EXPECT_TRUE(logger_->last_message == "Verbose: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");

  fmt = "Info: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}";
  SL_INFO(logger(), fmt, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::INFO);
  EXPECT_TRUE(logger_->last_message == "Info: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");

  fmt = "Warning: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}";
  SL_WARN(logger(), fmt, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::WARN);
  EXPECT_TRUE(logger_->last_message == "Warning: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");

  fmt = "Error: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}";
  SL_ERROR(logger(), fmt, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::ERROR);
  EXPECT_TRUE(logger_->last_message == "Error: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");

  fmt = "Critical: twenty args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}";
  SL_CRITICAL(logger(), fmt, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::CRITICAL);
  EXPECT_TRUE(logger_->last_message == "Critical: twenty args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");
  // clang-format on
}

TEST_F(MacrosTest, CustomLevel) {
  std::string fmt = "Custom: {}";
  auto calculatedLevel = [](Level level) { return level; };

  fmt = "Custom: trace";
  SL_LOG(logger(), Level::TRACE, fmt);
  EXPECT_TRUE(logger_->last_level == Level::TRACE);
  EXPECT_TRUE(logger_->last_message == "Custom: trace");

  fmt = "Custom: {}";
  SL_LOG(logger(), Level::DEBUG, fmt, "debug");
  EXPECT_TRUE(logger_->last_level == Level::DEBUG);
  EXPECT_TRUE(logger_->last_message == "Custom: debug");

  fmt = "Custom: {} is {}";
  SL_LOG(logger(), Level::VERBOSE, fmt, "level", "verbose");
  EXPECT_TRUE(logger_->last_level == Level::VERBOSE);
  EXPECT_TRUE(logger_->last_message == "Custom: level is verbose");

  fmt = "Custom: {}";
  SL_LOG(logger(), calculatedLevel(Level::INFO), fmt, "info");
  EXPECT_TRUE(logger_->last_level == Level::INFO);
  EXPECT_TRUE(logger_->last_message == "Custom: info");

  SL_LOG(logger(), calculatedLevel(Level::WARN), fmt, "warning");
  EXPECT_TRUE(logger_->last_level == Level::WARN);
  EXPECT_TRUE(logger_->last_message == "Custom: warning");

  SL_LOG(logger(), calculatedLevel(Level::ERROR), fmt, "error");
  EXPECT_TRUE(logger_->last_level == Level::ERROR);
  EXPECT_TRUE(logger_->last_message == "Custom: error");

  // clang-format off
  fmt = "Custom: critical; {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}";
  SL_LOG(logger(), calculatedLevel(Level::CRITICAL), fmt, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  EXPECT_TRUE(logger_->last_level == Level::CRITICAL);
  EXPECT_TRUE(logger_->last_message == "Custom: critical; 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20");
  // clang-format on
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
