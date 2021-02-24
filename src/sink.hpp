
/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XLOG_SINK
#define XLOG_SINK

#include <memory>
#include <queue>

#include <circular_buffer.hpp>
#include <event.hpp>
#include <sink_types.hpp>

#ifdef NDEBUG
#define IF_RELEASE true
#else
#define IF_RELEASE false
#endif

namespace xlog {

  class Sink {
   public:
    Sink() : events_(std::make_unique<decltype(events_)::element_type>()){};
    Sink(const Sink &) = delete;
    Sink(Sink &&) noexcept = delete;
    virtual ~Sink() = default;
    Sink &operator=(Sink const &) = delete;
    Sink &operator=(Sink &&) noexcept = delete;

    virtual SinkType type() const noexcept = 0;

    virtual const std::string &name() const noexcept = 0;

    template <typename... Args>
    void push(std::string_view name, Level level, std::string_view format,
              Args... args) noexcept(IF_RELEASE) {
      while (true) {
        auto node =
            events_->put(name, level, format, std::forward<Args>(args)...);

        // Event is queued successfully
        if (node) {
          size_ += node->size;
          node.release();
          break;
        }

        // Events queue is full. Flush and try to push again
        flush();
      }

      if (size_ >= 100u << 20) {
        flush();
      }
    }

    virtual void flush() noexcept = 0;
    virtual void rotate() noexcept = 0;

   protected:
    std::unique_ptr<CircularBuffer<Event, 1 << 18>> events_;
    size_t size_ = 0;
  };

}  // namespace xlog

#endif  // XLOG_SINK
