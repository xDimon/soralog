/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "soralog/impl/sink_to_file.hpp"

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
  }

  /**
   * @brief Creates a FakeLogger instance with a SinkToFile backend.
   * @param latency The latency for the sink (controls delayed writes).
   * @return A shared pointer to a FakeLogger instance.
   */
  std::shared_ptr<FakeLogger> createLogger(std::chrono::milliseconds latency) {
    auto sink = std::make_shared<SinkToFile>(
        "file",
        Level::TRACE,
        path_,
        Sink::ThreadInfoType::NONE,  // Ignore thread info in logs
        4,                           // Capacity: 4 events
        64,                          // Max message length: 64 bytes
        16384,                       // Buffer size: 16 KB
        latency.count());            // Log flush latency
    return std::make_shared<FakeLogger>(std::move(sink));
  }

 private:
  std::filesystem::path path_;
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
