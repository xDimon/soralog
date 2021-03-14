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

  class Sink {
   public:
    Sink() = delete;
    Sink(const Sink &) = delete;
    Sink(Sink &&) noexcept = delete;
    virtual ~Sink() = default;
    Sink &operator=(Sink const &) = delete;
    Sink &operator=(Sink &&) noexcept = delete;

    Sink(std::string name, size_t max_events, size_t max_buffer_size)
        : name_(std::move(name)),
          events_(max_events),
          max_buffer_size_(max_buffer_size) {
      assert(max_buffer_size_ >= sizeof(Event));
    };

    const std::string &name() const noexcept {
      return name_;
    }

    template <typename... Args>
    void push(std::string_view name, Level level, std::string_view format,
              const Args &... args) noexcept(IF_RELEASE) {
      while (true) {
        auto node = events_.put(name, level, format, args...);

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

    virtual void flush() noexcept = 0;
    virtual void rotate() noexcept = 0;

   protected:
    const std::string name_;
    CircularBuffer<Event> events_;
    const size_t max_buffer_size_;
    size_t size_ = 0;
  };

}  // namespace soralog

#endif  // SORALOG_SINK
