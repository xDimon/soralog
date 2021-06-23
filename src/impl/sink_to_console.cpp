/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/sink_to_console.hpp>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string_view>

#include <fmt/chrono.h>
#include <fmt/color.h>

namespace soralog {

  namespace {

    using namespace std::chrono_literals;

    namespace fmt_internal {
#if FMT_VERSION >= 70000
  using namespace fmt::detail; // NOLINT
#else
  using namespace fmt::internal; // NOLINT
#endif
    }  // namespace fmt_internal

    // Separator is using between logical parts of log record.
    // Might be any substring or symbol: space, tab, etc.
    // Couple of space is selected to differ of single space
    constexpr std::string_view separator = "  ";

    constexpr std::array<fmt::color, static_cast<size_t>(Level::TRACE) + 1>
        level_to_color_map{
            fmt::color::brown,         // OFF
            fmt::color::red,           // CRITICAL
            fmt::color::orange_red,    // ERROR
            fmt::color::orange,        // WARNING
            fmt::color::forest_green,  // INFO
            fmt::color::dark_green,    // VERBOSE
            fmt::color::medium_blue,   // DEBUG
            fmt::color::gray,          // TRACE
        };

    template <typename... Args>
    inline void pass(Args &&... styles) {}

    enum V {};
    template <typename T>
    V put_style(char *&ptr, T style) {
      auto size = std::end(style) - std::begin(style);
      std::memcpy(ptr, std::begin(style), size);
      ptr += size;  // NOLINT
      return {};
    }

    template <typename... Args>
    void put_style(char *&ptr, Args &&... styles) {
      pass(put_style(ptr, std::forward<Args>(styles))...);
    };

    void put_reset_style(char *&ptr) {
      const auto &style = fmt_internal::data::reset_color;
      auto size = std::end(style) - std::begin(style) - 1;
      std::memcpy(ptr, std::begin(style), size);
      ptr = ptr + size;  // NOLINT
    }

    void put_level_style(char *&ptr, Level level) {
      assert(level <= Level::TRACE);
      auto color = level_to_color_map[static_cast<size_t>(level)];  // NOLINT
      put_style(ptr, fmt_internal::make_foreground_color<char>(color),
                fmt_internal::make_emphasis<char>(fmt::emphasis::bold));
    }

    void put_name_style(char *&ptr) {
      put_style(ptr, fmt_internal::make_emphasis<char>(fmt::emphasis::bold));
    }

    void put_text_style(char *&ptr, Level level) {
      assert(level <= Level::TRACE);
      if (level <= Level::ERROR) {
        put_style(ptr, fmt_internal::make_emphasis<char>(fmt::emphasis::bold));
      } else if (level >= Level::DEBUG) {
        put_style(ptr,
                  fmt_internal::make_emphasis<char>(fmt::emphasis::italic));
      }
    }

    void put_separator(char *&ptr) {
      for (auto c : separator) {
        *ptr++ = c;  // NOLINT
      }
    }

    void put_level(char *&ptr, Level level) {
      const char *const end = ptr + 8;  // NOLINT
      const char *str = levelToStr(level);
      do {
        *ptr++ = *str++;  // NOLINT
      } while (*str != '\0');
      while (ptr < end) {
        *ptr++ = ' ';  // NOLINT
      }
    }

    void put_level_short(char *&ptr, Level level) {
      *ptr++ = levelToChar(level);  // NOLINT
    }

    template <typename T>
    void put_string(char *&ptr, const T &name) {
      for (auto c : name) {
        *ptr++ = c;  // NOLINT
      }
    }

    template <typename T>
    void put_string(char *&ptr, const T &name, size_t width) {
      if (width == 0)
        return;
      for (auto c : name) {
        if (c == '\0' or width == 0)
          break;
        *ptr++ = c;  // NOLINT
        --width;
      }
      while (width--) *ptr++ = ' ';  // NOLINT
    }

  }  // namespace

  SinkToConsole::SinkToConsole(std::string name, bool with_color,
                               std::optional<ThreadInfoType> thread_info_type,
                               std::optional<size_t> capacity,
                               std::optional<size_t> buffer_size,
                               std::optional<size_t> latency)
      : Sink(std::move(name), thread_info_type.value_or(ThreadInfoType::NONE),
             capacity.value_or(1u << 6),      // 64 events
             buffer_size.value_or(1u << 17),  // 128 Kb
             latency.value_or(200)),          // 200 ms
        with_color_(with_color),
        buff_(max_buffer_size_) {
    if (latency_ != std::chrono::milliseconds::zero()) {
      sink_worker_ = std::make_unique<std::thread>([this] { run(); });
    }
  }

