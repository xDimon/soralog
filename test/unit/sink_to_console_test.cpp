/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "soralog/impl/sink_to_console.hpp"

#if __cplusplus >= 202002L
#include <latch>
#endif

using namespace soralog;
using namespace testing;
using namespace std::chrono_literals;

class SinkToConsoleTest : public ::testing::Test {
 public:
  struct FakeLogger {
    explicit FakeLogger(std::shared_ptr<SinkToConsole> sink)
        : sink_(std::move(sink)) {}

    template <typename... Args>
    void debug(std::string_view format, const Args &...args) {
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
        "console",
        Level::TRACE,
        SinkToConsole::Stream::STDOUT,  // standard output stream
        false,                          // no color
        Sink::ThreadInfoType::ID,       // ignore thread info
        4,                              // capacity: 4 events
        64,                             // max message length: 64 byte
        16384,                          // buffers size: 16 Kb
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
      logger->debug(
          "round: {}, message: {}, delay: {}ms", round, i, abs(i - count / 2));
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

/**
 * @given Sink with zero-latency
 * @when Push four message to log every half-second
 * @then Messages will be written separately
 * @note Test is disabled, because it should be started and watched manually
 */
TEST_F(SinkToConsoleTest, MultithreadLogging) {
  auto logger = createLogger(40ms);

  size_t treads_n = 10;
  size_t iters_n = 100;

#if __cplusplus >= 202002L
  std::latch latch(treads_n);
#endif

  auto task = [&] {
#if __cplusplus >= 202002L
    latch.arrive_and_wait();
#endif
    std::mutex m;
    for (auto i = 0; i < iters_n; ++i) {
      logger->debug("iteration {}.1", i);
      logger->debug("iteration {}.2", i);
      logger->debug("iteration {}.3", i);
      logger->debug("iteration {}.4", i);
      logger->debug("iteration {}.5", i);
      logger->debug("iteration {}.6", i);
      logger->debug("iteration {}.7", i);
      std::unique_lock l(m);
      logger->debug("iteration {}.8", i);
      logger->debug("iteration {}.9", i);
      logger->debug("iteration {}.0", i);
    }
  };

  std::vector<std::thread> threads;
  for (auto i = 0; i < treads_n; ++i) {
    threads.emplace_back([&] {
      //      soralog::util::setThreadName(fmt::format("{}", i));
      task();
    });
  }

  for (auto &t : threads) {
    t.join();
  }
}
