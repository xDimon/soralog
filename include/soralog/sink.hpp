/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <fmt/format.h>

#include <soralog/circular_buffer.hpp>
#include <soralog/event.hpp>

#ifdef NDEBUG
#define IF_RELEASE true
#else
#define IF_RELEASE false
#endif

#if not defined(LIKELY_IF)
#if __cplusplus >= 202002L
#define LIKELY_IF(x) [[likely]] if (x)
#elif defined(__has_builtin)
#if __has_builtin(__builtin_expect)
#define LIKELY_IF(x) if (__builtin_expect((x), 1))
#else
#define LIKELY_IF(x) if (x)
#endif
#else
#define LIKELY_IF(x) if (x)
#endif
#endif

namespace soralog {

  /**
   * @class Sink
   * @brief Base class for all log sinks.
   *
   * Accumulates events in a circular buffer without locks and flushes them
   * to the destination when needed.
   */
  class Sink {
   public:
    /**
     * @enum ThreadInfoType
     * @brief Defines how thread info is logged.
     */
    enum class ThreadInfoType : uint8_t {
      NONE,  ///< Do not log thread info
      NAME,  ///< Log thread name
      ID     ///< Log thread ID
    };

    /**
     * @enum AtFaultReactionType
     * @brief Defines behavior on logging failure.
     */
    enum class AtFaultReactionType : uint8_t {
      WAIT,       ///< Retry writing
      TERMINATE,  ///< Exit process with error
      IGNORE      ///< Drop messages and continue
    };

    Sink() = delete;
    Sink(const Sink &) = delete;
    Sink(Sink &&) noexcept = delete;
    virtual ~Sink() = default;
    Sink &operator=(const Sink &) = delete;
    Sink &operator=(Sink &&) noexcept = delete;

    /**
     * @brief Constructs a Sink instance.
     * @param name Sink name.
     * @param level Minimum log level.
     * @param thread_info_type Thread info logging type.
     * @param max_events Max events in buffer.
     * @param max_message_length Max log message length.
     * @param max_buffer_size Max buffer size.
     * @param latency Maximum write delay, in milliseconds.
     * @param at_fault Reaction on write failure.
     */
    Sink(std::string name,
         Level level,
         ThreadInfoType thread_info_type,
         size_t max_events,
         size_t max_message_length,
         size_t max_buffer_size,
         size_t latency,
         AtFaultReactionType at_fault)
        : name_(std::move(name)),
          level_(level),
          thread_info_type_(thread_info_type),
          max_message_length_(max_message_length),
          max_buffer_size_(max_buffer_size),
          latency_(latency),
          at_fault_(at_fault),
          events_(max_events, max_message_length) {
      // Auto-fix buffer size
      if (max_buffer_size_ < max_message_length * 2) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        const_cast<size_t &>(max_buffer_size_) = max_message_length * 2;
      }
    }

    /**
     * @brief Constructs a composite sink forwarding to other sinks.
     * @param name Sink name.
     * @param level Minimum log level.
     * @param sinks List of underlying sinks.
     */
    Sink(std::string name,
         Level level,
         std::vector<std::shared_ptr<Sink>> sinks)
        : name_(std::move(name)),
          level_(level),
          thread_info_type_(),
          max_message_length_(),
          max_buffer_size_(),
          latency_(),
          at_fault_(),
          events_(1, 0),  // Unused: events go to the underlying sinks
          underlying_sinks_(std::move(sinks)) {};

    /**
     * @brief Gets the sink name.
     * @return Sink name.
     */
    const std::string &name() const noexcept {
      return name_;
    }

    /**
     * @brief Gets the minimum log level accepted by this sink.
     * @return Minimum log level.
     */
    Level level() const noexcept {
      return level_;
    }

    /**
     * @brief Logs an event.
     * @tparam Format Format string type.
     * @tparam Args Additional argument types.
     * @param name Logger name.
     * @param level Log level.
     * @param format Format string.
     * @param args Formatting arguments.
     */
    template <typename Format, typename... Args>
    void push(std::string_view name,
              Level level,
              const Format &format,
              const Args &...args) noexcept(IF_RELEASE) {
      if (level_ < level or level == Level::OFF or level == Level::IGNORE) {
        return;
      }
      if (underlying_sinks_.empty()) {
        for (size_t attempt = 0;; ++attempt) {
          {
            auto node = events_.put(name,
                                    thread_info_type_,
                                    level,
                                    format,
                                    max_message_length_,
                                    args...);

            // Event is queued successfully
            LIKELY_IF((bool)node) {
              break;
            }
          }
          // Buffer full: get it drained and retry. async_flush() wakes the
          // worker or, without one, drains here unless another thread does;
          // after a short spin give way to the draining thread. The last
          // resort is flush(): it waits for that thread and drains the rest
          // here, which matters when the worker itself logs
          if (attempt >= 1024) {
            flush();
          } else {
            if (attempt >= 64) {
              std::this_thread::yield();
            }
            async_flush();
          }
        }

        // Flush right away at zero latency. Otherwise wake the worker in
        // advance, before the buffer is full and producers have to flush it
        // themselves
        if (latency_ == std::chrono::milliseconds::zero()) {
          flush();
        } else if (events_.size() >= events_.capacity() * 4 / 5) {
          async_flush();
        }
      } else {
        for (const auto &sink : underlying_sinks_) {
          sink->push(name, level, format, args...);
        }
      }
    }

    /**
     * @brief Writes queued events to the destination; returns when those
     * queued before the call are written.
     */
    virtual void flush() noexcept = 0;

    /**
     * @brief Requests writing queued events, does not wait for it.
     */
    virtual void async_flush() noexcept = 0;

    /**
     * @brief Writes queued events; returns when those queued before the
     * call are written.
     */
    virtual void sync_flush() noexcept = 0;

    /**
     * @brief Performs log data rotation (e.g., reopens log files).
     */
    virtual void rotate() noexcept = 0;

   protected:
    /**
     * @brief Takes the next event unless the limit is exhausted.
     * @param limit Number of events still allowed to take; decremented on
     * each call, even if nothing is taken.
     * @return The event, or nothing if the limit is exhausted or there is no
     * published event.
     */
    CircularBuffer<Event>::NodeRef take_event(size_t &limit) noexcept(
        IF_RELEASE) {
      if (limit == 0) {
        return {};
      }
      --limit;
      return events_.get();
    }

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    const std::string name_;                   ///< Sink name.
    Level level_;                              ///< Minimum accepted log level.
    const ThreadInfoType thread_info_type_;    ///< Thread info logging type.
    const size_t max_message_length_;          ///< Max message length.
    const size_t max_buffer_size_;             ///< Max buffer size.
    const std::chrono::milliseconds latency_;  ///< Max write delay.
    const AtFaultReactionType at_fault_;       ///< Behavior on write failure.
    CircularBuffer<Event> events_;             ///< Event buffer.
    const std::vector<std::shared_ptr<Sink>> underlying_sinks_{};  ///< Sinks.
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
  };

}  // namespace soralog

#undef LIKELY_IF
