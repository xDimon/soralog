/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <soralog/sink.hpp>

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace soralog {
  using namespace std::chrono_literals;

  /**
   * @class SinkToEventLog
   * @brief A sink that writes log messages to the Windows Event Log.
   *
   * Events go to the Application log under the given source, one record per
   * message, as an insertion string of the given event ID. The sink registers
   * nothing: Event Viewer shows the text with a note that the description of
   * the event ID is not found, unless the installer of the application has
   * registered the source with a message file, where the message of that ID
   * is "%1".
   *
   * Levels map to record types: fatal, critical and error to error, warning
   * to warning, info, verbose and debug to information; trace is not logged.
   */
  class SinkToEventLog final : public Sink {
   public:
    SinkToEventLog() = delete;
    SinkToEventLog(SinkToEventLog &&) noexcept = delete;
    SinkToEventLog(const SinkToEventLog &) = delete;
    SinkToEventLog &operator=(SinkToEventLog &&) noexcept = delete;
    SinkToEventLog &operator=(const SinkToEventLog &) = delete;

    /**
     * @brief Constructs an Event Log sink.
     * @param name Name of the sink.
     * @param level Minimum logging level.
     * @param source Event source name, as Event Viewer shows it.
     * @param thread_info_type Optional thread info inclusion.
     * @param capacity Optional event buffer capacity.
     * @param max_message_length Optional maximum log message length.
     * @param buffer_size Optional total buffer size.
     * @param latency Optional max time before automatic flush.
     * @param at_fault Optional reaction type in case of failure (not used
     * by this sink).
     * @param event_id Optional event ID of the records; 0 by default.
     * @throws std::system_error if the event source can't be open.
     */
    SinkToEventLog(std::string name,
                   Level level,
                   std::string source,
                   std::optional<ThreadInfoType> thread_info_type = {},
                   std::optional<size_t> capacity = {},
                   std::optional<size_t> max_message_length = {},
                   std::optional<size_t> buffer_size = {},
                   std::optional<size_t> latency = {},
                   std::optional<AtFaultReactionType> at_fault = {},
                   std::optional<uint32_t> event_id = {});

    /**
     * @brief Destroys the sink, ensuring buffered logs are written.
     */
    ~SinkToEventLog() override;

    /**
     * @brief No-op: the Event Log has nothing to rotate.
     */
    void rotate() noexcept override {};

    /**
     * @brief Immediately writes buffered log messages to the Event Log.
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
     * @param bounded Take only the events queued before the flush started;
     * otherwise up to the capacity.
     */
    void internal_flush(bool wait = false, bool bounded = true) noexcept;

    /**
     * @brief Writes out queued events; called with flush_in_progress_ set.
     * @param limit Maximum number of events to write out.
     */
    void flush_events(size_t limit) noexcept;

    /// Event source name.
    const std::string source_;

    /// Event ID of the records.
    const uint32_t event_id_;

    /// Event source handle (HANDLE); void * to keep windows.h out.
    void *handle_ = nullptr;

    /// Worker thread for handling asynchronous log flushing.
    std::unique_ptr<std::thread> sink_worker_{};

    /// Buffer for a message before conversion to UTF-16.
    std::vector<char> buff_;

    /// Buffer for a message in UTF-16, as the Event Log takes it.
    std::vector<wchar_t> wide_buff_;

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
    std::atomic_flag flush_in_progress_{};
    /// Number of flushes started; changed only under flush_in_progress_.
    std::atomic_size_t flushes_started_ = 0;
    /// Number of the last finished flush; changed only under
    /// flush_in_progress_.
    std::atomic_size_t flushes_done_ = 0;
  };

}  // namespace soralog
