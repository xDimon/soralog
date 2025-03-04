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

/**
 * @class SinkToConsoleTest
 * @brief Test fixture for testing logging to console using SinkToConsole.
 *
 * This fixture provides a helper FakeLogger class to interact with the sink.
 */
class SinkToConsoleTest : public ::testing::Test {
 public:
  /**
   * @class FakeLogger
   * @brief A simple logger wrapper around SinkToConsole for testing.
   *
   * This mock logger directly writes debug messages to the SinkToConsole instance.
   */
  struct FakeLogger {
    explicit FakeLogger(std::shared_ptr<SinkToConsole> sink)
        : sink_(std::move(sink)) {}

    /**
     * @brief Logs a debug message.
     * @tparam Args Variadic template for message arguments.
     * @param format The format string.
     * @param args The arguments for the format string.
     */
    template <typename... Args>
    void debug(std::string_view format, const Args &...args) {
      sink_->push("logger", Level::DEBUG, format, args...);
    }

    /**
     * @brief Flushes the sink, ensuring all messages are written to the console.
     */
    void flush() {
      sink_->flush();
    }

   private:
    std::shared_ptr<SinkToConsole> sink_;
  };

  /**
   * @brief Creates a FakeLogger instance with a SinkToConsole backend.
   * @param latency The latency for the sink (controls delayed writes).
   * @return A shared pointer to a FakeLogger instance.
   */
  std::shared_ptr<FakeLogger> createLogger(std::chrono::milliseconds latency) {
    auto sink = std::make_shared<SinkToConsole>(
        "console",
        Level::TRACE,
        SinkToConsole::Stream::STDOUT,  // Standard output stream
        false,                          // No color formatting
        Sink::ThreadInfoType::ID,       // Log thread ID
        4,                              // Capacity: 4 events
        64,                             // Max message length: 64 bytes
        16384,                          // Buffer size: 16 KB
        latency.count());
    return std::make_shared<FakeLogger>(std::move(sink));
  }
};

/**
 * @test Logging
 * @brief Tests logging behavior with a console sink.
 *
 * @given A logger writing to a console sink with a specified latency.
 * @when Multiple messages are logged with varying delays.
 * @then The messages appear in the console output.
 */
TEST_F(SinkToConsoleTest, Logging) {
  auto logger = createLogger(20ms);
  auto delay = 1ms;
  int count = 100;

  // Logging in multiple rounds with varying delays
  for (int round = 1; round <= 3; ++round) {
    for (int i = 1; i <= count; ++i) {
      logger->debug(
          "round: {}, message: {}, delay: {}ms", round, i, abs(i - count / 2));
      std::this_thread::sleep_for(delay * abs(i - count / 2));
    }
  }

  // Ensure all buffered logs are flushed
  logger->flush();
}

/**
 * @test NonZeroLatencyLogging
 * @brief Tests delayed logging behavior with a non-zero latency.
 *
 * @given A sink with a one-second latency.
 * @when Messages are logged every half-second.
 * @then Messages are written to the console in pairs due to buffering.
 * @note This test is disabled as it requires manual observation.
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
 * @test ZeroLatencyLogging
 * @brief Tests immediate logging behavior with zero latency.
 *
 * @given A sink with zero-latency.
 * @when Messages are logged every half-second.
 * @then Messages are written to the console immediately.
 * @note This test is disabled as it requires manual observation.
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
 * @test MultithreadLogging
 * @brief Tests concurrent logging behavior in a multi-threaded environment.
 *
 * @given A sink with 40ms latency and multiple threads.
 * @when Multiple threads log messages simultaneously.
 * @then The messages are written to the console without corruption.
 */
TEST_F(SinkToConsoleTest, MultithreadLogging) {
  auto logger = createLogger(40ms);

  size_t threads_n = 10;
  size_t iters_n = 100;

#if __cplusplus >= 202002L
  std::latch latch(threads_n);  // Synchronization barrier for thread start
#endif

  auto task = [&] {
#if __cplusplus >= 202002L
    latch.arrive_and_wait();  // Ensure all threads start simultaneously
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
  for (auto i = 0; i < threads_n; ++i) {
    threads.emplace_back([&] {
      // Start logging task in each thread
      task();
    });
  }

  // Wait for all threads to complete
  for (auto &t : threads) {
    t.join();
  }
}