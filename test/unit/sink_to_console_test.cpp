/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "soralog/impl/sink_to_console.hpp"

using namespace soralog;
using namespace testing;
using namespace std::chrono_literals;

class SinkToConsoleTest : public ::testing::Test {
 public:
  struct FakeLogger {
    explicit FakeLogger(std::shared_ptr<SinkToConsole> sink) : sink_(std::move(sink)) {}

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

  std::shared_ptr<FakeLogger> createLogger(std::chrono::milliseconds latency) {
    auto sink = std::make_shared<SinkToConsole>(
        "console", false,
        Sink::ThreadInfoType::NONE,  // ignore thread info
        4,                           // capacity: 4 events
        16384,                       // buffers size: 16 Kb
        latency.count());
    return std::make_shared<FakeLogger>(std::move(sink));
  }
};

TEST_F(SinkToConsoleTest, Logging) {
  auto logger = createLogger(20ms);
  auto delay = 1ms;
  int count = 100;
  for (int round = 1; round <= 3; ++round) {
    for (int i = 1; i <= count; ++i) {
      logger->debug("round: {}, message: {}, delay: {}ms", round, i,
                    abs(i - count / 2));
      std::this_thread::sleep_for(delay * abs(i - count / 2));
    }
  }
  logger->flush();
}

/**
 * @given Sink with one second latency
 * @when Push four message to log every half-second
 * @then Messages will be written in pairs
 * @note Test is disabled, because it should be started and watched manually
 */
TEST_F(SinkToConsoleTest, DISABLED_NonZeroLatencyLogging) {
  auto logger = createLogger(1000ms);
  auto delay = 500ms;

  logger->debug("Uno");
  std::this_thread::sleep_for(delay);
  logger->debug("Dos");
  std::this_thread::sleep_for(delay);
  logger->debug("Tres");
  std::this_thread::sleep_for(delay);
  logger->debug("Cuatro");
  std::this_thread::sleep_for(delay);
  logger->flush();
}

/**
 * @given Sink with zero-latency
 * @when Push four message to log every half-second
 * @then Messages will be written separately
 * @note Test is disabled, because it should be started and watched manually
 */
TEST_F(SinkToConsoleTest, ZeroLatencyLogging) {
  auto logger = createLogger(0ms);
  auto delay = 500ms;

  logger->debug("Uno");
  std::this_thread::sleep_for(delay);
  logger->debug("Dos");
  std::this_thread::sleep_for(delay);
  logger->debug("Tres");
  std::this_thread::sleep_for(delay);
  logger->debug("Cuatro");
  std::this_thread::sleep_for(delay);
  logger->flush();
}
