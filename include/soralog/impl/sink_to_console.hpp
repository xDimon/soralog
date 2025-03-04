/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <soralog/sink.hpp>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace soralog {
  using namespace std::chrono_literals;

  /**
   * @class SinkToConsole
   * @brief A sink that outputs log messages to the console (stdout or stderr).
   *
   * This sink allows logging to either `stdout` or `stderr`, optionally
   * supporting colored output. It can buffer messages and flush them
   * asynchronously to improve performance.
   */
  class SinkToConsole final : public Sink {
   public:
    /**
     * @enum Stream
     * @brief Specifies the output stream for log messages.
     */
    enum class Stream : uint8_t {
      STDOUT = 1,  ///< Standard output stream.
      STDERR = 2   ///< Standard error stream.
    };

    SinkToConsole() = delete;
    SinkToConsole(SinkToConsole &&) noexcept = delete;
    SinkToConsole(const SinkToConsole &) = delete;
    SinkToConsole &operator=(SinkToConsole &&) noexcept = delete;
    SinkToConsole &operator=(const SinkToConsole &) = delete;

    /**
     * @brief Constructs a console sink for logging.
     * @param name Name of the sink.
     * @param level Minimum logging level.
     * @param stream_type Target output stream (stdout or stderr).
     * @param with_color Enables colored output if true.
     * @param thread_info_type Optional thread info inclusion.
     * @param capacity Optional event buffer capacity.
     * @param max_message_length Optional max log message length.
     * @param buffer_size Optional total buffer size.
     * @param latency Optional max time before automatic flush.
     * @param at_fault Optional reaction type in case of failure.
     */
    SinkToConsole(std::string name,
                  Level level,
                  Stream stream_type,
                  bool with_color = false,
                  std::optional<ThreadInfoType> thread_info_type = {},
                  std::optional<size_t> capacity = {},
                  std::optional<size_t> max_message_length = {},
                  std::optional<size_t> buffer_size = {},
                  std::optional<size_t> latency = {},
                  std::optional<AtFaultReactionType> at_fault = {});

    /**
     * @brief Destroys the console sink, ensuring proper cleanup.
     */
    ~SinkToConsole() override;

    /**
     * @brief Rotates the log (not applicable for console output).
     */
    void rotate() noexcept override {};

    /**
     * @brief Immediately flushes buffered log messages to the console.
     */
    void flush() noexcept override;

   protected:
    /**
     * @brief Flushes logs asynchronously in a worker thread.
     */
    void async_flush() noexcept override;

   private:
    /**
     * @brief Runs the background worker for log processing.
     */
    void run();

    /// Reference to the output stream (stdout or stderr).
    std::ostream &stream_;

    /// Enables colored output if true.
    const bool with_color_;

    /// Worker thread for handling asynchronous log flushing.
    std::unique_ptr<std::thread> sink_worker_{};

    /// Buffer for storing messages before writing to the console.
    std::vector<char> buff_;

    /// Mutex for synchronizing access to the log buffer.
    std::mutex mutex_;

    /// Condition variable for signaling the worker thread.
    std::condition_variable condvar_;

    /// Flag indicating that the sink is shutting down.
    std::atomic_bool need_to_finalize_ = false;

    /// Flag indicating that a flush has been requested.
    std::atomic_bool need_to_flush_ = false;

    /// Timestamp for the next scheduled flush.
    std::atomic<std::chrono::steady_clock::time_point> next_flush_ =
        std::chrono::steady_clock::time_point();

    /// Flag to prevent concurrent flushing operations.
    std::atomic_flag flush_in_progress_ = false;
  };

}  // namespace soralog
