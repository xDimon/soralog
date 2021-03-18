/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_SINK
#define SORALOG_SINK

#include <memory>
#include <string>
#include <string_view>

#include <soralog/circular_buffer.hpp>
#include <soralog/event.hpp>

#ifdef NDEBUG
#define IF_RELEASE true
#else
#define IF_RELEASE false
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
    enum class ThreadFlag {
      NONE,  //!< No log thread info
      NAME,  //!< Log thread name
      ID     //!< Log thread id
    };

    Sink() = delete;
    Sink(const Sink &) = delete;
    Sink(Sink &&) noexcept = delete;
    virtual ~Sink() = default;
    Sink &operator=(Sink const &) = delete;
    Sink &operator=(Sink &&) noexcept = delete;

    Sink(std::string name, ThreadFlag thread_flag, size_t max_events,
         size_t max_buffer_size)
        : name_(std::move(name)),
          thread_flag_(thread_flag),
          events_(max_events),
          max_buffer_size_(max_buffer_size) {
      assert(max_buffer_size_ >= sizeof(Event));
    };

    /**
     * @returns name of sink
     */
    const std::string &name() const noexcept {
      return name_;
    }

    /**
     * Emplaces new log event
     * @param name is name of logger
     * @param level is level log event
     * @param format is format of message
     * @param args arguments is of log message
     */
    template <typename... Args>
    void push(std::string_view name, Level level, std::string_view format,
              const Args &... args) noexcept(IF_RELEASE) {
      while (true) {
        auto node = events_.put(name, thread_flag_, level, format, args...);

        // Event is queued successfully
        if (node) {
          size_ += node->message().size();
          node.release();
          break;
        }

        // Events queue is full. Flush and try to push again
        flush();
      }

      if (size_ >= max_buffer_size_ - sizeof(Event)) {
        flush();
      }
    }

    /**
     * Does writing all events in destination place immediately
     */
    virtual void flush() noexcept = 0;

    /**
     * Does some actions to rorate log data (e.g. reopen log-file)
     */
    virtual void rotate() noexcept = 0;

   protected:
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    const std::string name_;
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    ThreadFlag thread_flag_;
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    CircularBuffer<Event> events_;
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    const size_t max_buffer_size_;
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    size_t size_ = 0;
  };

}  // namespace soralog

#endif  // SORALOG_SINK
