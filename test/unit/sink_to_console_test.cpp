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

   private:
    std::shared_ptr<SinkToConsole> sink_;
  };

  void SetUp() override {
    sink_ = std::make_shared<SinkToConsole>(
        "console", false,
        Sink::ThreadInfoType::NONE,  // ignore thread info
        4,                           // capacity: 4 events
        8182,                        // buffers size: 8 Kb
        10);                         // latency: 200 ms
    logger_ = std::make_shared<FakeLogger>(sink_);
  }
  void TearDown() override {}

  std::chrono::milliseconds latency_{10};  // latency: 10 ms
  std::shared_ptr<SinkToConsole> sink_;
  std::shared_ptr<FakeLogger> logger_;
};

TEST_F(SinkToConsoleTest, Logging) {
  logger_->debug("Uno: {}", 1);
  std::this_thread::sleep_for(latency_ * 0);
  logger_->debug("Dos: {} {}", 1, 2);
  std::this_thread::sleep_for(latency_ * 1);
  logger_->debug("Tres: {} {} {}", 1, 2, 3);
  std::this_thread::sleep_for(latency_ * 2);
  logger_->debug("Cuatro: {} {} {} {}", 1, 2, 3, 4);
  std::this_thread::sleep_for(latency_ * 3);
}
