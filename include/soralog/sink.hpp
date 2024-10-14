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
   * This is base class of all sink.
   * It is accumulate events in inner lock-free circular buffer and drop it into
   * destination place on demand or condition
   */
  class Sink {
   public:
    enum class ThreadInfoType : uint8_t {
      NONE,  //!< No log thread info
      NAME,  //!< Log thread name
      ID     //!< Log thread id
    };

    Sink() = delete;
    Sink(const Sink &) = delete;
    Sink(Sink &&) noexcept = delete;
    virtual ~Sink() = default;
    Sink &operator=(const Sink &) = delete;
    Sink &operator=(Sink &&) noexcept = delete;

    Sink(std::string name,
         Level level,
         ThreadInfoType thread_info_type,
         size_t max_events,
         size_t max_message_length,
         size_t max_buffer_size,
         size_t latency)
        : name_(std::move(name)),
          level_(level),
          thread_info_type_(thread_info_type),
          max_message_length_(max_message_length),
          max_buffer_size_(max_buffer_size),
          latency_(latency),
          events_(max_events, max_message_length) {
      // Auto-fix buffer size
      if (max_buffer_size_ < max_message_length * 2) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast,-warnings-as-errors)
        const_cast<size_t &>(max_buffer_size_) = max_message_length * 2;
      }
    }

    Sink(std::string name,
         Level level,
         std::vector<std::shared_ptr<Sink>> sinks)
        : name_(std::move(name)),
          level_(level),
          thread_info_type_(),
          max_message_length_(),
          max_buffer_size_(),
          latency_(),
          events_(0, 0),
          underlying_sinks_(std::move(sinks)) {};

    /**
     * @returns name of sink
     */
    const std::string &name() const noexcept {
      return name_;
    }

    /**
     * @returns minimal level which sink will accept
     */
    Level level() const noexcept {
      return level_;
    }

    /**
     * Emplaces new log event
     * @param name is name of logger
     * @param level is level log event
     * @param format is format of message
     * @param args arguments is of log message
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
        while (true) {
          {
            auto node = events_.put(name,
                                    thread_info_type_,
                                    level,
                                    format,
                                    max_message_length_,
                                    args...);

            // Event is queued successfully
            LIKELY_IF((bool)node) {
              size_ += node->message().size();
              break;
            }
          }

          // Events queue is full. Flush immediately and try to push again
          flush();
        }

        if (latency_ == std::chrono::milliseconds::zero()) {
          flush();
        } else if (size_ >= max_buffer_size_ * 4 / 5) {
          async_flush();
        }
      } else {
        for (const auto &sink : underlying_sinks_) {
          sink->push(name, level, format, args...);
        }
      }
    }

    /**
     * Does writing all events in destination place immediately
     */
    virtual void flush() noexcept = 0;

    /**
     * Does writing all events in destination place asynchronously
     */
    virtual void async_flush() noexcept = 0;

    /**
     * Does some actions to rorate log data (e.g. reopen log-file)
     */
    virtual void rotate() noexcept = 0;

   protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    const std::string name_;
    Level level_;
    const ThreadInfoType thread_info_type_;
    const size_t max_buffer_size_;
    const std::chrono::milliseconds latency_;
    const size_t max_message_length_;
    CircularBuffer<Event> events_;
    std::atomic_size_t size_ = 0;
    const std::vector<std::shared_ptr<Sink>> underlying_sinks_{};
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
  };

}  // namespace soralog

#undef LIKELY_IF
