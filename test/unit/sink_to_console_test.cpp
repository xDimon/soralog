/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "impl/sink_to_console.hpp"

using namespace soralog;
using namespace testing;

class SinkToConsoleTest : public ::testing::Test {
 public:
  struct FakeLogger {
    FakeLogger(std::shared_ptr<SinkToConsole> sink) : sink_(std::move(sink)) {}

    template <typename... Args>
    void debug(std::string_view format, const Args &... args) {
      sink_->push("logger", Level::DEBUG, format, args...);
    }

    void flush() {
      sink_->flush();
    }

   private:
    std::shared_ptr<SinkToConsole> sink_;
  };

  void SetUp() override {
    sink_ = std::make_shared<SinkToConsole>(
        "console", false,
        Sink::ThreadInfoType::NONE,  // ignore thread info
        4,                           // capacity: 4 events
        16384,                       // buffers size: 16 Kb
        latency_.count());
    logger_ = std::make_shared<FakeLogger>(sink_);
  }
  void TearDown() override {}

  std::chrono::milliseconds latency_{10};  // latency: 10 ms
  std::shared_ptr<SinkToConsole> sink_;
  std::shared_ptr<FakeLogger> logger_;
};

TEST_F(SinkToConsoleTest, Logging) {
  auto delay = std::chrono::milliseconds(5);
  logger_->debug("Uno");
  std::this_thread::sleep_for(delay * 1);
  logger_->debug("Dos");
  std::this_thread::sleep_for(delay * 2);
  logger_->debug("Tres");
  std::this_thread::sleep_for(delay * 3);
  logger_->debug("Cuatro");
  std::this_thread::sleep_for(delay * 4);
  logger_->flush();
}

/**
 * @given Sink with one second latency
 * @when Push four message to log every half-second
 * @then Messages will be written in pairs
 * @note Test is disabled, because it consume time and should be start manually
 */
TEST_F(SinkToConsoleTest, DISABLED_NonZeroLatencyLogging) {
  latency_ = std::chrono::milliseconds(1000);
  SetUp();

  auto delay = std::chrono::milliseconds(500);
  logger_->debug("Uno");
  std::this_thread::sleep_for(delay);
  logger_->debug("Dos");
  std::this_thread::sleep_for(delay);
  logger_->debug("Tres");
  std::this_thread::sleep_for(delay);
  logger_->debug("Cuatro");
  std::this_thread::sleep_for(delay);
  logger_->flush();
}

/**
 * @given Sink with zero-latency
 * @when Push four message to log every half-second
 * @then Messages will be written separately
 * @note Test is disabled, because it consume time and should be start manually
 */
TEST_F(SinkToConsoleTest, DISABLED_ZeroLatencyLogging) {
  latency_ = std::chrono::milliseconds::zero();
  SetUp();

  auto delay = std::chrono::milliseconds(500);
  logger_->debug("Uno");
  std::this_thread::sleep_for(delay);
  logger_->debug("Dos");
  std::this_thread::sleep_for(delay);
  logger_->debug("Tres");
  std::this_thread::sleep_for(delay);
  logger_->debug("Cuatro");
  std::this_thread::sleep_for(delay);
  logger_->flush();
}
