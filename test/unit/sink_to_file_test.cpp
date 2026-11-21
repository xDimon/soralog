/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <iterator>

#include "soralog/impl/sink_to_file.hpp"

#ifdef _WIN32
// After soralog headers: windows.h defines macros named like Level values
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef ERROR
#undef IGNORE
#else
#include <sys/resource.h>
#endif

using namespace soralog;
using namespace testing;
using namespace std::chrono_literals;

/**
 * @class SinkToFileTest
 * @brief Test fixture for testing logging to a file using SinkToFile.
 *
 * This fixture sets up a temporary file for logging and provides
 * a helper FakeLogger class to interact with the sink.
 */
class SinkToFileTest : public ::testing::Test {
 public:
  /**
   * @class FakeLogger
   * @brief A simple logger wrapper around SinkToFile for testing.
   *
   * This mock logger directly writes debug messages to the SinkToFile instance.
   */
  struct FakeLogger {
    explicit FakeLogger(std::shared_ptr<SinkToFile> sink)
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
     * @brief Flushes the sink, ensuring all messages are written to the file.
     */
    void flush() {
      sink_->flush();
    }

   private:
    std::shared_ptr<SinkToFile> sink_;
  };

  /**
   * @brief Sets up the test environment by creating a temporary log file.
   *
   * The file is created using `mkstemp` to ensure a unique and writable
   * file path.
   */
  void SetUp() override {
    std::string path(
        (std::filesystem::temp_directory_path() / "soralog_test_XXXXXX")
            .c_str());
    if (mkstemp(path.data()) == -1) {
      FAIL() << "Can't create output file for test";
    }
    path_ = std::filesystem::path(path);
  }

  /**
   * @brief Cleans up the test environment by removing the temporary log file.
   */
  void TearDown() override {
    std::remove(path_.native().data());
    sink_.reset();
  }

  /**
   * @brief Creates a FakeLogger instance with a SinkToFile backend.
   * @param latency The latency for the sink (controls delayed writes).
   * @return A shared pointer to a FakeLogger instance.
   */
  std::shared_ptr<FakeLogger> createLogger(std::chrono::milliseconds latency) {
    sink_ = std::make_shared<SinkToFile>(
        "file",
        Level::TRACE,
        path_,
        Sink::ThreadInfoType::NONE,  // Ignore thread info in logs
        4,                           // Capacity: 4 events
        64,                          // Max message length: 64 bytes
        16384,                       // Buffer size: 16 KB
        latency.count());            // Log flush latency
    return std::make_shared<FakeLogger>(sink_);
  }

 protected:
  std::filesystem::path path_;
  std::shared_ptr<SinkToFile> sink_;
};

/**
 * @test Logging
 * @brief Tests the logging behavior of SinkToFile under different delays.
 *
 * @given A logger writing to a file sink with a specified latency.
 * @when Multiple messages are logged with varying delays.
 * @then The messages are written to the file and can be flushed manually.
 */
TEST_F(SinkToFileTest, Logging) {
  auto logger = createLogger(20ms);  // Create a logger with 20ms flush latency
  auto delay = 1ms;                  // Base delay between log messages
  int count = 100;                   // Number of messages per round

  // Logging in multiple rounds with varying delays
  for (int round = 1; round <= 3; ++round) {
    for (int i = 1; i <= count; ++i) {
      logger->debug("round: {}, message: {}, delay: {}ms",
                    round,
                    i,
                    abs(i - (count / 2)));
      std::this_thread::sleep_for(delay * abs(i - (count / 2)));
    }
  }

  // Ensure all buffered logs are written to the file
  logger->flush();
}

/**
 * @test Rotating
 * @brief Tests reopening of the log file by rotate().
 *
 * @given A logger writing to a file sink with zero latency.
 * @when The log file is renamed and the sink is rotated.
 * @then The next message goes to a new file at the original path, and the
 *       renamed file stays.
 */
TEST_F(SinkToFileTest, Rotating) {
  std::filesystem::path path_to_old_file = path_.string() + ".old";

  // ASSERT_TRUE(not std::filesystem::exists(path_));
  ASSERT_TRUE(not std::filesystem::exists(path_to_old_file));

  auto logger = createLogger(0ms);  // Create a logger with zero flush latency

  logger->debug("Message to log-file #1");
  logger->flush();

  ASSERT_TRUE(std::filesystem::exists(path_));
  ASSERT_TRUE(not std::filesystem::exists(path_to_old_file));

  std::filesystem::rename(path_, path_to_old_file);
  sink_->rotate();

  logger->debug("Message to log-file #2");
  logger->flush();

  ASSERT_TRUE(std::filesystem::exists(path_));
  ASSERT_TRUE(std::filesystem::exists(path_to_old_file));

  std::filesystem::remove(path_);
  std::filesystem::remove(path_to_old_file);
}

namespace {
  /// @brief Counts lines in the file
  size_t countLines(const std::filesystem::path &path) {
    std::ifstream in(path);
    return std::count(std::istreambuf_iterator<char>(in),
                      std::istreambuf_iterator<char>(),
                      '\n');
  }

  /// @brief CPU time consumed by the whole process
  std::chrono::microseconds processCpuTime() {
#ifdef _WIN32
    FILETIME creation{}, exited{}, kernel{}, user{};
    GetProcessTimes(GetCurrentProcess(), &creation, &exited, &kernel, &user);
    // FILETIME counts 100-nanosecond intervals
    auto to_us = [](const FILETIME &time) {
      return std::chrono::microseconds(
          ((static_cast<uint64_t>(time.dwHighDateTime) << 32)
           | time.dwLowDateTime)
          / 10);
    };
    return to_us(kernel) + to_us(user);
#else
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return std::chrono::seconds(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec)
         + std::chrono::microseconds(usage.ru_utime.tv_usec
                                     + usage.ru_stime.tv_usec);
#endif
  }
}  // namespace

/**
 * @test FlushWritesAllEvents
 * @brief Tests that a single flush drains the whole event buffer.
 *
 * @given A file sink with large latency, so the worker does not interfere.
 * @when More events than the buffer capacity are logged, then flushed.
 * @then Every event is written to the file.
 */
TEST_F(SinkToFileTest, FlushWritesAllEvents) {
  auto logger = createLogger(1000ms);  // Capacity of buffer is 4 events
  const size_t count = 10;

  for (size_t i = 1; i <= count; ++i) {
    logger->debug("message #{}", i);
  }
  logger->flush();

  EXPECT_EQ(countLines(path_), count);
}

/**
 * @test IdleWorkerDoesNotSpin
 * @brief Tests that the worker thread of an idle sink sleeps.
 *
 * @given A file sink with small latency and nothing to log.
 * @when Time passes.
 * @then The process consumes almost no CPU time.
 */
TEST_F(SinkToFileTest, IdleWorkerDoesNotSpin) {
  auto logger = createLogger(10ms);
  std::this_thread::sleep_for(50ms);  // Let the worker start

  auto wall = 500ms;
  auto before = processCpuTime();
  std::this_thread::sleep_for(wall);
  auto spent = processCpuTime() - before;

  EXPECT_LT(spent, wall / 5);
}
