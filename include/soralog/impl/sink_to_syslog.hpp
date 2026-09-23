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
   * @class SinkToSyslog
   * @brief A sink that sends log messages to the system's syslog.
   *
   * This sink writes log messages to syslog, supporting optional buffering
   * and controlled flushing. With nonzero latency it operates in a separate
   * worker thread to handle log events efficiently.
   */
  class SinkToSyslog final : public Sink {
   public:
    SinkToSyslog() = delete;
    SinkToSyslog(SinkToSyslog &&) noexcept = delete;
    SinkToSyslog(const SinkToSyslog &) = delete;
    SinkToSyslog &operator=(SinkToSyslog &&) noexcept = delete;
    SinkToSyslog &operator=(const SinkToSyslog &) = delete;

    /**
     * @brief Constructs a syslog sink.
     * @param name Name of the sink.
     * @param level Minimum logging level.
     * @param ident Identifier string for syslog messages.
     * @param thread_info_type Optional thread info inclusion.
     * @param capacity Optional event buffer capacity.
     * @param max_message_length Optional max length of a log message.
     * @param buffer_size Optional total buffer size.
     * @param latency Optional max time before automatic flush.
     * @param at_fault Optional reaction type in case of failure (not used
     * by this sink).
     */
    SinkToSyslog(std::string name,
                 Level level,
                 std::string ident,
                 std::optional<ThreadInfoType> thread_info_type = {},
                 std::optional<size_t> capacity = {},
                 std::optional<size_t> max_message_length = {},
                 std::optional<size_t> buffer_size = {},
                 std::optional<size_t> latency = {},
                 std::optional<AtFaultReactionType> at_fault = {});

    /**
     * @brief Destroys the syslog sink, ensuring proper cleanup.
     */
    ~SinkToSyslog() override;

    /**
     * @brief Rotates the log (not applicable for syslog).
     */
    void rotate() noexcept override {};

    /**
     * @brief Immediately flushes buffered log messages to syslog.
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
     */
    void flush_events(size_t limit) noexcept;

    /// Flag to track whether syslog has been initialized.
    static std::atomic_bool syslog_is_opened_;

    /// Identifier used in syslog messages.
    const std::string ident_;

    /// Worker thread for handling asynchronous log flushing.
    std::unique_ptr<std::thread> sink_worker_{};

    /// Buffer for storing messages before writing to syslog.
    std::vector<char> buff_;

    /// Mutex for the worker to sleep on and to be woken up.
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
    /// Number of flushes started; changed only under flush_in_progress_.
    std::atomic_size_t flushes_started_ = 0;
    /// Number of the last finished flush; changed only under
    /// flush_in_progress_.
    std::atomic_size_t flushes_done_ = 0;
  };

}  // namespace soralog
