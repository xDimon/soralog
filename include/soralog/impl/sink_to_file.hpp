/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <soralog/sink.hpp>

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>

namespace soralog {
  using namespace std::chrono_literals;

  /**
   * @class SinkToFile
   * @brief A sink that writes log messages to a file.
   *
   * This sink logs messages to a specified file, supporting buffering
   * and asynchronous flushing. A background worker thread handles log
   * processing to minimize performance impact.
   */
  class SinkToFile final : public Sink {
   public:
    SinkToFile() = delete;
    SinkToFile(SinkToFile &&) noexcept = delete;
    SinkToFile(const SinkToFile &) = delete;
    SinkToFile &operator=(SinkToFile &&) noexcept = delete;
    SinkToFile &operator=(const SinkToFile &) = delete;

    /**
     * @brief Constructs a file sink for logging.
     * @param name Name of the sink.
     * @param level Minimum logging level.
     * @param path File path for log output.
     * @param thread_info_type Optional thread info inclusion.
     * @param capacity Optional event buffer capacity.
     * @param buffer_size Optional total buffer size.
     * @param max_message_length Optional maximum log message length.
     * @param latency Optional max time before automatic flush.
     * @param at_fault Optional reaction type in case of failure.
     */
    SinkToFile(std::string name,
               Level level,
               std::filesystem::path path,
               std::optional<ThreadInfoType> thread_info_type = {},
               std::optional<size_t> capacity = {},
               std::optional<size_t> buffer_size = {},
               std::optional<size_t> max_message_length = {},
               std::optional<size_t> latency = {},
               std::optional<AtFaultReactionType> at_fault = {});

    /**
     * @brief Destroys the file sink, ensuring buffered logs are written.
     */
    ~SinkToFile() override;

    /**
     * @brief Rotates the log file.
     *
     * Closes the current file and opens a new one with the same name.
     */
    void rotate() noexcept override;

    /**
     * @brief Immediately writes buffered log messages to the file.
     */
    void flush() noexcept override;

   protected:
    /**
     * @brief Asynchronously flushes log messages in a background thread.
     */
    void async_flush() noexcept override;

   private:
    /**
     * @brief Runs the background worker for log processing.
     */
    void run();

    /// Path to the log file.
    const std::filesystem::path path_;

    /// Worker thread for asynchronous logging.
    std::unique_ptr<std::thread> sink_worker_{};

    /// Buffer for storing log messages before writing to the file.
    std::vector<char> buff_;

    /// Output file stream.
    std::ofstream out_{};

    /// Mutex for synchronizing access to the log buffer.
    std::mutex mutex_{};

    /// Condition variable for signaling the worker thread.
    std::condition_variable condvar_{};

    /// Flag indicating that the sink is shutting down.
    std::atomic_bool need_to_finalize_ = false;

    /// Flag indicating that a flush operation is required.
    std::atomic_bool need_to_flush_ = false;

    /// Flag indicating that a log file rotation is needed.
    std::atomic_bool need_to_rotate_ = false;

    /// Timestamp for the next scheduled flush.
    std::atomic<std::chrono::steady_clock::time_point> next_flush_ =
        std::chrono::steady_clock::time_point();

    /// Flag to prevent concurrent flushing operations.
    std::atomic_flag flush_in_progress_ = false;
  };

}  // namespace soralog