  SinkToConsole::~SinkToConsole() {
    if (latency_ != std::chrono::milliseconds::zero()) {
      need_to_finalize_.store(true, std::memory_order_release);
      async_flush();
      sink_worker_->join();
      sink_worker_.reset();
    } else {
      flush();
    }
  }

  void SinkToConsole::async_flush() noexcept {
    if (latency_ != std::chrono::milliseconds::zero()) {
      need_to_flush_.store(true, std::memory_order_release);
      condvar_.notify_one();
    } else {
      flush();
    }
  }

  void SinkToConsole::flush() noexcept {
    bool false_v = false;
    if (not flush_in_progress_.compare_exchange_strong(
            false_v, true, std::memory_order_acq_rel)) {
      return;
    }

    auto *const begin = buff_.data();
    auto *const end = buff_.data() + buff_.size();  // NOLINT
    auto *ptr = begin;

    decltype(1s / 1s) psec = 0;
    std::tm tm{};
    std::array<char, 17> datetime{};  // "00.00.00 00:00:00"

    while (true) {
      auto node = events_.get();
      if (node) {
        const auto &event = *node;

        const auto time = event.timestamp().time_since_epoch();
        const auto sec = time / 1s;
        const auto usec = time % 1s / 1us;

        if (psec != sec) {
          tm = fmt::localtime(sec);
          fmt::format_to_n(datetime.data(), datetime.size(),
                           "{:0>2}.{:0>2}.{:0>2} {:0>2}:{:0>2}:{:0>2}",
                           tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday,
                           tm.tm_hour, tm.tm_min, tm.tm_sec);
          psec = sec;
        }

        // Timestamp

        std::memcpy(ptr, datetime.data(), datetime.size());
        ptr = ptr + datetime.size();  // NOLINT

        if (with_color_) {
          const auto &style =
              fmt_internal::make_foreground_color<char>(fmt::color::gray);

          auto size = std::end(style) - std::begin(style);
          std::memcpy(ptr, std::begin(style),
                      std::end(style) - std::begin(style));
          ptr = ptr + size;  // NOLINT
        }

        ptr = fmt::format_to_n(ptr, end - ptr, ".{:0>6}", usec).out;

        if (with_color_) {
          put_reset_style(ptr);
        }

        put_separator(ptr);

        // Thread

        switch (thread_info_type_) {
          case ThreadInfoType::NAME:
            put_string(ptr, event.thread_name(), 15);
            put_separator(ptr);
            break;

          case ThreadInfoType::ID:
            ptr = fmt::format_to_n(ptr, end - ptr, "T:{:<6}",
                                   event.thread_number())
                      .out;
            put_separator(ptr);
            break;

          default:
            break;
        }

        // Level

        if (with_color_) {
          put_level_style(ptr, event.level());
        }
        put_level(ptr, event.level());
        if (with_color_) {
          put_reset_style(ptr);
        }

        put_separator(ptr);

        // Name

        if (with_color_) {
          put_name_style(ptr);
        }
        put_string(ptr, event.name());
        if (with_color_) {
          put_reset_style(ptr);
        }

        put_separator(ptr);

        // Message

        if (with_color_) {
          put_text_style(ptr, event.level());
        }
        put_string(ptr, event.message());
        if (with_color_) {
          put_reset_style(ptr);
        }

        *ptr++ = '\n';  // NOLINT

        size_ -= event.message().size();
      }

      if ((end - ptr) < sizeof(Event) or not node
          or std::chrono::steady_clock::now()
              >= next_flush_.load(std::memory_order_acquire)) {
        next_flush_.store(std::chrono::steady_clock::now() + latency_,
                          std::memory_order_release);
        std::cout.write(begin, ptr - begin);
        ptr = begin;
      }

      if (not node) {
        bool true_v = true;
        if (need_to_flush_.compare_exchange_weak(true_v, false,
                                                 std::memory_order_acq_rel)) {
          std::cout.flush();
        }
        break;
      }
    }

    flush_in_progress_.store(false, std::memory_order_release);
  }

  void SinkToConsole::run() {
    util::setThreadName("log:" + name_);

    next_flush_.store(std::chrono::steady_clock::now(),
                      std::memory_order_relaxed);

    while (true) {
      {
        std::unique_lock lock(mutex_);
        if (condvar_.wait_until(lock,
                                next_flush_.load(std::memory_order_relaxed))
            == std::cv_status::no_timeout) {
          if (not need_to_flush_.load(std::memory_order_relaxed)
              and not need_to_finalize_.load(std::memory_order_relaxed)) {
            continue;
          }
        }
      }

      flush();

      if (need_to_finalize_.load(std::memory_order_acquire)
          && events_.size() == 0) {
        return;
      }
    }
  }
}  // namespace soralog
