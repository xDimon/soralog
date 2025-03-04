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

/**
 * @class MacrosTest
 * @brief Test fixture for testing logging macros.
 *
 * This fixture provides a fake logger implementation and verifies that
 * different logging macros work as expected.
 */
class MacrosTest : public ::testing::Test {
 public:
  /**
   * @class FakeLogger
   * @brief A simple in-memory logger for testing logging macros.
   *
   * This logger stores the last log message and its level, allowing
   * tests to verify expected behavior.
   */
  struct FakeLogger {
    template <typename Format, typename... Args>
    void log(Level lvl, const Format &format, Args &&...args) {
      last_level = lvl;
      using OutputIt = decltype(message_buf.begin());
      size_t len =
          ::fmt::vformat_to_n<OutputIt>(
              message_buf.begin(),
              message_buf.size(),
              ::fmt::detail_exported::compile_string_to_view<char>(format),
              ::fmt::make_format_args(args...))
              .size;
      last_message = std::string_view(message_buf.data(),
                                      std::min(len, message_buf.size()));
    }

    /**
     * @brief Returns the current log level.
     * @return Level The logging level (default: TRACE).
     */
    static Level level() {
      return Level::TRACE;
    }

    Level last_level = Level::OFF;      ///< Stores the last logged level
    std::array<char, 100> message_buf;  ///< Buffer for storing last message
    std::string_view last_message;      ///< Stores the last logged message
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

/**
 * @test NoArg
 * @brief Tests logging macros without arguments.
 *
 * @given A logger instance.
 * @when Logging macros are called without additional arguments.
 * @then The correct log level and message should be recorded.
 */
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

/**
 * @test OneArg
 * @brief Tests logging macros with one argument.
 *
 * @given A logger instance.
 * @when Logging macros are called with a single argument.
 * @then The correct log level and formatted message should be recorded.
 */
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

/**
 * @test TwoArg
 * @brief Tests logging macros with two arguments.
 *
 * @given A logger instance.
 * @when Logging macros are called with two arguments.
 * @then The correct log level and formatted message should be recorded.
 */
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

/**
 * @test TwentyArg
 * @brief Tests logging macros with twenty arguments.
 *
 * @given A logger instance.
 * @when Logging macros are called with exactly twenty arguments.
 * @then The correct log level and formatted message should be recorded.
 */
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
/**
 * @test CustomLevel
 * @brief Tests logging macros with dynamically computed log levels.
 *
 * @given A logger instance.
 * @when Logging macros are called with computed log levels.
 * @then The correct level and formatted message should be recorded.
 */
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

/**
 * @test CalculatedFormat
 * @brief Tests logging macros with dynamically computed format strings.
 *
 * @given A logger instance.
 * @when Logging macros are called with computed format strings.
 * @then The correct formatted message should be recorded.
 */
/**
 * @test CalculatedFormat
 * @brief Tests logging macros with dynamically calculated format strings.
 *
 * @given A logger instance.
 * @when Different types of format strings (string literals, C-strings,
 *        std::string_view, std::string) are used in logging macros.
 * @then The formatted log message should match the expected output.
 */
TEST_F(MacrosTest, CalculatedFormat) {
  // String literal format string
  SL_DEBUG(logger(), "ping => {}", "pong");
  EXPECT_TRUE(logger_->last_message == "ping => pong");

  // C-string format string
  static const char *c_string = "ping => {}";
  SL_DEBUG_DF(logger(), c_string, "pong");
  EXPECT_TRUE(logger_->last_message == "ping => pong");

  // std::string_view format string
  std::string_view string_view("ping => {}");
  SL_DEBUG_DF(logger(), string_view, "pong");
  EXPECT_TRUE(logger_->last_message == "ping => pong");

  // std::string format string
  std::string string("ping => {}");
  SL_DEBUG_DF(logger(), string, "pong");
  EXPECT_TRUE(logger_->last_message == "ping => pong");
}

/**
 * @test CalculatedArgs
 * @brief Tests logging macros with dynamically calculated arguments.
 *
 * @given A logger instance.
 * @when Logging macros receive arguments generated by lambda functions.
 * @then The formatted log message should match the expected output.
 */
TEST_F(MacrosTest, CalculatedArgs) {
  // clang-format off

  // Lambda functions for generating values dynamically
  auto number = [](int i) { return "#" + std::to_string(i); };
  auto length = [](std::string s) { return std::to_string(s.length()); };

  // Logging messages using calculated values
  SL_DEBUG(logger(), "Numbers: {}, {}, {}", number(1), number(2), number(3));
  EXPECT_TRUE(logger_->last_message == "Numbers: #1, #2, #3");

  SL_DEBUG(logger(), "Lengths: {}, {}, {}", length("*"), length("**"), length("***"));
  EXPECT_TRUE(logger_->last_message == "Lengths: 1, 2, 3");

  // clang-format on
}

/**
 * @test StructuredBinding
 * @brief Tests logging macros with structured binding variables.
 *
 * @given A logger instance.
 * @when A structured binding variable is logged.
 * @then The formatted log message should match the expected output.
 */
TEST_F(MacrosTest, StructuredBinding) {
  struct {
    int x = 1;
  } a;

  // Using structured binding to extract value
  auto &[x] = a;

  // Logging the structured binding variable
  SL_DEBUG(logger(), "x: {}", x);
  EXPECT_TRUE(logger_->last_message == "x: 1");

  // Using a dynamically formatted string
  static const char *df = "x: {}";
  SL_DEBUG_DF(logger(), df, x);
  EXPECT_TRUE(logger_->last_message == "x: 1");
}
