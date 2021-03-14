/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sink_to_console.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string_view>

#include <fmt/chrono.h>
#include <fmt/color.h>

namespace soralog {

  namespace {
    using namespace std::chrono_literals;

    constexpr std::string_view separator = "  ";

    constexpr auto level_to_bg(Level level) {
      using cl = fmt::color;
      auto mbc = [](cl cl) {
        return fmt::internal::make_background_color<char>(cl);
      };
      switch (level) {
        case Level::OFF:
        default:
          return mbc(cl::black);
        case Level::CRITICAL:
          return mbc(cl::red);
        case Level::ERROR:
          return mbc(cl::orange);
        case Level::WARN:
          return mbc(cl::yellow);
        case Level::INFO:
          return mbc(cl::green);
        case Level::VERBOSE:
          return mbc(cl::dark_green);
        case Level::DEBUG:
          return mbc(cl::blue);
        case Level::TRACE:
          return mbc(cl::gray);
      }
    }

    constexpr auto level_to_fg(Level level) {
      using cl = fmt::color;
      auto mfc = [](cl cl) {
        return fmt::internal::make_foreground_color<char>(cl);
      };
      switch (level) {
        case Level::OFF:
        default:
          return mfc(cl::black);
        case Level::CRITICAL:
          return mfc(cl::red);
        case Level::ERROR:
          return mfc(cl::orange_red);
        case Level::WARN:
          return mfc(cl::orange);
        case Level::INFO:
          return mfc(cl::dark_green);
        case Level::VERBOSE:
          return mfc(cl::green);
        case Level::DEBUG:
          return mfc(cl::blue);
        case Level::TRACE:
          return mfc(cl::gray);
      }
    }

    constexpr std::array<const char *, static_cast<uint8_t>(Level::TRACE) + 1>
        level_to_bgcolor_map = [] {
          std::array<const char *, static_cast<uint8_t>(Level::TRACE) + 1> r{};
          r[static_cast<uint8_t>(Level::OFF)] = "?Unknown";
          r[static_cast<uint8_t>(Level::CRITICAL)] = "Critical";
          r[static_cast<uint8_t>(Level::ERROR)] = "Error";
          r[static_cast<uint8_t>(Level::WARN)] = "Warning";
          r[static_cast<uint8_t>(Level::INFO)] = "Info";
          r[static_cast<uint8_t>(Level::VERBOSE)] = "Verbose";
          r[static_cast<uint8_t>(Level::DEBUG)] = "Debug";
          r[static_cast<uint8_t>(Level::TRACE)] = "Trace";
          return r;
        }();

    void put_separator(char *&ptr) {
      for (auto c : separator) {
        *ptr++ = c;  // NOLINT
      }
    }

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
      const auto &style = fmt::internal::data::reset_color;
      auto size = std::end(style) - std::begin(style) - 1;
      std::memcpy(ptr, std::begin(style), size);
      ptr = ptr + size;  // NOLINT
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

  }  // namespace

  SinkToConsole::SinkToConsole(std::string name, bool with_color,
                               size_t events_capacity, size_t buffer_size,
                               size_t latency_ms)
      : Sink(std::move(name), events_capacity, buffer_size),
        with_color_(with_color),
        buffer_size_(buffer_size),
        latency_(latency_ms),
        buff_(buffer_size_) {
    sink_worker_ = std::make_unique<std::thread>([this] { run(); });
  }

  SinkToConsole::~SinkToConsole() {
    need_to_finalize_ = true;
    flush();
    sink_worker_->join();
    sink_worker_.reset();
  }

  void SinkToConsole::flush() noexcept {
    need_to_flush_ = true;
    condvar_.notify_one();
  }

  void SinkToConsole::run() {
    auto next_flush = std::chrono::steady_clock::now();

    while (true) {
      if (events_.size() == 0) {
        if (need_to_finalize_) {
          return;
        }
      }

      std::unique_lock lock(mutex_);
      if (not condvar_.wait_for(lock, std::chrono::milliseconds(100),
                                [this] { return events_.size() > 0; })) {
        continue;
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

          if (with_color_) {
            const auto &style = fmt::internal::make_foreground_color<char>(
                fmt::terminal_color::black);

            auto size = std::end(style) - std::begin(style);
            std::memcpy(ptr, std::begin(style),
                        std::end(style) - std::begin(style));
            ptr = ptr + size;  // NOLINT
          }

          std::memcpy(ptr, datetime.data(), datetime.size());
          ptr = ptr + datetime.size();  // NOLINT

          if (with_color_) {
            const auto &style =
                fmt::internal::make_foreground_color<char>(fmt::color::gray);

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

          if (with_color_) {
            put_style(ptr, level_to_fg(event.level()),
                      fmt::internal::make_emphasis<char>(fmt::emphasis::bold));
          }
          put_level(ptr, event.level());
          if (with_color_) {
            put_reset_style(ptr);
          }

          put_separator(ptr);

          if (with_color_) {
            put_style(ptr,
                      fmt::internal::make_emphasis<char>(fmt::emphasis::bold));
          }
          put_string(ptr, event.name());
          if (with_color_) {
            put_reset_style(ptr);
          }

          put_separator(ptr);

          if (with_color_) {
            put_style(ptr,
                      event.level() == Level::TRACE
                          ? fmt::internal::make_foreground_color<char>(
                              fmt::color::dark_gray)
                          : event.level() == Level::DEBUG
                              ? fmt::internal::make_foreground_color<char>(
                                  fmt::color::gray)
                              : event.level() == Level::VERBOSE
                                  ? fmt::internal::make_foreground_color<char>(
                                      fmt::color::dim_gray)
                                  : fmt::internal::make_foreground_color<char>(
                                      fmt::color::black));
            if (event.level() <= Level::ERROR)
              put_style(
                  ptr, fmt::internal::make_emphasis<char>(fmt::emphasis::bold));
          }
          put_string(ptr, event.message());
          if (with_color_) {
            put_reset_style(ptr);
          }

          *ptr++ = '\n';  // NOLINT

          size_ -= event.message().size();
        }

        if ((end - ptr) < sizeof(Event) or not node
            or std::chrono::steady_clock::now() >= next_flush) {
          next_flush = std::chrono::steady_clock::now() + latency_;
          std::cout.write(begin, ptr - begin);
          ptr = begin;
        }

        if (not node) {
          bool true_v = true;
          if (need_to_flush_.compare_exchange_weak(true_v, false)) {
            std::cout.flush();
          }
          if (need_to_finalize_) {
            return;
          }
          break;
        }
      }
    }
  }
}  // namespace soralog
