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
   * and asynchronous flushing. With nonzero latency a background worker
   * thread handles log processing to minimize performance impact.
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
     * @param max_message_length Optional maximum log message length.
     * @param buffer_size Optional total buffer size.
     * @param latency Optional max time before automatic flush.
     * @param at_fault Optional reaction type in case of failure.
     */
    SinkToFile(std::string name,
               Level level,
               std::filesystem::path path,
               std::optional<ThreadInfoType> thread_info_type = {},
               std::optional<size_t> capacity = {},
               std::optional<size_t> max_message_length = {},
               std::optional<size_t> buffer_size = {},
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
     * @brief Requests flushing without waiting: wakes the worker, or
     * flushes in the calling thread at zero latency.
     */
    void async_flush() noexcept override;

    /**
     * @brief Flushes in the calling thread; returns when the events
     * queued before the call are written.
     */
    void sync_flush() noexcept override;

   private:
    /**
     * @brief Runs the background worker for log processing.
     */
    void run();

    /**
     * @brief Writes out queued events.
     * @param wait Wait for the thread flushing now, if any, and write out the
     * rest after it; otherwise leave the work to that thread.
     * @param bounded Take only the events queued before the flush started, so
     * that a producer flushing does not drain the events of others endlessly;
     * otherwise up to the capacity, enough for the worker to batch writes.
     */
    void internal_flush(bool wait = false, bool bounded = true) noexcept;

    /**
     * @brief Writes out queued events; called with flush_in_progress_ set.
     * @param limit Maximum number of events to write out.
     * @param flush_stream Flush the output stream when done.
     * @return True if the log file has been reopened.
     */
    bool flush_events(size_t limit, bool flush_stream) noexcept;

    /// Path to the log file.
    const std::filesystem::path path_;

    /// Worker thread for asynchronous logging.
    std::unique_ptr<std::thread> sink_worker_{};

    /// Buffer for storing log messages before writing to the file.
    std::vector<char> buff_;

    /// Output file stream.
    std::ofstream out_{};

    /// Mutex for the worker to sleep on and to be woken up.
    std::mutex mutex_{};

    /// Condition variable for signaling the worker thread.
    std::condition_variable condvar_{};

    /// Flag indicating that the sink is shutting down.
    std::atomic_bool need_to_finalize_ = false;

    /// Request to flush: wakes the worker and makes the next flush flush the
    /// output stream too.
    std::atomic_bool need_to_flush_ = false;

    /// Flag indicating that a log file rotation is needed.
    std::atomic_bool need_to_rotate_ = false;

    /// Timestamp for the next scheduled flush.
    std::atomic<std::chrono::steady_clock::time_point> next_flush_ =
        std::chrono::steady_clock::time_point();

    /// Flag to prevent concurrent flushing operations.
    std::atomic_flag flush_in_progress_{};
    /// Number of flushes started; changed only under flush_in_progress_.
    std::atomic_size_t flushes_started_ = 0;
    /// Number of the last finished flush; changed only under
    /// flush_in_progress_.
    std::atomic_size_t flushes_done_ = 0;
  };

}  // namespace soralog
